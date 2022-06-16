//
// Created by zhenkai on 2022/3/29.
//

#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <map>
#include "dict.h"

void Dict::Scan() {
    std::cout << "dict map size: " << m_last_load_map.size() << std::endl;
    for (auto &&item: m_last_load_map) {
        auto full_path = item.first;
        auto loader = item.second;
        std::cout << "--------" << full_path << "--------" << " mod time: " << loader->get_last_mod_time() << std::endl;
        auto dict_info = m_dict_map_offset[full_path];
        if (dict_info->empty()) {
            return;
        }
        for (auto &&info: *dict_info) {
            std::string line((char *) loader->get_mmap_addr() + std::get<0>(info.second), std::get<1>(info.second));
            std::cout << "key: " << info.first << " last_ch: " << (info.first)[info.first.size() - 1] << " line: "
                      << line
                      << std::endl;
        }
    }
}

std::string Dict::Find(const std::string &full_path, const std::string &key) {
    if (full_path.empty() || key.empty()) {
        return {};
    }

    std::unordered_map <std::string, std::shared_ptr <std::unordered_map <std::string,
            std::pair < int, short >>> > m_dict_map_offset_tmp;
    std::unordered_map <std::string, std::shared_ptr<Loader> > m_last_load_map_tmp;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_dict_map_offset_tmp = m_dict_map_offset;
        m_last_load_map_tmp = m_last_load_map;
    }
    if (m_dict_map_offset_tmp.count(full_path) > 0 && m_last_load_map_tmp.count(full_path) > 0) {
        if (m_dict_map_offset_tmp[full_path]->count(key) > 0) {
            auto dict_info = m_dict_map_offset_tmp[full_path];
            auto line = (*dict_info)[key];
            auto loader = m_last_load_map_tmp[full_path];
            return std::string((char *) loader->get_mmap_addr() + std::get<0>(line), std::get<1>(line));
        }
    }

    return {};
}

std::string Dict::Find(const char *full_path, const char *key) {
    if (full_path == NULL || key == NULL) {
        return {};
    }
    std::string f_path(full_path);
    std::string k(key);

    return Find(f_path, k);
}

int Dict::Load(const char *full_path) {
    if (full_path == NULL) {
        return -1;
    }
    std::string f_path(full_path);

    return Load(f_path);
}

int Dict::Load(const std::string &full_path) {
    if (full_path.empty()) {
        return -1;
    }
    if (m_last_load_map.find(full_path) == m_last_load_map.end()) {
        m_last_load_map.insert({full_path, std::make_shared<Loader>(full_path)});
        auto tmp = parse(m_last_load_map[full_path]);
        m_dict_map_offset.insert({full_path, std::move(tmp)});
        return 1;
    }

    return 0;
}

void Dict::Check_and_reload(const std::string &full_path, bool is_async_update) {
    if (m_is_reloading) {
        return;
    }
    if (!full_path.empty() && m_last_load_map.count(full_path) > 0) {
        auto loader = m_last_load_map[full_path];
        if (loader->check_reload(full_path)) {
            m_is_reloading = true;
            auto callback = [full_path, this]() {
                auto loader = std::make_shared<Loader>(full_path);
                auto tmp = parse(loader);
                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    if (m_dict_map_offset.count(full_path) > 0) {
                        m_dict_map_offset[full_path].swap(tmp);
                    } else {
                        m_dict_map_offset.insert({full_path, tmp});
                    }
                    m_last_load_map[full_path].swap(loader);
                }
                m_is_reloading = false;
            };
            if (is_async_update) {
                std::thread thr(callback);
                thr.detach();
            } else {
                callback();
            }
        }
    }
}

void Dict::Check_add_reload_all(bool is_async_update) {
    for (auto &&item: m_last_load_map) {
        const std::string &full_path = item.first;
        Check_and_reload(full_path, is_async_update);
    }
}

std::shared_ptr<std::unordered_map<std::string, std::pair<int, short>>>
Dict::parse(const std::shared_ptr<Loader> &loader) {
//    std::chrono::time_point<std::chrono::steady_clock> startTime = std::chrono::steady_clock::now();
    auto tmp = std::make_shared<std::unordered_map<std::string, std::pair<int, short>>>();
    char *mmap_addr = (char *) loader->get_mmap_addr();
    if (mmap_addr == NULL) {
        return tmp;
    }
    size_t mmap_size = loader->get_mmap_size();
    size_t offset = 0;

    // reserve
    size_t line_num = 0;
    while (offset < mmap_size) {
        char *ch = (char *) (mmap_addr + offset ++);
        if (*ch == '\n') {
            line_num ++;
        }
    }
    tmp->reserve(line_num);
    offset = 0;

    short key_len = 0;
    short length = 0;
    char *firstch;

    enum STATE {
        FLINE_FIRST, // 每一行的第一个字节
        FLINE, // 每一行的第一列
        OLINE // 第二行往后的列
    };
    STATE state = FLINE_FIRST;
    while (offset < mmap_size) {
        char *ch = (char *) (mmap_addr + offset);
        switch (state) {
            case FLINE_FIRST:
                if (*ch == '\t' || *ch == '\n' || *ch == ' ') {
                    state = OLINE;
                    goto LINE_END;
                }
                firstch = ch;
                key_len++;
                state = FLINE;
                break;
            case FLINE:
                if (*ch == '\t' || *ch == ' ') {
                    state = OLINE;
                } else {
                    key_len++;
                }
                break;
            case OLINE:
                if (*ch == '\n' || offset == mmap_size - 1) {
                    state = FLINE_FIRST;
                    if (key_len > 0 && length > 0 && firstch != NULL) {
                        tmp->insert({std::move(std::string(firstch, key_len)), {offset - length, length}});
                    }
                    length = 0;
                    key_len = 0;
                    firstch = NULL;
                    goto LINE_END;
                }
                break;
        }
        length++;
LINE_END:
        offset++;
    }

//    std::chrono::time_point<std::chrono::steady_clock> endTime = std::chrono::steady_clock::now();
//    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << " ms" << std::endl;

    return tmp;
}


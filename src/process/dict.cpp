//
// Created by zhenkai on 2022/4/12.
//

#include "dict.h"
#include "lockfile.h"
#include <thread>
#include <iostream>
#include <sstream>

Dict::Dict() {
    create_lock(LOCK_FILE_PATH);
}

static std::string gen_shared_name(const std::string &full_path) {
    char *filename = (char *)strrchr(full_path.c_str(), '/') + 1;
    struct stat sb;
    if (stat(full_path.c_str(), &sb) == -1) {
        return {};
    }
    std::ostringstream os;
    os << "/" << filename << ".idx." << sb.st_mtime;
    return os.str();
}

int Dict::Load(const std::string &full_path) {
    if (full_path.empty()) {
        return -1;
    }
    if (m_last_load_map.find(full_path) == m_last_load_map.end()) {
        if (rw_lock() > -1) {
            auto flag = -1;
            auto loader = std::make_shared<Loader>(full_path);
            std::string shared_name = gen_shared_name(full_path);
            auto idx = std::make_shared<IDX>(shared_name);
            if (!idx->Exist() && !parse(loader)) {
                goto UNLOCK;
            }
            if (idx->Exist()) {
                flag = 1;
                m_last_load_map.insert({full_path, loader});
                m_last_load_idx_map.insert({full_path, idx});
            }
UNLOCK:
            unlock();
            return flag;
        }
    }

    return 0;
}

int Dict::Load(const char *full_path) {
    if (full_path == NULL) {
        return -1;
    }
    std::string f_path(full_path);

    return Load(f_path);
}

std::string Dict::Find(const char *full_path, const char *key) {
    if (full_path == NULL || key == NULL) {
        return {};
    }
    std::string f_path(full_path);
    std::string k(key);

    return Find(f_path, k);
}

std::string Dict::Find(const std::string &full_path, const std::string &key) {
    if (full_path.empty() || key.empty() || m_last_load_map.count(full_path) == 0 ||
        m_last_load_idx_map.count(full_path) == 0) {
        return {};
    }
    std::shared_ptr<Loader> loader;
    std::shared_ptr<IDX> idx;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        loader = m_last_load_map[full_path];
        idx = m_last_load_idx_map[full_path];
    }
    auto val = idx->Get(key);
    if (!val.empty()) {
        char *len_s = const_cast<char *>(strrchr(val.c_str(), ',') + 1);
        return std::string((char *) loader->get_mmap_addr() + atoi(val.c_str()), atoi(len_s));
    }

    return {};
}

void Dict::Check_and_reload(const std::string &full_path, bool is_async_update) {
    if (m_reindexing) {
#ifdef DEBUG
        std::cout << "reindexing return, pid: " << getpid() << std::endl;
#endif
        return;
    }
    if (!full_path.empty() && m_last_load_map.count(full_path) > 0) {
        auto loader = m_last_load_map[full_path];
        if (loader->check_reload(full_path)) {
            m_reindexing = true;
            auto callback = [full_path, this]() {
                // 拿到锁重新生成idx
                if (rw_lock() > -1) {
#ifdef DEBUG
                    std::cout << " lock pid : " << getpid() << " thread id " << std::this_thread::get_id()
                              << std::endl;
#endif
                    // 判断idx有没有生成
                    std::string shared_name = gen_shared_name(full_path);
                    auto loader = std::make_shared<Loader>(full_path);
                    auto idx = std::make_shared<IDX>(shared_name);
                    if (!idx->Exist() && !parse(loader)) {
                        goto UNLOCK;
                    }
                    // 更新词典idx
                    {
                        std::unique_lock<std::mutex> lock(m_mutex);
                        m_last_load_map[full_path].swap(loader);
                        m_last_load_idx_map[full_path]->Unlink();
                        m_last_load_idx_map[full_path].swap(idx);
                    }
UNLOCK:
                    unlock();
                }
                m_reindexing = false;
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

bool Dict::parse(const std::shared_ptr<Loader> &loader) {
    //    std::chrono::time_point<std::chrono::steady_clock> startTime = std::chrono::steady_clock::now();
    char *mmap_addr = (char *) loader->get_mmap_addr();
    if (mmap_addr == NULL) {
        return false;
    }
    size_t mmap_size = loader->get_mmap_size();
    size_t offset = 0;

    // reserve
    size_t line_num = 0;
    size_t line_size = 0;
    size_t v_size = 0;
    size_t tmp = 0;
    while (offset < mmap_size) {
        char *ch = (char *) (mmap_addr + offset++);
        line_size++;
        if (*ch == '\n') {
            line_num++;
            tmp = offset - line_size;
            do {
                ++v_size;
            } while ((tmp = tmp / 10));
            v_size += strlen(",");
            tmp = line_size - 1;
            do {
                ++v_size;
            } while ((tmp = tmp / 10));
            line_size = 0;
        }
    }
    offset = 0;

    std::string shared_name = gen_shared_name(loader->get_full_path());
    IDX ht(shared_name);
    auto ret = ht.Hinit(line_num, v_size);
    if (!ret) {
#ifdef DEBUG
         std::cout << "hash table alloc error " << ret << std::endl;
#endif
        return false;
    }
    short key_len = 0;
    short length = 0;
    char *firstch;

    enum STATE {
        FLINE_FIRST, // 每一行的第一个字节
        FLINE, // 每一行的第一列
        OLINE // 第二行往后的列
    };
    STATE state = FLINE_FIRST;

    char value_tmp[100] = {0};
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
                        snprintf(value_tmp, 100, "%u,%u", offset - length, length);
                        ht.Set(firstch, key_len, value_tmp, strlen(value_tmp));
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

    return true;
}

void Dict::Scan() {
    std::cout << "dict map size: " << m_last_load_map.size() << std::endl;
    for (auto &&item: m_last_load_map) {
        auto full_path = item.first;
        auto loader = item.second;
        std::cout << "--------" << full_path << "--------" << " mod time: " << loader->get_last_mod_time() << std::endl;
        auto dict_idx = m_last_load_idx_map[full_path];
        dict_idx->Dump();
    }
}

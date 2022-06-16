//
// Created by zhenkai on 2022/3/29.
//

#ifndef PHP_EXT_DICT_H
#define PHP_EXT_DICT_H

#include <time.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include "loader.h"

class Dict {
public:
    int Load(const std::string &full_path);
    int Load(const char *full_path);
    void Check_and_reload(const std::string &full_path, bool is_async_update = false);
    void Check_add_reload_all(bool is_async_update = false);
    std::string Find(const std::string &full_path, const std::string &key);
    std::string Find(const char *full_path, const char *key);
    void Scan();

private:
    std::shared_ptr<std::unordered_map <std::string, std::pair<int, short> >> parse(const std::shared_ptr <Loader>& loader);

private:
    std::atomic<bool> m_is_reloading;
    std::mutex m_mutex;
    std::unordered_map <std::string, std::shared_ptr <std::unordered_map <std::string,
            std::pair <int, short >>> > m_dict_map_offset;
    std::unordered_map <std::string, std::shared_ptr<Loader> > m_last_load_map;
};


#endif //PHP_EXT_DICT_H

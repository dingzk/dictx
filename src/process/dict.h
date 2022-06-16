//
// Created by zhenkai on 2022/4/12.
//

#ifndef DICTX_DICT_H
#define DICTX_DICT_H

#include <time.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <limits.h>

#include "loader.h"
#include "hashtable.h"
#include "shmem.h"

#define LOCK_FILE_PATH "/tmp"

using IDX = HashTable<Shmem>;

class Dict {
public:
    Dict();
    int Load(const std::string &full_path);
    int Load(const char *full_path);
    void Check_and_reload(const std::string &full_path, bool is_async_update = true);
    void Check_add_reload_all(bool is_async_update = true);
    std::string Find(const std::string &full_path, const std::string &key);
    std::string Find(const char *full_path, const char *key);
    void Scan();

private:
    bool parse(const std::shared_ptr <Loader>& loader);

private:
    std::mutex m_mutex;
    std::atomic<bool> m_reindexing;
    std::unordered_map <std::string, std::shared_ptr<Loader> > m_last_load_map;
    std::unordered_map <std::string, std::shared_ptr<IDX> > m_last_load_idx_map;
};


#endif //DICTX_DICT_H

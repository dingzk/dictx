//
// Created by zhenkai on 2022/3/29.
//

#ifndef PHP_EXT_LOADER_H
#define PHP_EXT_LOADER_H


#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <string>
#include <unistd.h>

class Loader {
private:
    std::string m_full_path;
    char *mmap_addr;
    off_t offset;
    struct stat m_sb;

private:
    int mmap();

    void munmmap();

public:
    Loader(std::string full_path);

    char *mread(size_t size, size_t nmemb);

    void *get_mmap_addr();

    bool check_reload(const std::string &full_path);
    time_t get_last_mod_time();

    std::string get_full_path();

    size_t get_mmap_size();

    ~Loader();
};


#endif //PHP_EXT_LOADER_H

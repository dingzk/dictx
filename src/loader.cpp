//
// Created by zhenkai on 2022/3/29.
//

#include <random>
#include "loader.h"

#define CHECK_DELAY_TIME 1

Loader::Loader(std::string full_path) : m_full_path(std::move(full_path)), mmap_addr(NULL), offset(0) {
    this->mmap();
}

int Loader::mmap() {
    if (m_full_path.empty()) {
        return -1;
    }
    int fd = open(m_full_path.c_str(), O_RDONLY);
    if (fd < 0) {
        return -2;
    }
    if (fstat(fd, &m_sb) == -1) {
        close(fd);
        return -3;
    }
    mmap_addr = (char *) ::mmap(NULL, m_sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mmap_addr == MAP_FAILED) {
        close(fd);
        return -4;
    }
    close(fd);

    return 1;
}

char *Loader::mread(size_t size, size_t nmemb) {
    char *preptr = mmap_addr + offset;
    if (offset >= m_sb.st_size) {
        return NULL;
    }
    if (size > 0 && nmemb > 0) {
        offset += size * nmemb;
    }

    return preptr;
}

void *Loader::get_mmap_addr() {
    return mmap_addr;
}

size_t Loader::get_mmap_size() {
    return mmap_addr == NULL ? 0 : m_sb.st_size;
}

void Loader::munmmap() {
    if (mmap_addr != NULL) {
        ::munmap(mmap_addr, m_sb.st_size);
    }
}

bool Loader::check_reload(const std::string &full_path) {
    if (full_path.empty()) {
        return false;
    }
    int fd = open(full_path.c_str(), O_RDONLY);
    if (fd < 0) {
        return false;
    }
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        close(fd);
        return false;
    }
    close(fd);
    std::random_device rd;
    if (sb.st_mtime > m_sb.st_mtime + rd() % CHECK_DELAY_TIME) {
        return true;
    }

    return false;
}

time_t Loader::get_last_mod_time() {
    return m_sb.st_mtime;
}

std::string Loader::get_full_path() {
    return m_full_path;
}

Loader::~Loader() {
    this->munmmap();
}

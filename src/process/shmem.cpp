//
// Created by zhenkai on 2022/4/14.
//

#include "shmem.h"
#include <climits>
#include <cstring>

void *Shmem::OpenW(size_t requested_size) {
    if (m_shm_fd < 0) {
        m_shm_fd = shm_open(m_shared_name, O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (m_shm_fd > 0 && ftruncate(m_shm_fd, requested_size) < 0) {
            return nullptr;
        }
    } else {
        if (fcntl(m_shm_fd, F_SETFL, O_RDWR) < 0) {
            return nullptr;
        }
    }
    m_shm_addr = mmap(0, requested_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_shm_fd, 0);
    if (m_shm_addr == MAP_FAILED) {
        return nullptr;
    }

    return m_shm_addr;
}

void *Shmem::OpenR() {
    if (m_shm_fd < 0) {
        if ((m_shm_fd = shm_open(m_shared_name, O_RDONLY, 0644)) < 0) {
            return nullptr;
        }
    } else {
        if (fcntl(m_shm_fd, F_SETFL, O_RDONLY) < 0) {
            return nullptr;
        }
    }
    struct stat f_st;
    fstat(m_shm_fd, &f_st);
    m_shm_addr = mmap(NULL, f_st.st_size, PROT_READ, MAP_PRIVATE, m_shm_fd, 0);
    if (m_shm_addr == MAP_FAILED) {
        return nullptr;
    }

    return m_shm_addr;
}

bool Shmem::Exist() {
    int shm_fd = shm_open(m_shared_name, O_RDONLY, 0644);
    if (shm_fd < 0) {
        return false;
    }
    close(shm_fd);

    return true;
}

bool Shmem::Unlink() {
    if (shm_unlink(m_shared_name) > 0) {
        return true;
    }

    return false;
}

void *Shmem::GetShmAddr() {
    return m_shm_addr;
}

std::string Shmem::GetSharedName(bool flag) const {
    if (flag) {
        return m_shared_name;
    }
    char buf[NAME_MAX] = {'\0'};
    char file_path[NAME_MAX] = {'\0'};
    snprintf(buf, sizeof(buf), "/proc/self/fd/%d", m_shm_fd);
    int r = readlink(buf, file_path, sizeof(file_path) - 1);
    if (r < 0) {
        return {};
    }

    return std::string(file_path, strlen(file_path));
}

Shmem::~Shmem() {
    if (m_shm_fd <= 0) {
        return;
    }
    if (m_shm_addr) {
        struct stat f_st;
        fstat(m_shm_fd, &f_st);
        msync(m_shm_addr, f_st.st_size, MS_INVALIDATE |  MS_SYNC);
        munmap(m_shm_addr, f_st.st_size);
    }
    close(m_shm_fd);
}

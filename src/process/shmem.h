//
// Created by zhenkai on 2022/4/14.
//

#ifndef DICTX_SHMEM_H
#define DICTX_SHMEM_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string>

class Shmem {
public:
    explicit Shmem(const std::string &shared_name) : m_shared_name_str(shared_name),
                                                     m_shared_name(m_shared_name_str.c_str()),
                                                     m_shm_addr(nullptr), m_shm_fd(-1) {}

    explicit Shmem(const char *shared_name) : m_shared_name_str(std::move(std::string(shared_name))),
                                              m_shared_name(m_shared_name_str.c_str()),
                                              m_shm_addr(nullptr), m_shm_fd(-1) {}

    ~Shmem();

    void *OpenW(size_t requested_size);

    void *OpenR();

    bool Exist();

    bool Unlink();

    void *GetShmAddr();

    std::string GetSharedName(bool flag = true) const;

private:
    std::string m_shared_name_str;
    const char *m_shared_name;
    void *m_shm_addr;
    int m_shm_fd;

};


#endif //DICTX_SHMEM_H

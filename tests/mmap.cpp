//
// Created by zhenkai on 2022/4/7.
//

#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <string>
#include <unistd.h>
#include <string>

int main(void) {
    const std::string m_full_path = "/data1/apache2/config/dict/all_exempt.dict";

    int fd = open(m_full_path.c_str(), O_RDONLY);
    if (fd < 0) {
        return -2;
    }
    struct stat m_sb;
    if (fstat(fd, &m_sb) == -1) {
        close(fd);
        return -3;
    }
    char *mmap_addr = (char *) ::mmap(NULL, m_sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mmap_addr == MAP_FAILED) {
        close(fd);
        return -4;
    }
    close(fd);

    size_t i = 0;
    while (i < m_sb.st_size) {
       char ch = *(mmap_addr + i);
        if (i == m_sb.st_size - 1) {
            std::cout << ch << i << std::endl;
        }
       i++;
    }

    ::munmap(mmap_addr, m_sb.st_size);

    return 0;
}
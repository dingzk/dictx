#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

int main(void) {

    int shmfd = shm_open("/shm_test", O_RDONLY, 0644);

//    char *p = (char *)mmap(NULL, 20, PROT_READ, MAP_PRIVATE, shmfd, 0);
    char *p = (char *)mmap(NULL, 20, PROT_READ, MAP_SHARED, shmfd, 0);

    char demo[10] = {0};

    memcpy(demo, p, 5);

    // read(shmfd, demo, 5);

    std::cout << demo << std::endl;

    sleep(30);
    memset(demo, 0, 10);
    memcpy(demo, p, 5);
    std::cout << demo << std::endl;

    return 0;
}
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

    int shmfd = shm_open("/shm_test", O_RDWR|O_CREAT|O_TRUNC, 0644);

    shm_open("/shm_test", O_RDONLY, 0644);
    shm_open("/shm_test", O_RDONLY, 0644);

    char *p = (char *)mmap(NULL, 20, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);

    memset(p, 0, 20);

    const char *demo = "hello world";

//    strcpy(p, demo);

     write(shmfd, demo, strlen(demo) + 1);

//     close(shmfd);

    sleep(20);

    return 0;
}
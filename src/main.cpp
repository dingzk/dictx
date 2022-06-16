//
// Created by zhenkai on 2022/3/30.
//
#include <sys/types.h>
#include <sys/wait.h>

#include <iostream>
#include <thread>
#include <unistd.h>
#include "dictapi.h"

void test_thread(const char *full_path) {
    std::thread t([full_path]() {
        while (true) {
            dict_find(full_path, "50000");
        }
    });
    t.detach();
}

void test_process(const char *full_path) {
    pid_t pid = fork();
    if (pid == 0) {
        std::cout << "child process " << getpid() << std::endl;

        while (true) {

            int i = 50000;
            while (i --) {
                dict_load(full_path);
                auto res1 = dict_find(full_path, std::to_string(i).c_str());
                if (res1) {
                    std::cout << "res1 " << res1 << std::endl;
                }
                dict_check_and_reload(full_path);
//                sleep(1);
            }
            sleep(1);
        }
        return;
    }
    pid = fork();
    if (pid == 0) {
        while(true) {
            int i = 50000;
            while (i --) {
                dict_load(full_path);
                auto res2 = dict_find(full_path, std::to_string(i).c_str());
                if (res2) {
                    std::cout << "res2 " << res2 << std::endl;
                }
                dict_check_and_reload(full_path);
//                sleep(1);
            }
//            sleep(1);
        }
        return;
    }

    std::cout << "parent process " << getpid() << std::endl;
//    waitpid(pid, NULL, 0);
    wait(NULL);
}


int main(void)
{
    const char * full_path = "/data1/apache2/config/dict/all_exempt.dict";
//    int ret = dict_load(full_path);
//    std::cout << ret << std::endl;

//    auto res2 = dict_find(full_path, "20000");
//    if (res2) {
//        std::cout << "res2 " << res2 << std::endl;
//        free(res2);
//    }

//    test_thread(full_path);
    test_process(full_path);

    std::cout << "ok" << std::endl;

    dict_scan();
//    sleep(10);
//    dict_check_and_reload_all();
//    dict_scan();
//    sleep(20);


    return 0;

}

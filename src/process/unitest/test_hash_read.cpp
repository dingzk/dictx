//
// Created by zhenkai on 2022/4/18.
//

#include <iostream>

#include "shmem.h"
#include "hashtable.h"

int hashtable_test() {
    char *err = NULL;
    const char *shared_name = "/Dis_test";

    HashTable <Shmem> ht(shared_name);

    bool flag = ht.Set("a", "ok");
    std::cout << "set flag" << flag << std::endl;
    flag = ht.Set("a", "o1");
    std::cout << "set flag" << flag << std::endl;
    flag = ht.Set("b", "ok");
    std::cout << "set flag" << flag << std::endl;

    flag = ht.Set("b", "ok1");
    std::cout << "set flag" << flag << std::endl;

    std::cout << "get : " << ht.Get("a") << std::endl;

    flag = ht.Del("a");

    std::cout << "del flag" << flag << std::endl;

    std::cout << "get : " << ht.Get("b" ) << std::endl;
    std::cout << "get : " << ht.Get("a") << std::endl;

    ht.Dump();

//    sleep(20);

    return 0;
}

int main(void) {
//    sleep(20);
    hashtable_test();

//    sleep(25);


    return 0;
}
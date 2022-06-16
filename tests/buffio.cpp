//
// Created by zhenkai on 2022/4/7.
//

#include <fstream>
#include <iostream>

int main(void)
{
    const std::string m_full_path = "/data1/apache2/config/dict/all_exempt.dict";

    std::fstream f(m_full_path, std::fstream::in);

    char ch;
    if (f.is_open()) {
        std::cout << "open" << std::endl;
    } else {
        std::cout << "false" << std::endl;
    }
    char tmp = 0;
    while (f.get(ch)) {
        tmp = ch;
    }
    std::cout << tmp << std::endl;

    if (f.eof()) {
        std::cout << "eof reached" << std::endl;
    }

    f.close();

    return 0;
}
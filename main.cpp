#include <iostream>
#include "s_data.h"


void test_sdata()
{

    SData data;
    data.add_value<int>("teat_int", 1334);
    data.add_value<std::string>("test_string", "HELLO");

    auto res = data.get_value<int>("teat_int");
    auto str = data.get_value<std::string>("test_string");

    std::cout << "data-size : " << data.count() << std::endl;
    std::cout << "print all keys" << std::endl;
    for(const auto &key:data.keys())
    {
        std::cout << "    key:" << key << " Type:"<< SData::type_to_string(data.type_info(key))<< std::endl;
    }

    std::cout << "--------------------------------------------" << std::endl;

    std::size_t length;
    unsigned char *buffer;
    data.to_buffer(length, buffer);

    SData data_new(SData::from_buffer(length, buffer));
    std::cout << "data-size : " << data_new.count() << std::endl;
    std::cout << "print all keys" << std::endl;
    for(const auto &key:data_new.keys())
    {
        std::cout << "    key:" << key << " Type:"<< SData::type_to_string(data_new.type_info(key))<< std::endl;
    }
}

int main()
{
    test_sdata();
    return 0;
}
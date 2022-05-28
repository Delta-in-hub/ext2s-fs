#include <fcntl.h>
#include <string>
#include <cstdio>
#include "ext2like.hpp"
#include <iostream>

using namespace std;


int main(int argc, char **argv)
{
    auto t = last_group_calculation(8191);

    cout << get<0>(t) << endl;
    cout << get<1>(t) << endl;
    cout << get<2>(t) << endl;

    cout << roundup(5, 4) << endl;
}
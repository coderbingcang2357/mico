#include <string>
#include <cstring>
#include <iostream>
#include <vector>
#include <stdio.h>
#include "util.h"
using namespace std;



int main()
{
    std::vector<string> r;
    splitString("   file    2    333   f444", " ", &r);
    for (auto s : r)
    {
        std::cout << s << std::endl;
    }
    std::cout << "---\n";
    r.clear();
    splitString(std::string("   file    2    333   f444"), std::string(" "), &r);
    for (auto s : r)
    {
        std::cout << s << std::endl;
    }
    return 0;
}


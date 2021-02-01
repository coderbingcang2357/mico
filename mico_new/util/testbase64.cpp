#include "base64.h"
#include <iostream>
#include <vector>

int main() {
    char *p = "1234";
    std::string str = ::Base64Encode((unsigned char *)p, 4);
    std::vector<unsigned char> res = ::Base64Decode(str);
    std::cout << str << std::endl;
    std::cout << std::string((char *)&res[0], 4) << std::endl;
    if (std::string("\xff")[0] == 0xFF) {
        std::cout << "eq\n";
    } else {
        std::cout << "notq " << int(std::string("\xff")[0]) << std::endl;
    }
    return 0;
}

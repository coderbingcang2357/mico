#include "verfycodegen.h"
#include "sessionkeygen.h"

static char ascii_internal[] =
{
    '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',
    'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',
    'K',  'L',  'M',  'N',  'O',  'P',  'Q',  'R',  'S',  'T',
    'U',  'V',  'W',  'X',  'Y',  'Z',
    'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',
    'k',  'l',  'm',  'n',  'o',  'p',  'q',  'r',  's',  't',
    'u',  'v',  'w',  'x',  'y',  'z'
};

void genVerfyCode(int len, std::string *out)
{
    uint64_t rdnbr;
    for (int i = 0; i < len; i++)
    {
        rdnbr = ::getRandomNumber();
        out->push_back(ascii_internal[rdnbr % sizeof(ascii_internal)]);
    }
}

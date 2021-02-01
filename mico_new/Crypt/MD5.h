#ifndef MD5_H
#define MD5_H

#include <stdint.h>
#include <string>
#include <fstream>

typedef unsigned char byte;

using std::string;
using std::ifstream;

class MD5
{
public:
    MD5();
    MD5(const void *input, size_t length);
    MD5(const string &str);
    MD5(ifstream &in);
    void update(const void *input, size_t length);
    void update(const string &str);
    void update(ifstream &in);
    const byte* digest();
    string toString();
    void reset();

    void input(const char* in, size_t len);
    void output(char* out);
    void bytesToString(const byte *input, char* output);
    void stringTobytes(const char *input, byte* output);

private:
    void update(const byte *input, size_t length);
    void final();
    void transform(const byte block[64]);
    void encode(const uint32_t *input, byte *output, size_t length);
    void decode(const byte *input, uint32_t *output, size_t length);
    string bytesToHexString(const byte *input, size_t length);

    MD5(const MD5&);
    MD5& operator = (const MD5&);
    
    int charToHex(char ch);

private:
    uint32_t _state[4];    // state (ABCD)
    uint32_t _count[2];    // number of bits, modulo 2^64 (low-order word first) 
    byte _buffer[64];      // input buffer
    byte _digest[16];      // message digest
    bool _finished;        // calculate finished ?

    static const byte PADDING[64];  // padding for calculate
    static const char HEX[16];
    static const size_t BUFFER_SIZE = 1024;
};


#endif/*MD5_H*/

#ifndef I_TO_BYTEARRY_H
#define I_TO_BYTEARRY_H

#include <vector>

class IToByteArray
{
public:
    virtual ~IToByteArray(){}
    virtual void toByteArray(std::vector<char> *out) = 0;
};

class EncryptByteArray : public IToByteArray
{
public:
    EncryptByteArray(IToByteArray *target, std::vector<char> *enckey);
    void toByteArray(std::vector<char> *out);

private:
    std::vector<char> *m_enckey;
    IToByteArray      *m_bytearray;
};
#endif

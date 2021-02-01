#include <assert.h>
#include "itobytearray.h"
#include "Crypt/tea.h"

EncryptByteArray::EncryptByteArray(IToByteArray *target,
                                std::vector<char> *enckey)
    : m_enckey(enckey), m_bytearray(target) {}

void EncryptByteArray::toByteArray(std::vector<char> *out)
{
    std::vector<char> data;
    m_bytearray->toByteArray(&data);
    char buf[10 * 1024];
    // 加密
    int len = tea_enc((char *)&data[0], data.size(),
                buf,
                (char *)&(*m_enckey)[0],
                m_enckey->size());
    assert(uint32_t(len) < sizeof(buf));
    out->insert(out->end(), buf, buf + len);
}


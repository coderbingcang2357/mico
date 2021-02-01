#include "memoryhash.h"
#include <string.h>
#include <functional>
#include <assert.h>
#include <stdio.h>

uint64_t int64Hash(uint64_t key)
{
    key = (~key) + (key << 21); // key = (key << 21) - key - 1;
    key = key ^ (key >> 24);
    key = (key + (key << 3)) + (key << 8); // key * 265
    key = key ^ (key >> 14);
    key = (key + (key << 2)) + (key << 4); // key * 21
    key = key ^ (key >> 28);
    key = key + (key << 31);
    return key;
}

// 这是一个标志值, 随便取的
static int const InitFlag = 0x69feef96;

// 素数查找函数, 找小n的最大素数
int Sieve(int n)
{
    bool *a = new bool[n+1];
    for (int i = 2; i <= n; i++)  a[i] = true; 
    for (int i = 2; i*i <= n; i++)
    {
        if (!a[i])
            continue;

        for (int j = i; j*i <= n; j++)
        {
            a[j * i] = false;
        }
    }

    int i;
    for (i = n; i >= 2; --i)
    {
        if (a[i])
            break;
    }

    return i;
}

struct HashHead
{
    HashHead() : ptrlen(0),
            bucketsize(0),
            tableSize(0),
            maxelement(0),
            emptySlotSize(0)
    {}

    inline bool hasInited()
    {
        return inited == InitFlag;
    }

    int inited;
    int version;
    int ptrlen;
    int bucketsize;
    int tableSize;
    int maxelement;
    int datalen;
    int elementsize;
    int emptySlotSize;
    char *datastartPtr;
};

static const char Empty = 0;
static const char Used = 1;
static const char Delete = 2;

struct HashItem
{
    int checksum;
    char status; // empty 0, used: 1, delete: 2
    uint64_t key;
    int len;
    char data[0];
};

MemoryHash::MemoryHash() : hashhead(0)
{
}

MemoryHash::~MemoryHash()
{
}

bool MemoryHash::init(char *ptr, int len, int bucketsize)
{
    HashHead *head = (HashHead *)ptr;
    if (head->hasInited())
    {
        hashhead = head;
        hashhead->datastartPtr = ptr + sizeof(*head);
        return true;
    }
    hashhead = head;
    memset(ptr, 0, len);
    head->inited = InitFlag;
    head->version = 1;
    head->bucketsize = bucketsize + sizeof(HashItem);
    head->datalen = len - sizeof(*head);
    head->tableSize = head->datalen / head->bucketsize;
    head->tableSize = Sieve(head->tableSize);
    head->emptySlotSize = head->tableSize; // table还剩多少标记为Empty的slot
    head->maxelement = ((head->tableSize) * 2) / 3; // 保留一些余量, 否则会出错
    head->datastartPtr = ptr + sizeof(*head);
    head->elementsize = 0;

    return true;
}

int MemoryHash::maxElement()
{
    return hashhead->maxelement;
}

int MemoryHash::size()
{
    return hashhead->elementsize;
}

int MemoryHash::slotSize()
{
    return hashhead->tableSize;
}

void *MemoryHash::get(uint64_t key)
{
    HashItem *item = findItem(key);

    // 没找到
    if (item->status == Empty || item->status == Delete)
    {
        return 0;
    }
    // 找到了
    else if (item->key == key)
    {
        //return ((char *)item) + sizeof(*item);
        return item->data;
    }
    // 不可能
    else
    {
        assert(false);
        return 0;
    }
}

bool MemoryHash::remove(uint64_t key)
{
    HashItem *item = findItem(key);

    if (item->status != Empty
        && item->status != Delete
        && item->key == key)
    {
        hashhead->elementsize = hashhead->elementsize - 1;
        item->status = Delete;
        return true;
    }
    else
    {
        return false;
    }
}

// 这个函数只要不超过maxelement就不会出错
bool MemoryHash::set(uint64_t key, char *value, int len)
{
    if (hashhead->emptySlotSize == 1)
    {
        // 只剩一个标记为Empty的slot时就不可以再插入了, 否则可能导至findItem死
        // 循环
        // 彻底满了, 找不到任何一个没有使用的slot, 所有slot要么被使用要么被
        // 标记为Delete
        // full, 这里要不要进行一次垃圾收集??把标记为delete的项彻底删掉???
        // 通过rehash实现, 比较费时, 暂时不实现
        // rehash();
        // item = findItem(key);
        // assert(item != 0);
        return true;
    }

    HashItem *item = findItem(key);

    // 没找到
    if (item->status == Empty)
    {
        if (hashhead->elementsize >= hashhead->maxelement)
        {
            printf("memhash full.\n");
            return false;
        }
        else
        {
            // ok
            item->checksum = 0x41414141;
            item->status = Used;
            item->key = key;
            item->len = len;
            hashhead->elementsize++;
            hashhead->emptySlotSize--; // 用了一相标记为Empty的slot, 总数减一
            //char *data = ((char *)item) + sizeof(*item);
            char *data = item->data;
            memcpy(data, value, len);
            return true;
        }
    }
    // 找到了一个已经存在的 或都 一个被标志删除的, 就直接覆盖
    // 
    else
    {
        assert(item->status == Delete || item->status == Used);
        assert(item->key == key);

        item->checksum = 0x41414141;

        // 如果是新添加的就增加计数
        if (item->status == Delete)
            hashhead->elementsize++;
        // 只是修改, 啥都不干
        else if (item->status == Used)
            ;// donothing

        item->status = Used;
        item->key = key;
        item->len = len;


        //char *data = ((char *)item) + sizeof(*item);
        char *data = item->data;
        memcpy(data, value, len);
        return true;
    }
}

int retrycount = 0;
int maxretry = 0;
HashItem *MemoryHash::findItem(uint64_t key)
{
    uint64_t khash = int64Hash(key);
    // khash2不可以为0而且要比 tableSize 小否则可能导到死循环.
    // 目前这个值没用到
    uint64_t khash2 = 1;//(int64Hash(khash) % (hashhead->tableSize - 1)) + 1;
    int slot;
    int retry = 0;
    slot = nextSlot(khash, khash2, retry);
    HashItem *item = getItem(slot);
    for (;;)
    {
        if (item->status == Empty) // not used
        {
            break;
        }
        // used or deleted
        else 
        {
            assert(item->status == Used || item->status == Delete);

            // find
            if (item->key == key)
            {
                break;
            }
            // retry
            else
            {
                retry++;
                slot = nextSlot(khash, khash2, retry);
                item = getItem(slot);
            }
        }
    }
    if (retry > 0)
    {
        //printf("used: %d, retrys: %d\n", slot, retry);
        retrycount += retry;
        if (retry > maxretry)
            maxretry = retry;
    }
    return item;
}

inline HashItem *MemoryHash::getItem(int slot)
{
    HashItem *item = (HashItem *)(hashhead->datastartPtr  + slot * hashhead->bucketsize);
    return item;
}

// key目前没用到, 因为如果用key * i虽然冲突次数变少了但是速度却变慢了.
inline int MemoryHash::nextSlot(uint64_t hash, uint64_t key, int i)
{
    (void)key;
    //int32_t slot = (hash + key * i) % hashhead->tableSize;
    int32_t slot = (hash + i) % hashhead->tableSize;
    assert(slot >= 0);
    return slot;
}

// 调试用
void MemoryHash::printInfo()
{
    char *p = hashhead->datastartPtr;
    //int s = hashhead->tableSize * hashhead->bucketsize;
    //for (int i = 0; i < s; ++i)
    //{
    //    if (p[i] == 'a' || p[i] == 0x41)
    //        printf("%c", p[i]);
    //    else
    //    {
    //        printf(" ");
    //    }
    //}
    //printf("\n");
    for (int i = 0; i < hashhead->tableSize; ++i)
    {
        HashItem *item = ((HashItem *)p)  + i;
        if (item->status == Used)
        {
            printf("I");
        }
        else
        {
            printf(" ");
        }
        if (i % 80 == 0)
        {
            printf("\n");
        }
    }
}

MemoryHash::Iterator MemoryHash::iterator()
{
    MemoryHash::Iterator it(this);
    return it;
}

//MemoryHash::Iterator MemoryHash::end()
//{
//    MemoryHash::Iterator it(this);
//    return it;
//}

void MemoryHash::Iterator::next()
{
    HashItem *item;
    for (;;)
    {
        if (pos >= mhash->slotSize())
        {
            curitem = 0;
            return;
        }
        else
        {
            item = mhash->getItem(pos);
            if (item->status == Used) // ok find a used slot
            {
                ++iterCount;
                ++pos;
                curitem = item;
                return;
            }
            else
            {
                ++pos;
                curitem = 0;
            }
        }
    }
}

std::shared_ptr<std::pair<uint64_t, char *> > MemoryHash::Iterator::operator * ()
{
    if (curitem != 0)
    {
        return std::shared_ptr<std::pair<uint64_t, char *> >(
                        new std::pair<uint64_t, char *>(curitem->key, curitem->data));
    }
    else
    {
        return std::shared_ptr<std::pair<uint64_t, char *> >();
    }
}

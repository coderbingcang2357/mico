#ifndef memoryhash_h
#define memoryhash_h
#include <stdint.h>
#include <utility>
#include <memory>

struct HashHead;
struct HashItem;

class MemoryHash
{
public:
    MemoryHash();
    ~MemoryHash();
    bool init(char *ptr, int len, int bucketsize);
    void *get(uint64_t key);
    bool set(uint64_t key, char *value, int len);
    bool remove(uint64_t key);
    int maxElement();
    int slotSize();
    int size();
    void printInfo();

    class Iterator
    {
    public:
        Iterator(MemoryHash *mh) : iterCount(0), pos(0), curitem(0), mhash(mh) {}
        bool hasNext()
        {
            return iterCount < mhash->size();
        }

        void next();

        void operator ++ () // 前缀自加
        {
            next();
        }

        std::shared_ptr<std::pair<uint64_t, char *> > operator * ();

    private:
        int iterCount;
        int pos;
        HashItem *curitem;
        MemoryHash *mhash;
    };

    Iterator iterator();
    //Iterator end();
private:
    HashItem *findItem(uint64_t key);
    inline HashItem *getItem(int slot);
    inline int nextSlot(uint64_t hashkey, uint64_t slot, int i);

    HashHead *hashhead;
};

#endif

#ifndef CONNECTION__H
#define CONNECTION__H
#include <memory>
#include <vector>

static const int MAX_BUF = 10 * 1024; // 10k

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    Connection(int fd_);
    ~Connection();

    // std::pair<char *, int> getAvaiableBuf();
    // void updateDataSize(int incsize);
    std::pair<char *, int> getData();
    int read();
    void write(char buf[], int buflen);
    void processData();
    int sockfd() {return fd;}

private:
    void processPack(const std::vector<char> &pack);

    int fd;

    char data[MAX_BUF];
    int datalen;
    // std::vector<char> readbuf;
};

#endif

#ifndef COMMANDPROCESSOR__H
#define COMMANDPROCESSOR__H
#include <memory>
class Connection;

void getUserInfoByID(const std::shared_ptr<Connection> &conn, const char *data, int len);
void getUserInfoByAccount(const std::shared_ptr<Connection> &conn, const char *data, int len);

#endif

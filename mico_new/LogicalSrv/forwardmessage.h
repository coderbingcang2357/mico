#ifndef FORWARDMESSAGE_H
#define FORWARDMESSAGE_H

#include <vector>
class ForwardMessage
{
public:
    ForwardMessage();
    bool unpackFrom(const std::vector<char> &d);
private:
};

#endif

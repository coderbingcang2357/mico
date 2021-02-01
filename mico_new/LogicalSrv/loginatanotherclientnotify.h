#ifndef loginatanotherclientnotify_h
#define loginatanotherclientnotify_h
#include <memory>

class IClient;
class InternalMessage;
class LoginAtAnotherClientNotifyGen
{
public:
    static InternalMessage *genNotify(
        const std::shared_ptr<IClient> &user, uint64_t userid);
};

#endif

#ifndef ICOMMAND__H
#define ICOMMAND__H


class MicoServer;
class ICommand
{
public:
    enum CommandType 
    {
        UnUsed,
        //DeviceOffLine,
        ClaimDeviceCheckPasswdTimeout,
        UserOfflne,
        UserTimeout,
        DeviceTimeout,
        ForwardMessage,
        NewMessage
    };

    ICommand(int type = UnUsed) : m_commandType(type) {}
    virtual ~ICommand() {}
    int commandType() {return m_commandType;}
    virtual void execute(MicoServer *server) = 0;

private:
    int m_commandType;
};
#endif

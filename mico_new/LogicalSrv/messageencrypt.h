#ifndef MESSAGE_ENCRYPT_H
#define MESSAGE_ENCRYPT_H

class Message;
class ICache;
int encryptMessage(Message *msg, ICache *cache);
int decryptMsg(Message* msg, ICache *cache);
#endif

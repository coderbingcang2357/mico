#include <stdio.h>
#include <mqueue.h>
#include <errno.h>
#include <string.h>

int main(int argc, char ** argv)
{
    if (argc == 1)
    {
        printf("usage: %s nameofmsgqueue\n", argv[0]);
        return 0;
    }
    int res;
    for (int i = 1; i < argc; i++)
    {
        res = mq_unlink(argv[i]);
        if (res == 0)
            continue;
        // error
        printf("%s : %s\n", argv[i], strerror(errno));
        break;
    }
    return 0;
}

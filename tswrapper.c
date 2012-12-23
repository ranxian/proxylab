#include "csapp.h"

sem_t *ts_hnmutex;

struct hostent *gethostbyname_ts(const char *info, struct hostent *res)
{
    struct hostent *sharedp;

    sem_wait(ts_hnmutex);
    sharedp = gethostbyname(info);
    memcpy(res, sharedp, sizeof(struct hostent));
    sem_post(ts_hnmutex);

    return res;
}
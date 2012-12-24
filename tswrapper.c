#include "csapp.h"
#include "tswrapper.h"
sem_t *ts_hnmutex;

struct hostent *gethostbyname_ts(const char *info, struct hostent *res)
{
    printf("before - %s\n", info);
    struct hostent *sharedp;
    sem_wait(ts_hnmutex);
    sharedp = gethostbyname(info);
    memcpy(res, sharedp, sizeof(struct hostent));
    sem_post(ts_hnmutex);
    printf("end - %s\n", info);

    return res;
}

void tswrapper_init()
{
    sem_unlink("hnmutex");
    ts_hnmutex = sem_open("hnmutex", O_CREAT, DEF_MODE, 1);
}
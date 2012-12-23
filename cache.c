#include "cache.h"
#include <stdio.h>

int readcnt;
sem_t *mutexp, *w;
pthread_rwlock_t rwlock;

void cache_init() 
{
    readcnt = 0;

    pthread_rwlock_init(&rwlock, NULL);
    sem_open("mutexp", O_CREAT, DEF_MODE, 1);
    sem_open("w", O_CREAT, DEF_MODE, 1);

    cache.ca_head = cache.ca_tail = NULL;
    cache.ca_size = 0;
}

void cache_deinit()
{
    pthread_rwlock_destroy(&rwlock);
    CE *ptr = cache.ca_head;
    CE *next = ptr->next_entry;
    while (ptr) {
        free(ptr);
        ptr = next;
        next = ptr->next_entry;
    }
}

CE *is_in_cache(char *request) {
    sem_wait(mutexp);
    readcnt++;
    if (readcnt == 1)
        sem_wait(w);
    sem_post(mutexp);
    CE *res = cache.ca_head;
    int key = crc32(request);

    while (res) {
        if (key == res->key) {
            printf("Cache hit! %s\n", request);
            if (res->next_entry != NULL) {
                if (res->prev_entry != NULL) {
                    res->next_entry->prev_entry = res->prev_entry;
                    res->prev_entry->next_entry = res->next_entry;
                    res->next_entry = NULL;
                    res->prev_entry = cache.ca_tail;
                    cache.ca_tail->next_entry = res;
                    cache.ca_tail = res;
                }
            }
            return res;
        }
        res = res->next_entry;
    }

    sem_wait(mutexp);
    readcnt--;
    if (readcnt == 0)
        sem_post(w);
    sem_post(mutexp);
    return res;
}
void remove_cache_entry(CE *entry) {

    int size = entry->content_size;
    printf("free cache entry of size %d\n", size);
    Free(entry->content);

    if (entry == cache.ca_head) {
        cache.ca_head = entry->next_entry;
        Free(entry);
    } else if (entry == cache.ca_tail) {
        Free(entry);
    } else {
        entry->prev_entry->next_entry = entry->next_entry;
        Free(entry);
    }
    cache.ca_size -= size;
}


CE *add_cache_entry(char *request, char *content, int len) {
    printf("add object of size %d -- cache size: %ld\n", len, cache.ca_size);
    fflush(stdout);
    CE *res;
    // Rio_writen(1, content, len);
    sem_wait(w);
    while (cache.ca_size + len > MAX_CACHE_SIZE) {
        printf("1\n");
        remove_cache_entry(cache.ca_head);
    }
    cache.ca_size += len;

    if (cache.ca_head == NULL) {
        cache.ca_head = Malloc(sizeof(CE));
        cache.ca_tail = cache.ca_head;
        res = cache.ca_head;

        res->content = Malloc(len);
        memcpy(res->content, content, len);
        res->content_size = len;
        res->next_entry = NULL;
        res->prev_entry = NULL;
        res->key = crc32(request);
    } else {
        cache.ca_tail->next_entry = Malloc(sizeof(CE));
        res = cache.ca_tail->next_entry;

        res->content = Malloc(len);
        memcpy(res->content, content, len);
        res->content_size = len;
        res->next_entry = NULL;
        res->prev_entry = NULL;
        res->key = crc32(request);
        cache.ca_tail = res;
    }
    sem_post(w);
    return res;
}

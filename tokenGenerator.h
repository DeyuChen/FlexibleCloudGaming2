#ifndef TOKENBUCKET_H
#define TOKENBUCKET_H

#include "pool.h"
#include <pthread.h>

class TokenGenerator {
public:
    TokenGenerator(Queue<bool> &tokenBucket, int fps);

    ~TokenGenerator();

private:
    void init_token_gen(){ pthread_create(&thread, NULL, token_gen_entry, this); }
    static void* token_gen_entry(void* This){ ((TokenGenerator*)This)->token_gen(); }
    void token_gen();

    int ms;
    Queue<bool> &tokenBucket;
    pthread_t thread;
};

#endif

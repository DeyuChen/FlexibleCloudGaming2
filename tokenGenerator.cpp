#include "tokenGenerator.h"
#include <chrono>
#include <thread>

using namespace std;

TokenGenerator::TokenGenerator(Queue<bool> &tokenBucket, int fps)
    : tokenBucket(tokenBucket), ms(1000 / 30)
{
    init_token_gen();
}

TokenGenerator::~TokenGenerator(){
    if (thread){
        pthread_cancel(thread);
        pthread_join(thread, NULL);
    }
}

void TokenGenerator::token_gen(){
    while (true){
        this_thread::sleep_for(chrono::milliseconds(ms));
        tokenBucket.non_blocking_put(true);
    }
}

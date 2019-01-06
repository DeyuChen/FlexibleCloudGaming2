#ifndef POOL_H
#define POOL_H

#include <set>
#include <pthread.h>

template<class T>
class Pool {
public:
    Pool(int capacity = 0);
    void set_capacity(int v){ capacity = v; }
    T get();
    void put(T t);
private:
    std::set<T> available;
    int capacity;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty, not_full;
};

template<class T>
Pool<T>::Pool(int capacity) : capacity(capacity){
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&not_empty, NULL);
    pthread_cond_init(&not_full, NULL);
}

template<class T>
T Pool<T>::get(){
    pthread_mutex_lock(&mutex);
    while (available.empty()){
        pthread_cond_wait(&not_empty, &mutex);
    }
    T t = *available.begin();
    available.erase(t);
    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&mutex);
    return t;
}

template<class T>
void Pool<T>::put(T t){
    pthread_mutex_lock(&mutex);
    while (available.size() >= capacity){
        pthread_cond_wait(&not_full, &mutex);
    }
    available.insert(t);
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&mutex);
}

#endif

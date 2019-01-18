#ifndef POOL_H
#define POOL_H

#include <set>
#include <queue>
#include <deque>
#include <pthread.h>

template<class T>
class Pool {
public:
    Pool(int capacity = 0);
    void set_capacity(int v){ capacity = v; }
    T get();
    bool non_blocking_get(T &t);
    void put(const T &t);
    void non_blocking_remove(const T &t);

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
bool Pool<T>::non_blocking_get(T &t){
    pthread_mutex_lock(&mutex);
    if (available.empty()){
        pthread_mutex_unlock(&mutex);
        return false;
    }
    t = *available.begin();
    available.erase(t);
    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&mutex);
    return true;
}

template<class T>
void Pool<T>::put(const T &t){
    pthread_mutex_lock(&mutex);
    while (available.size() >= capacity){
        pthread_cond_wait(&not_full, &mutex);
    }
    available.insert(t);
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&mutex);
}

template<class T>
void Pool<T>::non_blocking_remove(const T &t){
    pthread_mutex_lock(&mutex);
    available.erase(t);
    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&mutex);
}

template<class T>
class Queue {
public:
    Queue(int capacity = 0);
    void set_capacity(int v){ capacity = v; }
    T get();
    bool non_blocking_get(T &t);
    void put(const T &t);
    void non_blocking_put(const T &t);
    T peek_back();
    T peek_front();
    bool is_full();

private:
    std::queue<T> available;
    int capacity;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty, not_full;
};

template<class T>
Queue<T>::Queue(int capacity) : capacity(capacity){
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&not_empty, NULL);
    pthread_cond_init(&not_full, NULL);
}

template<class T>
T Queue<T>::get(){
    pthread_mutex_lock(&mutex);
    while (available.empty()){
        pthread_cond_wait(&not_empty, &mutex);
    }
    T t = available.front();
    available.pop();
    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&mutex);
    return t;
}

template<class T>
bool Queue<T>::non_blocking_get(T &t){
    pthread_mutex_lock(&mutex);
    if (available.empty()){
        pthread_mutex_unlock(&mutex);
        return false;
    }
    t = available.front();
    available.pop();
    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&mutex);
    return true;
}

template<class T>
void Queue<T>::put(const T &t){
    pthread_mutex_lock(&mutex);
    while (available.size() >= capacity){
        pthread_cond_wait(&not_full, &mutex);
    }
    available.push(t);
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&mutex);
}

// drop the oldest item in the queue if full
template<class T>
void Queue<T>::non_blocking_put(const T &t){
    pthread_mutex_lock(&mutex);
    while (available.size() >= capacity){
        available.pop();
    }
    available.push(t);
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&mutex);
}

template<class T>
T Queue<T>::peek_back(){
    pthread_mutex_lock(&mutex);
    while (available.empty()){
        pthread_cond_wait(&not_empty, &mutex);
    }
    T t = available.back();
    pthread_mutex_unlock(&mutex);
    return t;
}

template<class T>
T Queue<T>::peek_front(){
    pthread_mutex_lock(&mutex);
    while (available.empty()){
        pthread_cond_wait(&not_empty, &mutex);
    }
    T t = available.front();
    pthread_mutex_unlock(&mutex);
    return t;
}

template<class T>
bool Queue<T>::is_full(){
    pthread_mutex_lock(&mutex);
    bool full = available.size() >= capacity;
    pthread_mutex_unlock(&mutex);
    return full;
}

#endif

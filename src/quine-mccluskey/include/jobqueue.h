#ifndef __wqueue_h__
#define __wqueue_h__
#include <pthread.h>
#include <list>
using namespace std;

template <typename T> class jobqueue {
    public:
        jobqueue() {
            pthread_mutex_init(&mutex, NULL);
        };
        ~jobqueue() {
            pthread_mutex_destroy(&mutex);
        };
        void add(T item) {
            pthread_mutex_lock(&mutex);
            q.push_back(item);
            pthread_mutex_unlock(&mutex);
        };
        T get() {
            pthread_mutex_lock(&mutex);
            T item = -1;
            if(!q.empty()){
                item = q.front();
                q.pop_front();
            }
            pthread_mutex_unlock(&mutex);
            return item;
        };
        int size() {
            pthread_mutex_lock(&mutex);
            int size = q.size();
            pthread_mutex_unlock(&mutex);
            return size;
        };

    private:
        list<T> q;
        pthread_mutex_t mutex;
};
#endif

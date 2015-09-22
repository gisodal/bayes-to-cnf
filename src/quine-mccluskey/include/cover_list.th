#include "cover_list.h"

template <typename T>
void cover_list<T>::init(const unsigned int value){
    if(value == 1)
        memset(covers, ~(T)0, sizeof(T)*SIZE*COVER_SIZE);
    else if(value == 0)
        memset(covers, (T)0, sizeof(T)*SIZE*COVER_SIZE);
    else
        memset(covers, (T) value, sizeof(T)*SIZE*COVER_SIZE);
}

template <typename T>
cover_list<T>& cover_list<T>::cast(void *v){
    return *((cover_list<T>*)v);
}

template <typename T>
void cover_list<T>::set_size(unsigned int size){
    SIZE = size;
}

template <typename T>
void cover_list<T>::set_cover_size(unsigned int size){
    COVER_SIZE = size;
}

template <typename T>
unsigned int cover_list<T>::size(){
    return SIZE;
}

template <typename T>
size_t cover_list<T>::bytes(unsigned int models, unsigned int n){
    return sizeof(cover_list<T>) + cover<T,0>::cover_size(models)*sizeof(cover_element<T>)*n;
}

template <typename T>
unsigned int cover_list<T>::cover_size(){
    return COVER_SIZE;
}

template <typename T>
cover<T,0>& cover_list<T>::operator[](unsigned int i){
    return *(cover<T,0>*) (covers + COVER_SIZE*i);
}

template <typename T>
void* cover_list<T>::end(){
    return &(covers[COVER_SIZE*SIZE]);
}

template <typename T>
void* cover_list<T>::begin(){
    return covers;
}



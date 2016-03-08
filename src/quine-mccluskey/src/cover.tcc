#ifndef COVER_TH
#define COVER_TH

#include "cover.h"
#include "bit.h"
#include <string.h>

#ifndef UNUSED
    #define UNUSED __attribute__((unused))
#endif

inline unsigned int div_round_up(unsigned int x, unsigned int y){
    //return (x + y - 1) / y; // overflow on x = 0
    return 1 + ((x - 1) / y);
}

template <typename T, unsigned int N>
cover<T,N>::cover(){
    init(0);
}

template <typename T, unsigned int N>
size_t cover<T,N>::cover_size(UNUSED const unsigned int n){
    return N;
}

template <typename T, unsigned int N>
void cover<T,N>::init(const unsigned int value, UNUSED const unsigned int SIZE){
    memset(elements,value,sizeof(T)*N);
}

template <typename T, unsigned int N>
cover<T,N>& cover<T,N>::cast(void *v){
    return *((cover<T,N>*)v);
}

template <typename T, unsigned int N>
cover_element<T>* cover<T,N>::get_cover_elements() const{
    return ((cover_element<T>*)this);
}

template <typename T, unsigned int N>
T* cover<T,N>::get_elements() const {
    return ((T*)this);
}

template <typename T, unsigned int N>
bool cover<T,N>::test(const unsigned int i) const {
    const unsigned int m = sizeof(T)*8;
    cover_element<T> *elements = get_cover_elements();
    return elements[i/m].test(i%m);
}

template <typename T, unsigned int N>
unsigned int cover<T,N>::count(UNUSED const unsigned int SIZE) const {
    unsigned int bc = 0;
    for(unsigned int i = 0; i < N; i++)
        bc += bitcount(elements[i]);
    return bc;
}

template <typename T, unsigned int N>
bool cover<T,N>::none(UNUSED const unsigned int SIZE) const {
    cover_element<T> *elements = get_cover_elements();
    for(unsigned int i = 0; i < N; i++)
        if(elements[i] != 0)
            return false;
    return true;
}

template <typename T, unsigned int N>
bool cover<T,N>::all(UNUSED const unsigned int SIZE) const {
    for(unsigned int i = 0; i < N; i++)
        if(elements[i] != ~(T)0)
            return false;
    return true;
}

template <typename T, unsigned int N>
T& cover<T,N>::operator[](const unsigned int i){
    return elements[i];
}

template <typename T, unsigned int N>
cover_element<T>& cover<T,N>::operator()(const unsigned int i){
    return ((cover_element<T>*) elements)[i];
}

template <typename T, unsigned int N>
size_t cover<T,N>::size(UNUSED const unsigned int SIZE){
    return sizeof(T)*N*8;
}

template <typename T, unsigned int N>
void cover<T,N>::set(const unsigned int i) {
    const unsigned int m = sizeof(T)*8;
    cover_element<T> *elements = get_cover_elements();
    elements[i/m].set(i%m);
}

template <typename T, unsigned int N>
void cover<T,N>::set_lsb(const unsigned int n) {
    const unsigned int m = sizeof(T)*8;
    cover_element<T> *elements = get_cover_elements();
    for(unsigned int i = 0; i < n/m; i++)
        elements[i] = ~(T)0;

    if(n % m)
        elements[n/m] = ((1 << ((n%m)-1))-1) | (1 << ((n%m)-1));
}

template <typename T, unsigned int N>
void cover<T,N>::set_all(UNUSED const unsigned int SIZE) {
    cover_element<T> *elements = get_cover_elements();
    for(unsigned int i = 0; i < N; i++)
        elements[i] = ~0;
}

template <typename T, unsigned int N>
bool cover<T,N>::equals(cover &c, UNUSED const unsigned int SIZE) const {
    for(unsigned int i = 0; i < N; i++)
        if(elements[i] != c[i])
            return false;
    return true;
}

template <typename T, unsigned int N>
cover<T,N>& cover<T,N>::assign(unsigned int v, UNUSED const unsigned int SIZE){
    T* t = (T*) this;
    size_t size = sizeof(T)*8;
    for(unsigned int i = 0; i < N && i < (sizeof(unsigned int)/sizeof(T)); i++)
        t[i] = (T) (v>>(i*size));
    return *this;
}

template <typename T, unsigned int N>
cover<T,N>& cover<T,N>::assign(cover &c, UNUSED const unsigned int SIZE) {
    for(unsigned int i = 0; i < N; i++)
        elements[i] = c[i];
    return *this;
}

template <typename T, unsigned int N>
cover<T,N>& cover<T,N>::or_assign(cover &c, UNUSED const unsigned int SIZE) {
    for(unsigned int i = 0; i < N; i++)
        elements[i] |= c[i];
    return *this;
}

template <typename T, unsigned int N>
cover<T,N>& cover<T,N>::and_assign(cover &c, UNUSED const unsigned int SIZE) {
    for(unsigned int i = 0; i < N; i++)
        elements[i] &= c[i];
    return *this;
}

template <typename T, unsigned int N>
cover<T,N>& cover<T,N>::xor_assign(cover &c, UNUSED const unsigned int SIZE) {
    for(unsigned int i = 0; i < N; i++)
        elements[i] ^= c[i];
    return *this;
}

// cover of with variable amount of elements
// ==========================================================

template <typename T>
void cover<T,0>::init(const unsigned int value, const unsigned int N){
    memset(elements,value,sizeof(T)*N);
}

template <typename T>
cover<T,0>& cover<T,0>::cast(void *v){
    return *((cover<T,0>*)v);
}

template <typename T>
size_t cover<T,0>::cover_size(unsigned int N){
    return div_round_up(N,sizeof(cover_element<T>)*8);
}

template <typename T>
size_t cover<T,0>::bytes(unsigned int N){
    return sizeof(T)*N;
}

template <typename T>
cover_element<T>* cover<T,0>::get_cover_elements() const{
    return ((cover_element<T>*)this);
}

template <typename T>
T* cover<T,0>::get_elements() const {
    return ((T*)this);
}

template <typename T>
bool cover<T,0>::test(unsigned int i) const {
    const unsigned int m = sizeof(T)*8;
    cover_element<T> *elements = get_cover_elements();
    return elements[i/m].test(i%m);
}

template <typename T>
unsigned int cover<T,0>::count(const unsigned int N) const {
    unsigned int bc = 0;
    cover_element<T> *elements = get_cover_elements();
    for(unsigned int i = 0; i < N; i++)
        bc += bitcount(*((T*)&(elements[i])));
    return bc;
}

template <typename T>
bool cover<T,0>::none(const unsigned int N) const {
    for(unsigned int i = 0; i < N; i++)
        if(elements[i] != 0)
            return false;
    return true;
}

template <typename T>
bool cover<T,0>::all(const unsigned int N) const {
    for(unsigned int i = 0; i < N; i++)
        if(elements[i] != ~(T)0)
            return false;
    return true;
}

template <typename T>
bool cover<T,0>::any(const unsigned int N) const {
    for(unsigned int i = 0; i < N; i++)
        if(elements[i] != 0)
            return true;
    return false;
}


template <typename T>
T& cover<T,0>::operator[](const unsigned int i){
    return elements[i];
}

template <typename T>
cover_element<T>& cover<T,0>::operator()(const unsigned int i){
    return ((cover_element<T>*) elements)[i];
}

template <typename T>
size_t cover<T,0>::size(const unsigned int N){
    return sizeof(T)*N*8;
}

template <typename T>
void cover<T,0>::clear(const unsigned int i) {
    const unsigned int m = sizeof(T)*8;
    cover_element<T> *elements = get_cover_elements();
    elements[i/m].clear(i%m);
}

template <typename T>
void cover<T,0>::set(const unsigned int i) {
    const unsigned int m = sizeof(T)*8;
    cover_element<T> *elements = get_cover_elements();
    elements[i/m].set(i%m);
}

template <typename T>
void cover<T,0>::set_lsb(const unsigned int n) {
    const unsigned int m = sizeof(T)*8;
    for(unsigned int i = 0; i < n/m; i++)
        elements[i] = ~(T)0;

    if(n % m)
        elements[n/m] = ((T)1 << (n%m))-1;
}

template <typename T>
void cover<T,0>::set_all(const unsigned int N) {
    for(unsigned int i = 0; i < N; i++)
        elements[i] = ~0;
}

template <typename T>
bool cover<T,0>::equals(cover &c, const unsigned int N) const {
    for(unsigned int i = 0; i < N; i++)
        if(elements[i] != c[i])
            return false;
    return true;
}

template <typename T>
cover<T,0>& cover<T,0>::assign(unsigned int v, const unsigned int N){
    size_t size = sizeof(T)*8;
    for(unsigned int i = 0; i < N && i < (sizeof(unsigned int)/sizeof(T)); i++)
        elements[i] = (T) (v>>(i*size));
    return *this;
}

template <typename T>
cover<T,0>& cover<T,0>::assign(cover &c, const unsigned int N) {
    for(unsigned int i = 0; i < N; i++)
        elements[i] = c[i];
    return *this;
}

template <typename T>
cover<T,0>& cover<T,0>::or_assign(cover &c, const unsigned int N) {
    for(unsigned int i = 0; i < N; i++)
        elements[i] |= c[i];
    return *this;
}

template <typename T>
cover<T,0>& cover<T,0>::and_assign(cover &c, const unsigned int N) {
    for(unsigned int i = 0; i < N; i++)
        elements[i] &= c[i];
    return *this;
}

template <typename T>
cover<T,0>& cover<T,0>::xor_assign(cover &c, const unsigned int N) {
    for(unsigned int i = 0; i < N; i++)
        elements[i] ^= c[i];
    return *this;
}

template <typename T>
cover<T,0>& cover<T,0>::not_assign(unsigned int N){
    for(unsigned int i = 0; i < N; i++)
        elements[i] = ~elements[i];
    return *this;
}


//cover_list::iterator::iterator& operator++(unsigned int){
//    return (element += size);
//}
//
//cover_list::iterator::iterator& operator--(unsigned int){
//    return (element -= size);
//}
//
//cover_list::iterator& cover_list::iterator::operator=(const void *v, unsigned int s){
//    element = (cover_element<T>*) v;
//    size = s;
//}
//
//cover_list::iterator& cover_list::iterator::operator=(const iterator &it){
//    size = it.size;
//    element = it.element;
//    return *this;
//}
//
//bool cover_list::iterator::operator!=(const iterator &it) const {
//    return element != it.element;
//}
//
//bool cover_list::iterator::operator==(const iterator &it) const {
//    return element == it.element;
//}







#endif

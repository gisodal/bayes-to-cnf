#ifndef COVER_ELEMENT_TH
#define COVER_ELEMENT_TH

#include "bit.h"

template <typename T>
cover_element<T>::cover_element(){
    value = 0;
}

template <typename T>
cover_element<T>::cover_element(unsigned int v){
    value = (T) v;
}

template <typename T>
size_t cover_element<T>::size() const {
    return sizeof(T)*8;
}

template <typename T>
unsigned int cover_element<T>::count() const{
    return bitcount(value);
}

template <typename T>
bool cover_element<T>::test(unsigned int i) const {
    return (value >> i) & (T)1;
}

template <typename T>
bool cover_element<T>::any() const {
    return value;
}

template <typename T>
bool cover_element<T>::none() const {
    return value == (T)0;
}

template <typename T>
bool cover_element<T>::all() const {
    return value == ~(T)0;
}

template <typename T>
void cover_element<T>::clear_all(){
    value = (T)0u;
}

template <typename T>
void cover_element<T>::set_all(){
    value = ~(T)0;
}

template <typename T>
void cover_element<T>::set_lsb(unsigned int i){
    value = ((T)1 << i)-1;
}

template <typename T>
void cover_element<T>::set(unsigned int i){
    value |= (((T)1) <<i);
}

template <typename T>
void cover_element<T>::negate(){
    value = ~value;
}

template <typename T>
void cover_element<T>::clear(unsigned int i){
    value &= ~(((T)1) << i);
}

template <typename T>
bool cover_element<T>::power_of_two_or_zero() const {
    return (value & (value - 1)) == (T)0;
}

template <typename T>
cover_element<T>& cover_element<T>::operator[](unsigned int i){
    return (this)[i];
}

template <typename T>
bool cover_element<T>::operator<(const cover_element<T> &e) const {
    return value < e.value;
}

template <typename T>
bool cover_element<T>::operator==(const cover_element<T> &e) const {
    return value == e.value;
}

template <typename T>
bool cover_element<T>::operator!=(const cover_element<T> &e) const {
    return value == e.value;
}

template <typename T>
cover_element<T>& cover_element<T>::operator=(T v){
    value = v;
    return *this;
}

template <typename T>
cover_element<T>& cover_element<T>::operator=(const cover_element &e){
    value = e.value;
    return *this;
}

template <typename T>
cover_element<T>& cover_element<T>::operator|=(const cover_element &e){
    value |= e.value;
    return *this;
}

template <typename T>
cover_element<T>& cover_element<T>::operator&=(const cover_element &e){
    value &= e.value;
    return *this;
}

template <typename T>
cover_element<T>& cover_element<T>::operator^=(const cover_element &e){
    value ^= e.value;
    return *this;
}

template <typename T>
cover_element<T>& cover_element<T>::operator|(const cover_element &e) const {
    static cover_element<T> tmp;
    tmp.value = value | e.value;
    return tmp;
}

template <typename T>
cover_element<T>& cover_element<T>::operator&(const cover_element &e) const {
    static cover_element<T> tmp;
    tmp.value = value & e.value;
    return tmp;
}

template <typename T>
cover_element<T>& cover_element<T>::operator^(const cover_element &e) const {
    static cover_element<T> tmp;
    tmp.value = value ^ e.value;
    return tmp;
}

#endif

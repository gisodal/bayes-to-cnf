#ifndef BIT_H
#define BIT_H

#include <stdlib.h>
#include <stdint.h>

template <typename T>
inline void set_bit(T &x, unsigned i){
    x |= ((T)1<<i);
}

template <typename T>
inline void clear_bit(T &x, unsigned i){
    x &= ~((T)1<<i);
}

template <typename T>
inline void toggle_bit(T &x, unsigned int i){
    x ^= ((T)1 << i);
}

template <typename T>
inline bool is_set_bit(T x, unsigned int i){
    return (x >> i) & 1;
}

template <typename T>
inline void set_min_value(T &x, T y){
    x = y ^ ((x ^ y) & -(x < y));
}

template <typename T>
inline unsigned int bitcount(T x){
    uint16_t count = 0;
    while(x){
        count += x & 1;
        x >>= 1;
    }
    return count;
}

//template <>
//inline uint16_t bitcount(uint32_t x){
//    x = x - ((x >> 1) & 0x55555555);
//    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
//    return (((x + (x >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
//}

template <>
inline unsigned int bitcount(uint32_t x){ // assume uint32_t == unsigned int
    return __builtin_popcount(x);
}

template <>
inline unsigned int bitcount(uint64_t x){
    return __builtin_popcountl(x);;
}

//template <>
//inline unsigned int bitcount(uint128_t x){
//    return bitcount((uint64_t) x) + bitcount((uint64_t) (x>>64));
//}

template <typename T>
inline bool is_power_of_two_or_zero(const T x){
    return (x & (x - 1)) == (T) 0;
}

#endif

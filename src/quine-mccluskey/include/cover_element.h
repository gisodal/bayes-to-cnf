#ifndef COVER_ELEMENT_H
#define COVER_ELEMENT_H

#include <stdlib.h>
#include <stdint.h>

template <typename T>
class cover_element {
    public:
        cover_element();
        cover_element(unsigned int);
        size_t size() const;
        unsigned int count() const;
        bool test(unsigned int) const;
        bool any() const;
        bool none() const;
        bool all() const;
        void clear_all();
        void set_all();
        void set_lsb(unsigned int);
        void set(unsigned int);
        void negate();
        void clear(unsigned int);
        bool power_of_two_or_zero() const;
        bool operator==(const cover_element&) const;
        bool operator!=(const cover_element&) const;
        bool operator<(const cover_element&) const;
        cover_element& operator[](unsigned int i);
        cover_element& operator=(T);
        cover_element& operator=(const cover_element&);
        cover_element& operator|=(const cover_element&);
        cover_element& operator&=(const cover_element&);
        cover_element& operator^=(const cover_element&);
        cover_element& operator|(const cover_element&) const;
        cover_element& operator&(const cover_element&) const;
        cover_element& operator^(const cover_element&) const;
        T value;
};

#include "cover_element.th"

typedef uint32_t element_t;
typedef cover_element<element_t> cover_element_t;

#endif

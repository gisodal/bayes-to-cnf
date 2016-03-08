#ifndef COVER_H
#define COVER_H

#include <stdlib.h>
#include <stdint.h>

#include "cover_element.h"

template <typename T, unsigned int N>
class cover {
    public:
        cover();
        static size_t cover_size(const unsigned int);
        void init(const unsigned int, const unsigned int SIZE = N);
        static cover& cast(void*);
        bool test(const unsigned int) const;
        unsigned int count(const unsigned int SIZE = N) const;
        bool none(const unsigned int SIZE = N) const;
        bool all(const unsigned int SIZE = N) const;
        size_t size(const unsigned int SIZE = N);
        void set(const unsigned int);
        void set_lsb(const unsigned int i);
        void set_all(const unsigned int SIZE = N);
        cover_element<T>* get_cover_elements() const;
        T* get_elements() const;

        T& operator[](unsigned int);
        cover_element<T>& operator()(const unsigned int);

        bool equals(cover&,const unsigned int SIZE = N) const;
        cover& assign(unsigned int,const unsigned int SIZE = N);
        cover& assign(cover&,const unsigned int SIZE = N);
        cover& or_assign(cover&,const unsigned int SIZE = N);
        cover& and_assign(cover&,const unsigned int SIZE = N);
        cover& xor_assign(cover&,const unsigned int SIZE = N);

        //bool operator==(cover&) const;
        //bool operator!=(cover&) const;
        //cover& operator=(unsigned int);
        //cover& operator=(cover&);
        //cover& operator|=(cover&);
        //cover& operator&=(cover&);
        //cover& operator^=(cover&);
        //cover& operator|(cover&) const;
        //cover& operator&(cover&) const;
        //cover& operator^(cover&) const;

    private:
        T elements[N];
};

template <typename T>
class cover<T,0> {
    public:
        void init(const unsigned int, const unsigned int);
        static cover& cast(void*);
        static size_t cover_size(const unsigned int);
        static size_t bytes(const unsigned int);
        bool test(unsigned int) const;
        unsigned int count(const unsigned int) const;
        bool none(const unsigned int) const;
        bool any(const unsigned int) const;
        bool all(const unsigned int) const;
        size_t size(const unsigned int);
        void clear(const unsigned int);
        void set(const unsigned int);
        void set_lsb(const unsigned int i);
        void set_all(const unsigned int);
        cover_element<T>* get_cover_elements() const;
        T* get_elements() const;

        T& operator[](const unsigned int);
        cover_element<T>& operator()(const unsigned int);

        bool equals(cover&, const unsigned int) const;
        cover& not_assign(const unsigned int);
        cover& assign(unsigned int, const unsigned int);
        cover& assign(cover&, const unsigned int);
        cover& or_assign(cover&, const unsigned int);
        cover& and_assign(cover&, const unsigned int);
        cover& xor_assign(cover&, const unsigned int);

    private:
        T elements[];
};

#include "../src/cover.tcc"

typedef cover<uint32_t,0> cover_t;

#include "cover_list.h"

#endif

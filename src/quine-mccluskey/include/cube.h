#ifndef CUBE_H
#define CUBE_H

#include "cover_element.h"
#include <tuple>

template<typename T>
using base_cube = std::tuple< cover_element<T>, cover_element<T> >;

template <typename T>
class cube : public base_cube<T> {
    public:
        inline cover_element<T>& operator[](const int i) {
            return (i?std::get<1>(*this):std::get<0>(*this));
        };
        static cube<T>* cast(void *v){
            return (cube<T>*) v;
        };
};

#endif


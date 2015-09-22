#ifndef QM_H
#define QM_H

#if __LP64__

#ifndef _INT128_T
#define _INT128_T
typedef __int128_t  int128_t;
#endif

#ifndef _UINT128_T
#define _UINT128_T
typedef __uint128_t uint128_t;
#endif

#ifndef INT128_MIN
#define INT128_MIN  ((__int128_t)0 - ((__int128_t)1 << 126) - ((__int128_t)1 << 126))
#endif

#ifndef INT128_MAX
#define INT128_MAX  ((__int128_t)-1 + ((__int128_t)1 << 126) + ((__int128_t)1 << 126))
#endif

#ifndef UINT128_MAX
#define UINT128_MAX (((__uint128_t)1 << 127) - (__uint128_t)1 + ((__uint128_t)1 << 127))
#endif

// #else
// #include <boost/multiprecision/cpp_int.hpp>
// using boost::multiprecision::uint128_t;
//
// #ifndef INT128_MIN
// #define INT128_MIN  ((uint128_t)0 - ((uint128_t)1 << 126) - ((__int128_t)1 << 126))
// #endif
//
// #ifndef INT128_MAX
// #define INT128_MAX  ((__int128_t)-1 + ((__int128_t)1 << 126) + ((__int128_t)1 << 126))
// #endif
//
// #ifndef UINT128_MAX
// #define UINT128_MAX (((uint128_t)1 << 127) - (uint128_t)1 + ((uint128_t)1 << 127))
// #endif

#endif

#include "cube.h"
#include "cover_element.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <algorithm>
#include <unordered_set>
#include <set>
#include <vector>

template <typename M>
class qm {
    public:
        qm();
        ~qm();

        inline void add_variable(uint32_t, int index = -1);
        inline void add_model(cube<M>);
        inline void add_model(M);
        void clear();
        int solve();

        int canonical_primes();
        size_t required_size();
        void print(bool characters = false);

        template <typename T> int compute_primes(std::vector< cube<T> >&);
        template <typename T> int quine_mccluskey(std::vector< cube<T> >&);
        template <typename P, typename T> int reduce(std::vector< cube<P> >&);
        template <typename T> inline unsigned int get_weight(cube<T>&, const T&) const;
        template <typename P> void print_cubes(std::vector< cube<P> >&);
        template <typename P> void cpy_primes(std::vector< cube<P> >&);
        void get_clause(std::vector<int32_t>&, unsigned int);
        unsigned int get_primes_size();
        void remove_prime(cube<M>&);

        bool reduced();
        int unate_cover();
    private:
        size_t cube_size;
        std::vector<unsigned int> variables;         // variables with least significant bit first (variables[0])
        std::set< M > models;
        std::vector< cube<M> > primes;
};

template <typename M>
inline void qm<M>::add_variable(uint32_t v, int index){
    if(index == -1)
        variables.push_back(v);
    else {
        if(variables.size() <= (unsigned int) index)
            variables.resize(index+1);
        variables[index] = v;
    }
}

template <typename M>
inline void qm<M>::add_model(M model){
    models.insert(model);
}

template <typename M>
inline void qm<M>::add_model(cube<M> model){
    if(model[1].any()){
        cover_element<M> m;
        m = model[0];

        while(true){
            models.insert(m.value);

            bool changed = false;
            for(unsigned int i = 0; i < variables.size(); i++){
                if(!m.test(i) && model[1].test(i)){
                    cover_element<M> t;
                    t.set_lsb(i);
                    t.negate();
                    t |= model[0];
                    m &= t;
                    m.set(i);

                    changed = true;
                    break;
                }
            }
            if(!changed)
                break;
        }

    } else {
        models.insert(model[0].value);
    }
}

#endif


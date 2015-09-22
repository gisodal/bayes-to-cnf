#ifndef TYPES_H
#define TYPES_H

//#define max(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
//#define min(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })
//#define bound(a,b,c) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); __typeof__ (c) _c = (c); (_a < _b)||(_a > _c)? (_a < _b?_b:_c) : _a; })
//#define abs(a) ({ __typeof__ (a) _a = (a); _a < 0 ? -_a : _a; })
#include <stdint.h>

typedef int SOCKET;
typedef unsigned int PORT;
typedef int FILE_DESCRIPTOR;
typedef char HASH [33];
typedef char NAME [64];
typedef char IP [16];
typedef uint32_t ID;
typedef uint32_t SIZE;
typedef char* BITSTREAM;
typedef char BIT;

#ifndef UN
    #define UN __attribute__((unused))
#endif

#endif


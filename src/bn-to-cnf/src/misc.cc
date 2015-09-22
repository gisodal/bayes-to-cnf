#include "misc.h"
#include <libgen.h>
#include <string.h>

const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

const char *get_basename(const char *filename) {
    char *path = strdup(filename);
    const char *bname = basename(path);
    return bname;
}

void remove_ext(const char *filename){
    const char *dot = strrchr(filename, '.');
    char *s = (char*) dot;
    if(s != NULL)
        s[0] = '\0';
}


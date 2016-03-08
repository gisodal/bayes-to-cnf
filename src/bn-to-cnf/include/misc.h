#ifndef MISC_H
#define MISC_H

#include <string.h>

const char *get_filename_ext(const char *filename);
char *get_basename(char *filename);
void remove_ext(const char *filename);

#endif


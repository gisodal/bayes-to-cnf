#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>

#define THROW_BUFFER_SIZE 512
class throw_string_error {
    char err[THROW_BUFFER_SIZE];
    public:
        throw_string_error(const char *format, ...){
            va_list args;
            va_start (args, format);
            vsnprintf(err, THROW_BUFFER_SIZE-1, format, args);
            va_end(args);
            err[THROW_BUFFER_SIZE-1] = '\0';
        };
        char* what() { return err; };

};

#endif

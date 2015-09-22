#ifndef READER_H
#define READER_H

#include <string>
#include <vector>
#include <stack>
#include <stdarg.h>
#include "error.h"

typedef throw_string_error parse_error;
typedef throw_string_error reader_error;

enum delimiter_type {
    no_delimiter            = 0,
    ignore_delimiter        = 1,
    standard_delimiter      = 2,
    eof_delimiter           = 3,

    // scoped delimiters
    scope_delimiter         = 4,
    string_scope_delimiter  = 5,
    comment_delimiter       = 6
};

struct delimiter_element {
    delimiter_type type;
    char c;
    char ce;
};

typedef delimiter_element delimiter_t;

class reader {
    public:
        reader();
        ~reader();

        bool open();
        void close();

        void add_delimiter(delimiter_type, char, char ce = '\0');
        void add_delimiters(delimiter_type, unsigned int, ...);
        bool scoped_delimiter(delimiter_type);
        delimiter_t create_delimiter(delimiter_type, char, char ce = '\0');

        bool is_delimiter(std::string);
        delimiter_type is_delimiter(char);
        delimiter_t get_delimiter(char);
        void print_delimiters();
        std::string last_word();
        std::string get_word();
        std::string get_word_peek();
        int get_row();
        int get_col();
        void get_word_equal_or_assert(std::string);
        void get_word_not_equal_or_assert(std::string);
        bool get_word_peek_equal(std::string);
        bool get_word_peek_not_equal(std::string);

        char get_char();
        void unget_char(char);
        int get_scope_depth();
        delimiter_t get_scope_delimiter();
        void set_filename(std::string);
        void set_fd(FILE*);
        std::string get_filename();
        void allow_eof(bool);
        static std::vector<std::string> string_to_words(std::string);
    private:
        bool eof_allowed;
        std::string filename;
        FILE *file;
        int row;
        int col;
        std::vector<delimiter_t> delimiters;
        std::stack<delimiter_t> scope;
        bool hierarchical;
        bool commented;
        std::string bufferword;
};

#endif

#include "reader.h"
#include <algorithm>
#include <stdio.h>

using namespace std;

vector<string> reader::string_to_words(string text){
    vector<string> words;
    FILE * file;
    file = tmpfile();
    fputs (text.c_str(), file);
    rewind(file);

    reader input;
    input.add_delimiters(ignore_delimiter, 3, ' ', '\t', '\n', '\r');
    input.set_fd(file);
    while(input.get_word_peek() != input.last_word())
        words.push_back(input.get_word());

    return words;
}


reader::reader(){
    file = NULL;
    row = 1;
    col = 0;
    hierarchical = true;
    commented = false;
    eof_allowed = true;
}

reader::~reader(){
    close();
}

void reader::set_fd(FILE *f){
    file = f;
}

bool reader::open(){
    if(!file && !filename.empty()){
        file = fopen(filename.c_str(), "r");
        if(file){
            row = 1;
            col = 0;
            hierarchical = true;
            commented = false;
            eof_allowed = true;
            while(!scope.empty())
                scope.pop();
            return true;
        }
    }
    return false;
}

void reader::close(){
    if(file){
        fclose(file);
        file = NULL;
    }
}

void reader::allow_eof(bool e){
    eof_allowed = e;
}

string reader::last_word(){
    return string("END_OF_FILE");
}

void reader::unget_char(char c){
    if(c == '\n'){
        row--;
        col = 80; // NOTE: total guess :)
    } else col--;

    ungetc(c, file);
}

void reader::set_filename(string f){
    filename = f;
}

string reader::get_filename(){
    return filename;
}

char reader::get_char(){
    char c = fgetc(file);
    if(c == '\n'){
        row++;
        col = 0;
    } else col++;

    if(c == EOF)
        close();

    return c;
}

int reader::get_scope_depth(){
    return scope.size();
}

delimiter_t reader::get_scope_delimiter(){
    if(scope.empty())
        return create_delimiter(no_delimiter,'\0');
    else
        return scope.top();
}

void reader::get_word_not_equal_or_assert(string c){
    string w = get_word();
    if(w == c)
        throw parse_error("error on %d:%d: expecting '%s', but got '%s'\n", row, col, c.c_str(), w.c_str());
}

void reader::get_word_equal_or_assert(string c){
    string w = get_word();
    if(w != c)
        throw parse_error("error on %d:%d: expecting '%s', but got '%s'\n", row, col, c.c_str(), w.c_str());
}

bool reader::get_word_peek_not_equal(string c){
    return !get_word_peek_equal(c);
}

bool reader::get_word_peek_equal(string c){
    string w = get_word_peek();
    return (w == c);
}

delimiter_t reader::create_delimiter(delimiter_type dt, char c, char ce){
    delimiter_t d;
    d.type = dt;
    d.c = c;
    d.ce = ce;
    return d;
}

void reader::add_delimiter(delimiter_type dt, char c, char ce){
    bool scoped = scoped_delimiter(dt);
    if(scoped && ce == '\0')
        throw reader_error("scoped delimiters must have ending character");
    else if(!scoped && ce != '\0')
        throw reader_error("ending character provided for non-scoped delimiter");
    else
        delimiters.push_back(create_delimiter(dt,c,ce));
}

bool reader::scoped_delimiter(delimiter_type dt){
    return (int) dt >= (int) scope_delimiter;
}

void reader::add_delimiters(delimiter_type dt, unsigned int argc, ...){
    va_list args;
    va_start (args, argc);

    if(dt == no_delimiter)
        throw reader_error("Cannot not add delimiter of no type");
    else if(scoped_delimiter(dt)){
        for(unsigned int i = 0; i < argc; i++){
            char lsd = (char) va_arg(args, int);
            char rsd = (char) va_arg(args, int);
            add_delimiter(dt, lsd, rsd);
        }
    } else {
        for(unsigned int i = 0; i < argc; i++)
            add_delimiter(dt, (char) va_arg(args, int));
    }

    va_end(args);
}

delimiter_t reader::get_delimiter(char c){
    if(c == EOF)
        return create_delimiter(eof_delimiter, c);
    else { // NOTE: // commented and hierarchical are mutually exclusive
        if(commented){
            if(!scope.empty() && scope.top().ce == c)
                return scope.top();
            else return create_delimiter(ignore_delimiter, c);
        } else if(hierarchical){
            delimiter_t *d = NULL;
            for(unsigned int i = 0; i < delimiters.size(); i++){
                if(delimiters[i].c == c)
                    return delimiters[i];
                else if (scoped_delimiter(delimiters[i].type) && delimiters[i].ce == c){
                    if(delimiters[i].type != comment_delimiter)
                        return delimiters[i];
                    else d = &(delimiters[i]);
                }
                if(d != NULL)
                    return *d;
            }
        } else if(!scope.empty()){
            delimiter_t d = scope.top();
            if(c == d.ce)
                return d;
        }
        return create_delimiter(no_delimiter, c);
    }
}

bool reader::is_delimiter(string w){
    if(w.size() > 1 || w.size() == 0)
        return false;
    else {
        for(unsigned int i = 0; i < delimiters.size(); i++){
            if(delimiters[i].type != comment_delimiter){
                if(w[0] == delimiters[i].c)
                    return true;
                else if (scoped_delimiter(delimiters[i].type)
                    && w[0] == delimiters[i].ce)
                    return true;
            }
        }
    }
    return false;
}

delimiter_type reader::is_delimiter(char c){
    return get_delimiter(c).type;
}

void reader::print_delimiters(){
    printf("\nStandard delimiters (%lu): ", delimiters.size());
    for(unsigned int i = 0; i < delimiters.size(); i++)
        if(delimiters[i].type == standard_delimiter)
            printf("'%c' ", delimiters[i].c);

    printf("\n  Ignore delimiters (%lu): ", delimiters.size());
    for(unsigned int i = 0; i < delimiters.size(); i++)
        if(delimiters[i].type == ignore_delimiter)
            delimiters[i].c=='\n'?printf("'\\n' "):printf("'%c' ", delimiters[i].c);

    printf("\n   scope delimiters (%lu): ", delimiters.size());
    for(unsigned int i = 0; i < delimiters.size(); i++)
        if(scoped_delimiter(delimiters[i].type))
            printf("'%c%c' ", delimiters[i].c, delimiters[i].ce);

    printf("\n");
}

int reader::get_row(){
    return row;
}

int reader::get_col(){
    return col;
}

string reader::get_word_peek(){
    if(bufferword.empty())
        bufferword = get_word();

    return bufferword;
}

string reader::get_word(){
    string word;
    if(!bufferword.empty()){
        word = bufferword;
        bufferword.clear();
    } else {

        if(file){
            char c;
            delimiter_t d;
            do {
                    c = get_char();
                d = get_delimiter(c);
                if(d.type == standard_delimiter){
                    if(word.empty())
                        word += c;
                    else unget_char(c);
                } else if (d.type == ignore_delimiter) {
                    if(word.empty()){
                        d.type = no_delimiter;
                        continue;
                    } else if(!hierarchical){
                        word += c;
                        d.type = no_delimiter;
                        continue;
                    }
                } else if(d.type == string_scope_delimiter) {
                    if(word.empty()){
                        word += c;
                        if(hierarchical){
                            scope.push(d);
                            hierarchical = false;
                        } else { // stack cannot be empty and top element must be scope_delimiter
                            if(scope.top().ce == c){
                                scope.pop();
                                hierarchical = true;
                            } else {
                                d.type = no_delimiter;
                                continue;
                            }
                        }
                    } else unget_char(c);
                } else if(d.type == comment_delimiter){
                    if(word.empty()){
                        if(commented){
                            if(get_scope_delimiter().ce == c){
                                scope.pop();
                                commented = false;
                            } else throw reader_error("Error at %d:%d: cannot end comment with '%c' instead of '%c'\n", row, col, c, scope.top().ce);
                        } else {
                            if(d.c == c){
                                scope.push(d);
                                commented = true;
                            } else throw reader_error("Error at %d:%d: found comment ender '%c' whitout comment to end\n", row, col, c);
                        }
                        d.type = no_delimiter;
                    } else unget_char(c);

                } else if(d.type == scope_delimiter) {
                    if(hierarchical){
                        if(word.empty()){
                            word += c;
                            if(c == d.c)
                                scope.push(d);
                            else {
                                if(!scope.empty()){
                                    if(get_scope_delimiter().ce == c)
                                        scope.pop();
                                    else throw reader_error("Error at %d:%d: found scope ender '%c', while exprecting '%c'", row, col, c, get_scope_delimiter().ce);
                                } else throw reader_error("Error at %d:%d: found scope ender '%c', while in scope depth %d", row, col, c, get_scope_depth());
                            }
                        } else unget_char(c);
                    } else {
                        word += c;
                        d.type = no_delimiter;
                        continue;
                    }
                } else if(d.type == eof_delimiter) {
                    if(word.empty())
                        word = last_word();

                } else word += c;

            } while(d.type == no_delimiter);

        } else {
            if(!scope.empty())
                throw reader_error("Error at end of file: unmatched '%c', scope still at depth %d\n", scope.top().c, scope.size());
            if(!eof_allowed)
                throw reader_error("Error at end of file: EOF encountered while processing\n", scope.top().c, scope.size());

            word = last_word();
        }
    }

    return word;
}


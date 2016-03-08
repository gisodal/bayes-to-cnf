#include "hugin.h"
#include <stdarg.h>

using namespace std;
using namespace HUGIN;

hugin::hugin(){
    input.add_delimiters(standard_delimiter, 3, '=', ';', ',');
    input.add_delimiters(ignore_delimiter, 3, ' ', '\t', '\n', '\r');
    input.add_delimiters(string_scope_delimiter, 1, '"', '"');
    input.add_delimiters(scope_delimiter, 2, '{', '}', '(', ')');
    input.add_delimiters(comment_delimiter, 1, '%', '\n');
}

bool hugin::process(string f){
    try {
        filename = f;
        input.set_filename(f);
        if(input.open()){
            try {
                definition.parse(input);
            } catch (parse_error& e) {
                fprintf(stderr, "an error occurred while reading: %s\n", e.what());
                return false;
            }

            input.close();
            return true;
        } else fprintf(stderr, "could not open file %s\n", input.get_filename().c_str());
    } catch(reader_error& e) {
        fprintf(stderr, "an error occurred while reading: %s\n", e.what());
    }

    return false;
}

void domain_definition::parse(reader &input){
    input.allow_eof(false);
    header.parse(input);
    input.allow_eof(true);

    while(input.get_word_peek() != input.last_word()){
        input.allow_eof(false);
        domain_element element;
        element.parse(input);
        elements.push_back(element);
        input.allow_eof(true);
    }
}

void domain_element::parse(reader &input){
    if(input.get_word_peek_equal("node")){
        type = node_domain_element;
        input.get_word_equal_or_assert("node");
        name = input.get_word();
    } else if (input.get_word_peek_equal("potential")){
        type = potential_domain_element;
        input.get_word_equal_or_assert("potential");
        input.get_word_equal_or_assert("(");
        while(input.get_word_peek_not_equal(")") && input.get_word_peek_not_equal("{")){
            if(name.empty())
                name = input.get_word();
            else {
             name += " ";
             name += input.get_word();
            }
        }

        input.get_word_equal_or_assert(")");
    } else throw parse_error("error on %d:%d: expecting domain element specifier, but got '%s'", input.get_row(), input.get_col(), input.get_word_peek().c_str());

    input.get_word_equal_or_assert("{");
    while(input.get_word_peek_not_equal("}")){
        attribute nattr;
        nattr.parse(input);
        attr.push_back(nattr);
    }
    input.get_word_equal_or_assert("}");
}

void domain_header::parse(reader &input){
    input.get_word_equal_or_assert("net");
    input.get_word_equal_or_assert("{");
    while(input.get_word_peek_not_equal("}")){
        attribute nattr;
        nattr.parse(input);
        attr.push_back(nattr);
    }
    input.get_word_equal_or_assert("}");
}

void attribute::parse(reader &input){
    name = input.get_word();
    input.get_word_equal_or_assert("=");
    value.parse(input);
    input.get_word_equal_or_assert(";");
}

variable::variable(){
    dimensions = 0;
    type = undefined_variable;
}

void variable::parse(reader &input, unsigned int d){
    if(d > dimensions){
        dimensions = d;
        dim.push_back(0);
    }

    if(input.get_word_peek_equal("(")){
        input.get_word_equal_or_assert("(");
        parse(input, d+1);
        input.get_word_equal_or_assert(")");
        if(d > 0)
            dim[d-1]++;
        if(input.get_word_peek_equal("("))
            parse(input, d);

    } else if(input.get_word_peek_not_equal(")")) {
        if(type == undefined_variable){
            if(input.get_word_peek_equal("\""))
                type = string_variable;
            else
                type = float_variable;
        }

        int values = 0;
        do {
            values++;
            if(type == string_variable){
                input.get_word_equal_or_assert("\"");
                if(input.get_word_peek_not_equal("\""))
                    value.push_back(input.get_word());
                input.get_word_equal_or_assert("\"");
            } else value.push_back(input.get_word());
        } while(d>0 && input.get_word_peek_not_equal(")"));
        if(d > 0)
            dim[dim.size()-1] = values;
    }
}

void attribute::print(){
    printf("            %s %s = ", (value.type==float_variable?"float":(value.type==string_variable?"string":"unknown_type")), name.c_str());
    value.print();
}

void variable::print(){
    printf("%dd", dimensions);
    for(unsigned int i = 0; i < dim.size(); i++)
        printf("[%d]", dim[i]);
    for(unsigned int i = 0; i < value.size(); i++)
        printf(" %s", value[i].c_str());
    printf("\n");
}

void domain_definition::print(){
    printf("DEFINITION:\n");
    header.print();
    for(unsigned int i = 0; i < elements.size(); i++){
        elements[i].print();
    }
}

void domain_header::print(){
    printf("\n    HEADER:\n");
    printf("        ATTRIBUTES:\n");
    for(unsigned int i = 0; i < attr.size(); i++){
        attr[i].print();
    }
}

void domain_element::print(){
    printf("\n    ELEMENT\n");
    printf("        TYPE : %s\n", (type==node_domain_element?"node":(type==potential_domain_element?"potential":"?")));
    printf("        NAME : %s\n", name.c_str());
    printf("        ATTRIBUTES:\n");
    for(unsigned int i = 0; i < attr.size(); i++){
        attr[i].print();
    }

}

void hugin::print(){
    definition.print();
}

attribute* domain_element::get_attribute(string varname){
    for(auto it = attr.begin(); it != attr.end(); it++)
        if(it->name == varname)
            return &*it;
    return NULL;
}

void bidirected(dbn_t *net, vector<string> &words, string comb[2], int s = 0, int d = 0){
    typedef dbn_t::node node;

    if(d < 2) {
        for(unsigned int i = s; i < words.size()-(2-d); i++){
            comb[d] = i;
            bidirected(net, words,comb,i+1,d+1);
        }
    } else if(d == 2) {
        for(unsigned int i = s; i < words.size(); i++){
            comb[d] = i;
            node *n0 = net->get_node(comb[0]);
            node *n1 = net->get_node(comb[1]);

            n0->parent.push_back(n1);
            n1->parent.push_back(n0);
            n0->child.push_back(n1);
            n1->child.push_back(n0);
        }
    } else return;
}

bayesnet* hugin::get_bayesnet(){
    dbn_t* net = new dbn_t;
    bn_t *bn = NULL;

    if(net){
        for(auto it = definition.elements.begin(); it != definition.elements.end(); it++){
            if(it->type == node_domain_element){
                dbn_t::node *n = net->get_node(it->name);
                attribute *attr = it->get_attribute("states");
                if(attr == NULL)
                    throw hugin_error("node does not appear to have any states");
                n->value = attr->value.value;
            } else if(it->type == potential_domain_element){
                vector<string> words = reader::string_to_words(it->name);
                bool base = true;
                vector<string> pre;
                vector<string> post;
                for(unsigned int i = 0; i < words.size(); i++){
                    if(words[i] == "|")
                        base = false;
                    else {
                        if(base)
                            pre.push_back(words[i]);
                        else post.push_back(words[i]);
                    }
                }

                if(pre.size() > 1){ // bidirectional edges
                    std::string comp[2];
                    bidirected(net, pre, comp);
                }

                for(unsigned int i = 0; i < pre.size(); i++){
                    dbn_t::node *pre_n = net->get_node(pre[i]);
                    for(unsigned int j = 0; j < post.size(); j++){
                        dbn_t::node *post_n = net->get_node(post[j]);

                        pre_n->parent.push_back(post_n);
                        post_n->child.push_back(pre_n);
                    }
                }

                dbn_t::node *n = net->get_node(words[0]);
                attribute *attr = it->get_attribute("data");

                if (attr == NULL)
                    throw hugin_error("node does not appear to have a CPT");
                if (n == NULL)
                    throw hugin_error("node '%s' not found to store CPT", words[0].c_str());

                for (unsigned int i = 0; i < attr->value.value.size(); i++)
                    n->cpt.push_back(atof(attr->value.value[i].c_str()));

            } else throw hugin_error("node type unknown");
        }

        try {
            net->finalize();
        } catch(dynamic_bayesnet_error &e){
            printf("dynamic bayesnet error: %s\n", e.what());
            throw hugin_error("error finalizing net: %s", e.what());
        }

        bn = new bn_t();
        bn->init(net);
        delete net;
    } else throw hugin_error("could not allocate bayesnet");

    if(bn)
        bn->set_filename(filename.c_str());

    return bn;
}


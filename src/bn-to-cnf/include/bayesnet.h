#ifndef BAYESNET_H
#define BAYESNET_H

#include <string>
#include <vector>
#include <map>
#include "error.h"
#include "types.h"
#include "config.h"

typedef throw_string_error bayesnet_error;
typedef throw_string_error dynamic_bayesnet_error;

class cnf;

class dynamic_bayesnet {
    friend class bayesnet;
    public:
        struct node {
            std::vector <dynamic_bayesnet::node*> parent;
            std::vector <dynamic_bayesnet::node*> child;
            std::vector <std::string> value;
            std::vector <probability_t> cpt;
            std::vector <unsigned int> dim;
            std::string name;

            node(std::string node_name) : name(node_name) {};
            node(){};
        };

        dynamic_bayesnet::node* get_node(std::string);
        void finalize();
        int wcc();
        void print();
    private:
        unsigned int parent_size;
        unsigned int child_size;
        unsigned int dim_size;
        unsigned int cpt_size;

        std::map<std::string,dynamic_bayesnet::node> nodes;
};

class bayesnet {
    friend class cnf;

    struct bayesdict {
        bayesdict& operator=(bayesdict*);
        std::vector< std::vector<std::string> > value;
        std::map<std::string,unsigned int> name_to_id;
        std::vector<std::string> id_to_name;
    };

    public:
        bayesnet();
        ~bayesnet();
        void set_filename(const char*);
        const char* get_filename();
        void init(dynamic_bayesnet *);
        std::string get_node_name(unsigned int);
        std::string get_node_value_name(unsigned int,unsigned int);
        int get_node_id(std::string);
        void print();

        bayesdict* get_dict();
        bayesdict* get_dict_cpy();
        void clear_dict();
        char* serialize();
        void deserialize(char*,bool);
        void clear();
        bayesnet& operator=(bayesnet*);
        bayesnet& operator=(bayesnet&);
        unsigned int get_nr_variables();

        static bayesnet* read(char*);

        inline uint32_t* get_states();
        inline uint32_t* get_parent(unsigned int);
        inline uint32_t* get_child(unsigned int);
        inline uint32_t get_states(unsigned int);
        inline probability_t* get_cpt(unsigned int);
        inline unsigned int get_parent_size(unsigned int);
        inline unsigned int get_child_size(unsigned int);
        inline unsigned int get_cpt_size(unsigned int);
        inline unsigned int get_parent_size();
        inline unsigned int get_child_size();
        inline unsigned int get_cpt_size();
    private:
        void destroy();
        BITSTREAM msg;
        SIZE msg_size;
        bool dirty;
        SIZE size;
        probability_t *cpt;
        uint32_t
            *parent,
            *child,
            *states,
            *cpt_offset,
            *parent_offset,
            *child_offset;

        bayesdict *dict;
        std::string filename;
};

typedef dynamic_bayesnet dbn_t;
typedef bayesnet bn_t;

inline uint32_t* bayesnet::get_parent(unsigned int i){
    return &parent[parent_offset[i]];
}

inline uint32_t* bayesnet::get_child(unsigned int i){
    return &child[child_offset[i]];
}

inline uint32_t bayesnet::get_states(unsigned int i){
    return states[i];
}

inline uint32_t* bayesnet::get_states(){
    return states;
}

inline probability_t* bayesnet::get_cpt(unsigned int i){
   return &cpt[cpt_offset[i]];
}

inline unsigned int bayesnet::get_parent_size(unsigned int i){
    return parent_offset[i+1]-parent_offset[i];
}

inline unsigned int bayesnet::get_child_size(unsigned int i){
    return child_offset[i+1]-child_offset[i];
}

inline unsigned int bayesnet::get_cpt_size(unsigned int i){
    return cpt_offset[i+1]-cpt_offset[i];
}

inline unsigned int bayesnet::get_parent_size(){
    return parent_offset[size];
}

inline unsigned int bayesnet::get_child_size(){
    return child_offset[size];
}

inline unsigned int bayesnet::get_cpt_size(){
    return cpt_offset[size];
}

#endif

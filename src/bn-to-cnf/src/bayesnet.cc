#include "bayesnet.h"
#include <deque>
#include <list>
#include <algorithm>
#include <string.h>
#include "cnf.h"
#include "parser.h"
using namespace std;

dynamic_bayesnet::node* dynamic_bayesnet::get_node(std::string name){
    dynamic_bayesnet::node* n = &(nodes[name]);
    n->name = name;
    return n;
}

int dynamic_bayesnet::wcc(){
    int node_count = nodes.size();
    if(node_count == 0)
        return 0;

    vector< list<string> > subnet;
    map<dynamic_bayesnet::node*,bool> frontier;
    for(auto it = nodes.begin(); it != nodes.end(); it++)
        frontier[&(it->second)] = true;

    deque<dynamic_bayesnet::node*> q;
    while(node_count > 0){
        unsigned int subnet_id = subnet.size();
        subnet.resize(subnet_id+1);

        for(auto it = frontier.begin(); it != frontier.end(); it++){
            if(it->second){
                q.push_back(it->first);
                it->second = false;
                node_count--;
                break;
            }
        }

        while(!q.empty()){
            dynamic_bayesnet::node *n = q.front();
            subnet[subnet_id].push_back(n->name);
            q.pop_front();
            node_count--;

            for(unsigned int i = 0; i < n->parent.size(); i++){
                if(frontier[n->parent[i]]){
                    frontier[n->parent[i]] = false;
                    q.push_back(n->parent[i]);
                }
            }

            for(unsigned int i = 0; i < n->child.size(); i++){
                if(frontier[n->child[i]]){
                    frontier[n->child[i]] = false;
                    q.push_back(n->child[i]);
                }
            }
        }
    }

    int list_id = -1;
    for(unsigned int i = 0; i < subnet.size(); i++){
        if(list_id == -1 || (subnet[list_id].size() < subnet[i].size()))
            list_id = i;
    }

    int node_count_deleted = 0;
    if(list_id >= 0){
        int node_count_deleted = nodes.size() - subnet[list_id].size();
        if(node_count_deleted > 0){
            auto it = nodes.begin();
            while(it != nodes.end()){
                if(find(subnet[list_id].begin(), subnet[list_id].end(), it->second.name) == subnet[list_id].end()){
                    auto del_it = it;
                    it++;
                    nodes.erase(del_it);
                } else it++;
            }
        }
    } else throw dynamic_bayesnet_error("no connected components");

    return node_count_deleted;
}

void dynamic_bayesnet::print(){
    printf("number of nodes: %lu\n", nodes.size());
    for(auto it = nodes.begin(); it != nodes.end(); it++){
        printf("  * node %s\n", it->first.c_str());

        printf("    - values   (%lu):", it->second.value.size());
        for(unsigned int j = 0; j < it->second.value.size(); j++)
            printf(" [%s]", it->second.value[j].c_str());
        printf("\n");

        printf("    - parents  (%lu):", it->second.parent.size());
        for(unsigned int j = 0; j < it->second.parent.size(); j++)
            printf(" %s", it->second.parent[j]->name.c_str());
        printf("\n");

        printf("    - children (%lu):", it->second.child.size());
        for(unsigned int j = 0; j < it->second.child.size(); j++)
            printf(" %s", it->second.child[j]->name.c_str());
        printf("\n");

        printf("    - CPT");
        for(unsigned int j = 0; j < it->second.dim.size(); j++)
            printf("[%d]", it->second.dim[j]);
        printf(" (%lu):", it->second.cpt.size());
        for(unsigned int j = 0; j < it->second.cpt.size(); j++)
            printf(" %lf", it->second.cpt[j]);
        printf("\n");
    }
}

void dynamic_bayesnet::finalize(){
    wcc();

    parent_size = 0;
    child_size = 0;
    dim_size = 0;
    cpt_size = 0;
    for(auto it = nodes.begin(); it != nodes.end(); it++){
        it->second.dim.resize(it->second.parent.size()+1);
        it->second.dim[0] = it->second.value.size();
        for(unsigned int i = 0; i < it->second.parent.size(); i++)
            it->second.dim[i+1] = it->second.parent[i]->value.size();

        unsigned int mult = it->second.dim[0];
        for(unsigned int i = 1; i < it->second.dim.size(); i++)
            mult *= it->second.dim[i];

        if(mult){
            if(it->second.cpt.size() != mult)
                throw dynamic_bayesnet_error("CPT of node '%s' is incomplete", it->second.name.c_str());
        } else throw dynamic_bayesnet_error("A node connect to '%s' has no values", it->second.name.c_str());


        parent_size += it->second.parent.size();
        dim_size += it->second.dim.size();
        cpt_size += it->second.cpt.size();
    }
    child_size = parent_size;
}

// ---------------------------------------------------------

bayesnet::bayesdict& bayesnet::bayesdict::operator=(bayesnet::bayesdict* dict){
    if (this != dict) {
        value = dict->value;
        name_to_id = dict->name_to_id;
        id_to_name = dict->id_to_name;
    }
    return *this;
}

bayesnet::bayesdict* bayesnet::get_dict(){
    return dict;
}

bayesnet::bayesdict* bayesnet::get_dict_cpy(){
    bayesnet::bayesdict* dict_cpy = new bayesnet::bayesdict();
    *dict_cpy = dict;
    return dict_cpy;
}

bayesnet::bayesnet(){
    dict = NULL;
    size = 0;
    msg = NULL;
    dirty = true;
    clear();
}

bayesnet::~bayesnet(){
    destroy();
    clear();
    clear_dict();
}

void bayesnet::clear_dict(){
    if(dict){
        delete dict;
        dict = NULL;
    }
}

unsigned int bayesnet::get_nr_variables(){
    return size;
}

void bayesnet::init(dynamic_bayesnet *dbn){
    size = dbn->nodes.size();
    msg = NULL;
    dirty = true;

    cpt = (probability_t*) malloc(sizeof(probability_t)*dbn->cpt_size);
    parent = (uint32_t*) malloc(sizeof(uint32_t)*dbn->parent_size);
    child = (uint32_t*) malloc(sizeof(uint32_t)*dbn->child_size);
    cpt_offset = (uint32_t*) malloc(sizeof(uint32_t)*(size+1));
    parent_offset = (uint32_t*) malloc(sizeof(uint32_t)*(size+1));
    child_offset = (uint32_t*) malloc(sizeof(uint32_t)*(size+1));
    states = (uint32_t*) malloc(sizeof(uint32_t)*size);

    clear_dict();
    dict = new bayesdict();

    for(auto it = dbn->nodes.begin(); it != dbn->nodes.end(); it++){
        unsigned int id = dict->id_to_name.size();
        dynamic_bayesnet::node *n = &(it->second);

        dict->id_to_name.push_back(n->name);
        dict->name_to_id[n->name] = id;
    }

    parent_offset[0] = 0;
    child_offset[0] = 0;
    cpt_offset[0] = 0;
    for(auto it = dbn->nodes.begin(); it != dbn->nodes.end(); it++){
        dynamic_bayesnet::node *n = &(it->second);
        unsigned int id = dict->name_to_id[n->name];

        parent_offset[id+1] = parent_offset[id] + n->parent.size();
        for(unsigned int i = 0; i < n->parent.size(); i++)
            parent[parent_offset[id]+i] = dict->name_to_id[n->parent[i]->name];

        child_offset[id+1] = child_offset[id] + n->child.size();
        for(unsigned int i = 0; i < n->child.size(); i++)
            child[child_offset[id]+i] = dict->name_to_id[n->child[i]->name];

        states[id] = n->value.size();

        cpt_offset[id+1] = cpt_offset[id] + n->cpt.size();
        memcpy(cpt + cpt_offset[id], &(n->cpt[0]), sizeof(probability_t)*n->cpt.size());

        dict->value.resize(id+1);
        dict->value[id] = n->value;
    }
}

int bayesnet::get_node_id(string name){
    if(dict){
        auto it = dict->name_to_id.find(name);
        if(it != dict->name_to_id.end())
            return it->second;
    }
    return -1;
}

string bayesnet::get_node_value_name(unsigned int id, unsigned int i){
    if(dict && id < size && i < dict->value[id].size())
        return dict->value[id][i];
    else return string("");
}

string bayesnet::get_node_name(unsigned int id){
    if(dict && id < size)
        return dict->id_to_name[id];
    else return string("");
}

void bayesnet::print(){
    printf("number of nodes: %u\n", size);
    for(unsigned int i = 0; i < size; i++){
        if(dict){
            printf("  * node %d: %s\n", i, dict->id_to_name[i].c_str());

            printf("    - values   (%lu):", states[i]);
            for(unsigned int j = 0; j < dict->value[i].size(); j++)
                printf(" [%s]", dict->value[i][j].c_str());
            printf("\n");
        } else {
            printf("  * node %d\n", i);
            printf("    - values: %d\n", states[i]);
        }

        unsigned int parent_size = get_parent_size(i);
        printf("    - parents  (%u):", parent_size);
        uint32_t* parent = get_parent(i);
        for(unsigned int j = 0; j < parent_size; j++)
            printf(" %d", parent[j]);
        printf("\n");

        unsigned int child_size = get_child_size(i);
        printf("    - children (%u):", child_size);
        uint32_t* child = get_child(i);
        for(unsigned int j = 0; j < child_size; j++)
            printf(" %d", child[j]);
        printf("\n");

        unsigned int cpt_size = get_cpt_size(i);
        probability_t* cpt = get_cpt(i);
        printf("    - CPT      (%u):",cpt_size);
        for(unsigned int j = 0; j < cpt_size; j++)
            printf(" %lf", cpt[j]);
        printf("\n");
    }
}

bayesnet* bayesnet::read(char *infile){
    parser<hugin> net;
    printf("Parsing HUGIN file\n");
    if(!net.process(infile))
        fprintf(stderr, "Failed to parse %s\n", infile);
    else {
        printf("DONE\n\n");

        // get bayesian network
        bayesnet *bn = NULL;
        try {
            printf("Loading Bayesian Netork\n");
            bn = net.get_bayesnet();
            if(bn == NULL){
                fprintf(stderr, "FAILED\n");
                return NULL;
            } else {
                printf("DONE\n\n");
                return bn;
            }
        } catch(throw_string_error &e){
            fprintf(stderr, "error: %s\n", e.what());
            fprintf(stderr, "FAILED\n");
            return NULL;
        }
    }
    return NULL;
}

void bayesnet::clear(){
    if(size > 0){
        size = 0;
        free(cpt);
        free(parent);
        free(child);
        free(cpt_offset);
        free(parent_offset);
        free(child_offset);
        free(states);
    }
    cpt = NULL;
    parent = NULL;
    child = NULL;
    cpt_offset = NULL;
    parent_offset = NULL;
    child_offset = NULL;
    states = NULL;
}

void bayesnet::destroy(){
    if(msg){
        free(msg);
        msg = NULL;
        dirty = true;
        msg_size = 0;
    }
}

template <class T>
inline void scpy(BITSTREAM &s, T &v, bool reverse = false,unsigned int n = 1){
    if(reverse)
        memcpy((void*) &v, s, sizeof(T)*n);
    else
        memcpy(s,(void*) &v, sizeof(T)*n);

    s += sizeof(T)*n;
}

template <class T>
inline void scpy(BITSTREAM &s, T *&v, bool reverse = false, unsigned int n = 1){
    if(reverse)
        memcpy((void*) v, s, sizeof(T)*n);
    else
        memcpy(s,(void*) v, sizeof(T)*n);

    s += sizeof(T)*n;
}

char* bayesnet::serialize(){
    if(dirty || msg == NULL){
        destroy();

        // calculate size
        msg_size = sizeof(SIZE)*10;
        msg_size += sizeof(uint32_t) * 5 * (size+1);
        msg_size += sizeof(probability_t) * cpt_offset[size];
        msg_size += sizeof(uint32_t) * parent_offset[size];
        msg_size += sizeof(uint32_t) * child_offset[size];

        msg = (BITSTREAM) malloc(sizeof(BIT)*msg_size);
        if(msg == NULL)
            return NULL;

        bool reverse = false;
        BIT *s = msg;

        scpy(s,msg_size);

        // offsets
        scpy(s,size,reverse);
        scpy(s,cpt_offset,reverse,size+1);
        scpy(s,size,reverse);
        scpy(s,parent_offset,reverse,size+1);
        scpy(s,size,reverse);
        scpy(s,child_offset,reverse,size+1);
        scpy(s,size,reverse);
        scpy(s,states,reverse,size+1);

        // data
        scpy(s,cpt_offset[size],reverse);
        scpy(s,cpt,reverse,cpt_offset[size]);
        scpy(s,parent_offset[size],reverse);
        scpy(s,parent,reverse,parent_offset[size]);
        scpy(s,child_offset[size],reverse);
        scpy(s,child,reverse,child_offset[size]);

        dirty = false;
    }
    return msg;
}

void bayesnet::deserialize(BITSTREAM strm, bool shared = false){
    if(strm){
        destroy();

        if(!shared){
            msg = strm;
            dirty = false;
        }

        bool reverse = true;
        BIT *s = strm;
        scpy(s,msg_size,reverse);
        scpy(s,size,reverse);

        cpt_offset = (uint32_t*) malloc(sizeof(uint32_t)*(size+1));
        parent_offset = (uint32_t*) malloc(sizeof(uint32_t)*(size+1));
        child_offset = (uint32_t*) malloc(sizeof(uint32_t)*(size+1));
        states = (uint32_t*) malloc(sizeof(uint32_t)*(size+1));

        // offsets
        scpy(s,cpt_offset,reverse,size+1);
        scpy(s,size,reverse);
        scpy(s,parent_offset,reverse,size+1);
        scpy(s,size,reverse);
        scpy(s,child_offset,reverse,size+1);
        scpy(s,size,reverse);
        scpy(s,states,reverse,size+1);

        cpt = (probability_t*) malloc(sizeof(probability_t)*cpt_offset[size]);
        parent = (uint32_t*) malloc(sizeof(uint32_t)*parent_offset[size]);
        child = (uint32_t*) malloc(sizeof(uint32_t)*child_offset[size]);

        // data
        SIZE tmp_size;
        scpy(s,tmp_size,reverse);
        scpy(s,cpt,reverse,tmp_size);
        scpy(s,tmp_size,reverse);
        scpy(s,parent,reverse,tmp_size);
        scpy(s,tmp_size,reverse);
        scpy(s,child,reverse,tmp_size);

    }
}

bayesnet& bayesnet::operator=(bayesnet& net){
    if (this != &net) {
       this->deserialize(net.serialize(),true);
    }
    return *this;
}

bayesnet& bayesnet::operator=(bayesnet* net){
    if (this != net) {
       this->deserialize(net->serialize(),true);
    }
    return *this;
}

void bayesnet::set_filename(const char *f){
    filename = f;
}

const char* bayesnet::get_filename(){
    return filename.c_str();
}


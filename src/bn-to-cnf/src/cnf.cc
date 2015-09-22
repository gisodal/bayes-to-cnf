#include "cnf.h"
#include "misc.h"
#include <quine-mccluskey/qm.h>
#include "bayesnet.h"
#include <stack>
#include <array>
#include <string.h>
#include <algorithm>
#include <set>
#include <tuple>
#include <array>
#include <map>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

template <class T>
T& dynamic_assign(std::vector<T> &v, unsigned int i){
    if(v.size() <= i)
        v.resize(i+1, 0);
    return v[i];
}

void expression::finalize(){
    for(auto it = literals.begin(); it != literals.end(); it++)
        it->sat.resize(it->clauses.size());

    for(auto it = clauses.begin(); it != clauses.end(); it++)
        it->sat.resize(it->literals.size());
}

void expression::init(){
    for(auto it = literals.begin(); it != literals.end(); it++){
        memset(&((it->sat)[0]), 0, it->sat.size()*sizeof(bool_t));
        unsat = it->sat.size();
    }

    for(auto it = clauses.begin(); it != clauses.end(); it++){
        memset(&it->sat[0], 0, it->sat.size()*sizeof(bool_t));
        unsat = it->sat.size();
    }

    unsat = clauses.size();
    empty = 0;
}

cnf::cnf(){
    filename = NULL;
    clear();
}

cnf::~cnf(){
    if(filename){
        free(filename);
        filename = NULL;
    }
}

int cnf::condition(int l){
    if(l <= LITERALS){
        literal_t &ll = expr.literals[l];
        for(auto cit = ll.clauses.begin(); cit != ll.clauses.end(); cit++){
            clause_t &c = expr.clauses[*cit];
            for(unsigned int i = 0; i < c.literals.size(); i++){
                if(c.literals[i] == l)
                    memset(&(c.sat[0]), 1, c.sat.size()*sizeof(bool_t));
                else if(c.literals[i] == -1*l)
                    c.sat[i] = 1;
            }
        }
    }
    return 0;
}

void cnf::clear(){
    values.clear();
    literal_to_variable.clear();
    variable_to_literal.clear();
    clause_to_variable.clear();
    clause_to_probability.clear();
    probability_to_weight.clear();

    LITERALS = 0;
    VARIABLES = 0;
    OPT_SUPRESS_CONSTRAINTS = false;
    OPT_EQUAL_PROBABILITIES = false;
    OPT_DETERMINISTIC_PROBABILITIES = false;
    OPT_DETERMINISM = false;
    OPT_SYMPLIFY = false;
    OPT_QUINE_MCCLUSKEY = false;
    OPT_BOOL = false;
    expr.clauses.clear();
    expr.literals.clear();
    expr.unsat = 0;
    expr.empty = 0;
    qm_eligible = 0;
    qm_possible = 0;
    qm_variable_count.clear();
    QM_LIMIT = -1;
    encoding = 0; // default encoding containing constraints
    this->~cnf();
}


int cnf::read(char *name){
    FILE *file = fopen(name,"r");
    if(file){
        char buf[256];
        unsigned int probabilities = 0;
        while(fgets(buf, sizeof(buf), file) != NULL){
            unsigned int v, x = 0;
            char c = '\0';
            sscanf(buf, "%u%c", &v, &c);
            VARIABLES = (v >= VARIABLES?v+1:VARIABLES);

            if(c == '>')
                x = 1;
            else if(c == ':')
                x = 2;

            if(x > 0){
                char *str = strchr(buf, '>');
                if(str == NULL)
                    str = strchr(buf, ':')+1;
                else
                    str += 1;

                char *token = strsep(&str, ":");
                char *str2, *fstr2;
                fstr2 = str2 = strdup(token);
                if(x == 2){
                    char *token = strsep(&str, ":");
                    unsigned int ctop;
                    sscanf(token, "%u",&ctop);

                    unsigned int size = clause_to_probability.size();
                    clause_to_probability.resize(size+1);
                    clause_to_probability[size] = ctop;

                    float p;
                    token = strsep(&str, ":");
                    sscanf(token, "%f",&p);
                    if(probability_to_weight.size() <= ctop)
                        probability_to_weight.resize(ctop+1);
                    probability_to_weight[ctop] = p;

                    clause_to_variable.push_back(v);
                }

                if(x == 2)
                    expr.clauses.resize(expr.clauses.size()+1);

                unsigned int count = 0;
                token = strsep(&str2, ",");
                while( token != NULL && token[0] != '\0'){
                    literal_value_t l;
                    sscanf(token, "%d", &l);

                    LITERALS = (l >= LITERALS?l+1:LITERALS);

                    if(x == 2)
                        expr.clauses.back().literals.push_back(l);

                    if(literal_to_variable.size() <= l)
                        literal_to_variable.resize(l+1);
                    literal_to_variable[l] = v;

                    count++;
                    token = strsep(&str2, ",");
                }

                if(x == 1){
                    if(values.size() <= v)
                        values.resize(v+1);
                    values[v] = count;
                }
                free(fstr2);
            }
        }
        //fix_literal_implications(); FIXME: literals to clause "pointers" not implemented

        variable_to_literal.assign(VARIABLES,~0u);
        for(unsigned int i = 0; i < literal_to_variable.size(); i++)
            if(variable_to_literal[literal_to_variable[i]] > i)
                variable_to_literal[literal_to_variable[i]] = i;

        fclose(file);
    } else fprintf(stderr, "Could not open file '%s'\n", name);
    return 0;
}

int cnf::write(char* basename, bool dimacs, bayesnet *bn){
    char name[100];
    strcpy(name, basename);
    if(!dimacs)
        sprintf(name+strlen(name), ".%u", encoding);

    sprintf(name+strlen(name),".cnf");
    printf("output: %s\n", name);
    FILE *file = fopen(name,"w");
    if(file){
        if(dimacs){
            fprintf(file, "c DIMACS CNF Format\n");
            fprintf(file, "c ===================================================\n");
            unsigned int counter = 0;
            for(unsigned int i = 0; i < expr.clauses.size(); i++)
                if(!OPT_SYMPLIFY || get_weight(i) != 1)
                    counter++;

            fprintf(file, "p cnf %u %u\n", LITERALS+probability_to_weight.size(), counter);
            bool removed = false;
            for(unsigned int i = 0; i < expr.clauses.size(); i++){
                if(!OPT_SYMPLIFY || get_weight(i) != 1){
                    clause &c = expr.clauses[i];
                    for(unsigned int j = 0; j < c.literals.size(); j++)
                        fprintf(file, "%d ", c.literals[j]);

                    if(get_weight(i) != -1 && (!OPT_SYMPLIFY || get_weight(i) != 0))
                        fprintf(file, "%u ", LITERALS+1+clause_to_probability[i]);

                    fprintf(file,"0\n");
                } else removed = true;
            }

            fprintf(file, "c ===================================================\n");
            fprintf(file, "c\n");
            fprintf(file, "c clauses       : %-6u\n", expr.clauses.size());
            fprintf(file, "c literals      : %-6u (1-%u)\n", LITERALS,LITERALS);
            fprintf(file, "c probabilities : %-6u (%u-%u)\n", probability_to_weight.size(), LITERALS+1, LITERALS+probability_to_weight.size());
            fprintf(file, "c\n");
            fprintf(file, "c probabilities:\n");
            for(unsigned int i = 0; i < probability_to_weight.size(); i++)
                fprintf(file, "c %u %f\n", LITERALS+1+i, probability_to_weight[i]);

            fprintf(file, "c\nc variable to literal mapping:\n");
            for(unsigned int v = 0; v < VARIABLES; v++){
                fprintf(file, "c %u = {",v);
                for(unsigned int l = 0; l < values[v]; l++){
                    if(l > 0) fprintf(file,",");
                    fprintf(file, "%u",variable_to_literal[v]+l);
                }
                fprintf(file,"}\n");
            }

            if(bn){
                fprintf(file, "c\nc variable and value names:\n");
                fprintf(file, "c  <variable> <#values> <variable name>\n");
                fprintf(file, "c     <value literal> <value name>\n");
                fprintf(file, "c    [<value literal> <value name>]\n");
                for(unsigned int v = 0; v < VARIABLES; v++){
                    fprintf(file, "c %u %u \"%s\"\n", v, values[v], bn->get_node_name(v).c_str());
                    for(unsigned int l = 0; l < values[v]; l++)
                        fprintf(file, "c   %u \"%s\"\n", variable_to_literal[v]+l, bn->get_node_value_name(v,l).c_str());
                }
            }

        } else {
            for(unsigned int v = 0; v < VARIABLES; v++){
                fprintf(file, "%u>", v);
                if(OPT_BOOL && values[v] == 2){
                    fprintf(file, "%u\n", variable_to_literal[v]);
                } else {
                    for(unsigned int l = 0; l < values[v]; l++){
                        if(l > 0)
                            fprintf(file, ",");
                        fprintf(file, "%u", variable_to_literal[v]+l);
                    }
                    fprintf(file, "\n");
                }
            }
            for(unsigned int i = 0; i < expr.clauses.size(); i++){
                clause &c = expr.clauses[i];
                fprintf(file, "%u:", clause_to_variable[i]);
                for(unsigned int j = 0; j < c.literals.size(); j++){
                    if(j > 0)
                        fprintf(file, ",");
                    fprintf(file, "%d", c.literals[j]);
                }
                fprintf(file, ":%u:%f\n", clause_to_probability[i], probability_to_weight[clause_to_probability[i]]);
            }
        }
        fclose(file);
    } else fprintf(stderr, "Could not open file '%s'\n", name);
    return 0;
}

void cnf::stats(){
    int max = 0;
    int min = -1;
    unsigned long int total = 0;
    for(auto it = expr.clauses.begin(); it != expr.clauses.end(); it++){
        int size = it->literals.size();
        total += size;
        if(size > max)
            max = size;
        if(min < 0 || size < min)
            min = size;
    }

    vector<uint32_t> sizes(max+1,0);
    for(auto it = expr.clauses.begin(); it != expr.clauses.end(); it++)
        sizes[it->literals.size()]++;

    printf("Optimizations\n");
    printf("    Suppress constraints : %s\n", (OPT_SUPRESS_CONSTRAINTS?"YES":"NO") );
    printf("    Equal probabilities  : %s\n", (OPT_EQUAL_PROBABILITIES?"YES":"NO") );
    printf("    Determinism          : %s\n", (OPT_DETERMINISM?"YES":"NO") );
    printf("    Simplify             : %s\n", (OPT_SYMPLIFY?"YES":"NO") );
    printf("\n");
    printf("Variables       : %d\n", VARIABLES);
    printf("Probabilities   : %d\n", probability_to_weight.size());
    printf("Literals        : %d\n", LITERALS);
    printf("Clauses         : %d\n", expr.clauses.size());
    printf("Literal/clauses : %.2f \n", (float) total/expr.clauses.size());
    printf("Clause sizes    : %d-%d\n", min, max);
    //printf("clauses/size    : ");
    //for(unsigned int i = 1; i <= max; i++)
    //    printf("%5d ", sizes[i]);
    //printf("\n                  ");
    //for(unsigned int i = 1; i <= max; i++)
    //    printf("%5d ", i);
    //printf("\n\n");

    if(qm_possible){
        char dfilename[200];
        if(OPT_BOOL)
            sprintf(dfilename,"%s.bool.qm",filename);
        else
            sprintf(dfilename,"%s.qm",filename);

        FILE *file = fopen(dfilename,"w");
        if(file){
            fprintf(file,"# clause_size,clauses_count,sum_clause_count\n");
            unsigned sum = 0;
            for(unsigned int i = 0; i < qm_variable_count.size(); i++){
                sum += qm_variable_count[i];
                fprintf(file,"%u,%u,%u\n",i, qm_variable_count[i], sum);
            }
            fclose(file);
        }
    }


    //printf("#file,encoding,bool,qm_limit,variables,probabilities,literals,clauses,literal/clauses,clause_min_size,clause_max_size");
    //printf("\n");

    //printf("%s,",filename);
    //printf("%d,",encoding);
    //printf("%d,", OPT_BOOL);
    //printf("%d,", QM_LIMIT);
    //printf("X%d,", VARIABLES);
    //printf("P%d,", probability_to_weight.size());
    //printf("L%d,", LITERALS);
    //printf("C%d,", expr.clauses.size());
    //printf("LC%.4f,", (float) total/expr.clauses.size());
    //printf("%d,%d,", min, max);
    //printf("\n");
}

int cnf::encode(bayesnet *bn){
    if(encoding < 0)
        return -1;

    VARIABLES = bn->size;
    values.resize(VARIABLES);
    memcpy(&values.front(), bn->states,sizeof(uint32_t)*VARIABLES);

    LITERALS = 0;
    for(unsigned int i = 0; i < VARIABLES; i++){
        if(OPT_BOOL && bn->states[i] == 2)
            LITERALS += 1;
        else
            LITERALS += bn->states[i];
    }

    variable_to_literal.resize(VARIABLES);
    literal_to_variable.resize(LITERALS+1);
    unsigned int l = 1;
    for(unsigned int i = 0; i < VARIABLES; i++){
        variable_to_literal[i] = l;
        if(OPT_BOOL && values[i] == 2){
            literal_to_variable[l] = i;
            l += 1;
        } else {
            for (unsigned int v=0; v < values[i]; v++)
                literal_to_variable[l+v] = i;
            l += values[i];
        }
    }

    switch(encoding){
        case 0:
            encode_constraints(bn);
            break;
        case 1:
            encode_probabilities(bn);
            break;
        default:
            clear();
            return 1;
    }

    if(OPT_EQUAL_PROBABILITIES)
        encode_equal_probabilities();

    if(OPT_DETERMINISTIC_PROBABILITIES)
        encode_deterministic_probabilities();

    if(OPT_DETERMINISM)
        encode_determinism();

    if(OPT_QUINE_MCCLUSKEY)
        ;

    if(expr.clauses.size() == 0)
        clear();
    else expr.finalize();

    return 0;
}

void cnf::init(){
    expr.init();
}

void cnf::encode_constraints(bayesnet *bn){
    //expr.literals.resize(LITERALS);

    // variable encoding
    for(unsigned int i = 0; i < bn->size; i++){
        if(OPT_BOOL && bn->states[i] == 2)
            continue;

        uint32_t cid = expr.clauses.size();
        expr.clauses.resize(cid+1);
        clause_t &c = expr.clauses.back();
        clause_to_variable.push_back(i);
        clause_to_probability.push_back(-1);
        for(unsigned int v = 0; v < bn->states[i]; v++){
            uint32_t lid = v_to_l(i,v);
            c.literals.push_back(lid);

            //literal_t &l = expr.literals[lid];
           //l.clauses.push_back(cid);
        }
    }

    // constraint encoding
    for(unsigned int i = 0; i < bn->size; i++){
        if(OPT_BOOL && bn->states[i] == 2)
            continue;

        unsigned int values = bn->states[i];
        uint32_t literal_base = v_to_l(i,0);
        for(unsigned int v = 0; v < values-1; v++){
            for(unsigned int vv = v+1; vv < values; vv++){
                unsigned int cid = expr.clauses.size();
                expr.clauses.resize(cid+1);
                clause &c = expr.clauses.back();
                clause_to_variable.push_back(i);
                clause_to_probability.push_back(-1);

                c.literals.push_back(-1*(literal_base+v));
                c.literals.push_back(-1*(literal_base+vv));

               // clause_to_probability.push_back(1);

               // c.negated.push_back(true);
               // c.literals.push_back(literal_base+v);
               // literal_t &l = expr.literals[literal_base+v];
               // l.negated.push_back(true);
               // l.clauses.push_back(cid);

               // c.negated.push_back(true);
               // c.literals.push_back(literal_base+vv);
               // literal_t &l2 = expr.literals[literal_base+vv];
               // l2.negated.push_back(true);
               // l2.clauses.push_back(cid);
            }
        }
    }

    // all constraints have weight 0
    //for(unsigned int i = 0; i < clause_to_probability.size(); i++)
    //    probability_to_weight.push_back(0);

    // probability encoding
    encode_probabilities(bn);
}

void cnf::encode_probabilities(bayesnet *bn){
    //expr.literals.resize(LITERALS+1);

    // probability encoding
    for(unsigned int i = 0; i < bn->size; i++){
        unsigned int m = bn->get_parent_size(i);
        int *max = (int*) malloc(sizeof(int)*(m+1));
        int *ctr = (int*) malloc(sizeof(int)*(m+1));
        int *variable = (int*) malloc(sizeof(int)*(m+1));
        for(unsigned int j = 0; j < m; j++){
            variable[j] = bn->get_parent(i)[j];
            max[j] = values[variable[j]];
            ctr[j] = 0;
        }
        variable[m] = i;
        max[m] = values[i];
        ctr[m] = 0;

        int q = 0;
        while(true){
            clause_to_variable.push_back(i);
            probability_t p = bn->cpt[bn->cpt_offset[i]+q++];
            //if(p != 0 && p != 1){
                clause_to_probability.push_back(probability_to_weight.size());
                probability_to_weight.push_back(p);
            //} else clause_to_probability.push_back((uint32_t) p);

            uint32_t cid = expr.clauses.size();
            expr.clauses.resize(cid+1);
            clause_t &c = expr.clauses.back();
            for(unsigned int i = 0; i <= m; i++){
                uint32_t lid;
                if(OPT_BOOL && max[i] == 2){
                    lid = v_to_l(variable[i],0);
                    if(ctr[i]==1)
                        c.literals.push_back(-1*lid);
                    else
                        c.literals.push_back(lid);
                    //c.negated.push_back((ctr[i]==1));
                } else {
                    lid = v_to_l(variable[i],ctr[i]);
                    c.literals.push_back(-1*lid);
                }

                //literal_t &l = expr.literals[lid];
                //l.clauses.push_back(-1*cid);
            }

            if(ctr[m] < max[m]-1)
                ctr[m]++;
            else {
                for(int q = m-1; q >= 0; q--){
                    if(ctr[q] < max[q]-1){
                        ctr[q]++;
                        for(int r = q+1; r <= m; r++)
                            ctr[r] = 0;
                        break;
                    }
                }
                if(ctr[m] != 0)
                    break;
            }
        }

        free(max);
        free(ctr);
        free(variable);
    }
}

void cnf::encode_determinism(){
    // 1. set probabilities from 0 to -1 (remove probability literal)
    vector <probability_t> p_to_w;
    map< int, int > p_to_p;
    p_to_p[-1] = -1;
    for(unsigned int i = 0; i < clause_to_probability.size(); i++){
        int p = clause_to_probability[i];
        probability_t w = get_weight(i);
        if(w == 0)
            clause_to_probability[i] = -1;
        else if(p_to_p.find(p) == p_to_p.end()){
            p_to_p[p] = p_to_w.size();
            p_to_w.push_back(w);
        }
    }
    probability_to_weight = p_to_w;
    for(unsigned int i = 0; i < clause_to_probability.size(); i++)
        clause_to_probability[i] = p_to_p[clause_to_probability[i]];

    // 2. remove all clauses with probability 1
    p_to_p.clear(); p_to_w.clear();
    p_to_p[-1] = -1;
    for(unsigned int i = 0; i < clause_to_probability.size(); i++){
        int p = clause_to_probability[i];
        probability_t w = get_weight(i);
        if(w != 1 && p_to_p.find(p) == p_to_p.end()){
            p_to_p[p] = p_to_w.size();
            p_to_w.push_back(w);
        }
    }
    for(unsigned int i = 0; i < clause_to_probability.size(); i++){
        if(get_weight(i) == 1){
            clause_to_probability.erase(clause_to_probability.begin()+i);
            expr.clauses.erase(expr.clauses.begin()+i);
            clause_to_variable.erase(clause_to_variable.begin()+i);
            i--;
        } else clause_to_probability[i] = p_to_p[clause_to_probability[i]];
    }
    probability_to_weight = p_to_w;
}


void cnf::encode_deterministic_probabilities(){
    vector <probability_t> p_to_w;
    p_to_w.push_back(0);
    p_to_w.push_back(1);

    int d[2] = { 0 };
    map< int, int > p_to_p;
    p_to_p[-1] = -1;
    for(unsigned int i = 0; i < clause_to_probability.size(); i++){
        int p = clause_to_probability[i];
        probability_t w = get_weight(i);

        if(w == 0 || w == 1){
            p_to_p[p] = (uint32_t) w;
            d[(uint32_t) w] = 1;
        } else if(p_to_p.find(p) == p_to_p.end()){
            p_to_p[p] = p_to_w.size();
            p_to_w.push_back(w);
        }
    }

    if(!d[1]) p_to_w.erase(p_to_w.begin()+1);
    if(!d[0]) p_to_w.erase(p_to_w.begin());
    probability_to_weight = p_to_w;
    for(unsigned int i = 0; i < clause_to_probability.size(); i++){
        int &p = clause_to_probability[i];
        p = p_to_p[p];
        if(p > 0)
            p = p + d[0] + d[1] - 2;
    }
}

void cnf::encode_equal_probabilities(){
    map< unsigned, vector<unsigned int> > variable_to_clause;
    for(unsigned int i = 0; i < clause_to_variable.size(); i++)
        variable_to_clause[clause_to_variable[i]].push_back(i);

    vector<probability_t> p_to_w;
    for(unsigned int v = 0; v < VARIABLES; v++){
        vector<unsigned int> &cpt_clause = variable_to_clause[v];

        map< probability_t, vector<int> > probability_to_clause;
        for(unsigned int idx = 0; idx < cpt_clause.size(); idx++){
            unsigned int i = cpt_clause[idx];
            probability_t p = get_weight(i);
            if(p >= 0)
                probability_to_clause[p].push_back(i);
        }
        for(auto it = probability_to_clause.begin(); it != probability_to_clause.end(); it++){
            unsigned int pid = p_to_w.size();
            p_to_w.push_back(it->first);
            for(unsigned int idx = 0; idx < it->second.size(); idx++)
                clause_to_probability[it->second[idx]] = pid;
        }
    }
    probability_to_weight = p_to_w;
}

template <class T>
void cnf::reduce(std::vector< uint32_t > &clauses, std::vector<clause> &nclauses, std::map<uint32_t,uint32_t> &l_to_i, uint32_t p, uint32_t v){
    qm<T> q;

    // variables
    std::map <unsigned int, std::vector <unsigned int> > v_to_l;
    for(auto mlit = l_to_i.begin(); mlit != l_to_i.end(); mlit++){
        q.add_variable(mlit->first, mlit->second);
        v_to_l[literal_to_variable[mlit->first]].push_back(mlit->first);
    }

    // create constraint clauses
    std::vector < cube<T> > constraints;
    for(auto vit = v_to_l.begin(); vit != v_to_l.end(); vit++){
        std::vector <unsigned int> &literals = vit->second;

        if(literals.size() > 1){
            // variables constraint
            {
                constraints.resize(constraints.size()+1);
                cube<T> &m = constraints.back();
                m[0].clear_all();
                m[1].set_lsb(l_to_i.size());
                for(unsigned int i = 0; i < literals.size(); i++)
                    m[1].clear(l_to_i[literals[i]]);
            }

            // mutual exclusive values
            for(unsigned int l1 = 0; l1 < literals.size()-1; l1++){
                for(unsigned int l2 = l1+1; l2 < literals.size(); l2++){
                    constraints.resize(constraints.size()+1);
                    cube<T> &m = constraints.back();
                    T tl1 = l_to_i[literals[l1]];
                    T tl2 = l_to_i[literals[l2]];

                    m[0].clear_all();
                    m[0].set(tl1);
                    m[0].set(tl2);

                    m[1].set_lsb(l_to_i.size());
                    m[1].clear(tl1);
                    m[1].clear(tl2);
                }
            }
        }
    }

    // add constraint clauses
    for(auto it = constraints.begin(); it != constraints.end(); it++)
        q.add_model(*it);

    // models
    for(auto cit = clauses.begin(); cit != clauses.end(); cit++){
        cube<T> model;
        model[0].clear_all();
        model[1].set_lsb(l_to_i.size());
        clause_t &clause = expr.clauses[*cit];
        for(unsigned int i = 0; i < clause.literals.size(); i++){
            unsigned int idx = l_to_i[clause.literals[i]];
            model[1].clear(idx);
            if(signbit(clause.literals[i]))
                model[0].set(idx);
        }
        q.add_model(model);
    }

    //FILE *file = fopen("output","a");
    //fprintf(file,"-v%d -o", q.variables.size());
    //for(auto it = q.models.begin(); it != q.models.end(); it++){
    //    if(it != q.models.begin())
    //        fprintf(file, ",");
    //    fprintf(file,"%lu", *it);
    //}
    //fprintf(file,"\n");
    //fclose(file);

    q.solve();

    // remove constraint clauses
    for(auto it = constraints.begin(); it != constraints.end(); it++)
        q.remove_prime(*it);

    printf("function size: %u -> %u\n", clauses.size(), q.get_primes_size());
    if(clauses.size() < q.get_primes_size())
        fprintf(stderr, "ERROR: nr of clauses increased!!\n");
    //printf("primes %d: ", q.primes.size());
    //for(unsigned int i = 0;i < q.primes.size(); i++)
    //    printf("(%lu,%lu)", q.primes[i][0].value, q.primes[i][1].value);
    //printf("\n");

    // copy primes
    unsigned int offset = nclauses.size();
    nclauses.resize(offset+q.get_primes_size());
    for(unsigned int i = 0; i < q.get_primes_size(); i++){
        std::vector<int32_t> &l = nclauses[offset+i].literals;
        q.get_clause(l, i);
        for(unsigned int j = 0; j < l.size(); j++)
            l[j] = -1*l[j];

        clause_to_probability[offset+i] = p;
        clause_to_variable[offset+i] = v; // FIXME check if memory is not corrupted
    }
}

void cnf::encode_prime(bayesnet *bn){
    encode_probabilities(bn);
    encode_equal_probabilities();
    encode_deterministic_probabilities();


    if(expr.clauses.size() > 0){
        qm_variable_count.clear();
        qm_eligible = 0;
        qm_possible = 0;
        unsigned int offset = 0;
        std::vector<clause> nclauses;

        for(unsigned int v = 0; v < VARIABLES; v++){
            unsigned int CLAUSES = bn->get_states(v);
            for(unsigned int p = 0; p < bn->get_parent_size(v); p++)
                CLAUSES *= bn->get_states(bn->get_parent(v)[p]);

            map< uint32_t, vector<uint32_t> > probability_to_clause;
            for(unsigned int c = 0; c < CLAUSES; c++)
                probability_to_clause[clause_to_probability[offset+c]].push_back(offset+c);

            for(auto mit = probability_to_clause.begin(); mit != probability_to_clause.end(); mit++){
                vector<uint32_t> &clauses = mit->second;
                map <uint32_t, uint32_t> l_to_i;
                for(auto cit = clauses.begin(); cit != clauses.end(); cit++){
                    clause_t &clause = expr.clauses[*cit];
                    for(auto lit = clause.literals.begin(); lit != clause.literals.end(); lit++){
                        if(l_to_i.find(*lit) == l_to_i.end())
                            l_to_i[*lit] = l_to_i.size()-1;
                    }
                }
                printf("(%u/%u) probability: %-3d variables: %-3d clauses: %-3d\n", v, VARIABLES, mit->first, l_to_i.size(), mit->second.size());
                // stats ----
                if(clauses.size() > 1){
                    dynamic_assign(qm_variable_count, l_to_i.size())++;
                    qm_possible++;
                    // FIXME: we don't attempt quine-mccluskey when nr of variables is too big
                    #if __LP64__
                    if((QM_LIMIT > 0 && l_to_i.size() > QM_LIMIT) || l_to_i.size() > 128){
                    #else
                    if((QM_LIMIT > 0 && l_to_i.size() > QM_LIMIT) || l_to_i.size() > 64){
                    #endif
                        printf("  SKIPPED\n");
                    //if(l_to_i.size() > 32){
                        unsigned int offset = nclauses.size();
                        nclauses.resize(offset+clauses.size());
                        for(unsigned int i = 0; i < clauses.size(); i++)
                            nclauses[offset+i] = expr.clauses[clauses[i]];
                    } else {
                        qm_eligible++;
                        if(l_to_i.size() <= 32)
                            reduce<uint32_t>(clauses, nclauses, l_to_i, mit->first, v);
                        else if(l_to_i.size() <= 64)
                            reduce<uint64_t>(clauses, nclauses, l_to_i, mit->first, v);
                        #if __LP64__
                        else if(l_to_i.size() <= 128)
                            reduce<uint128_t>(clauses, nclauses, l_to_i, mit->first, v);
                        #endif
                    }
                } else if(clauses.size() == 1) {
                    unsigned int idx = nclauses.size();
                    nclauses.resize(idx+1);
                    nclauses[idx] = expr.clauses[clauses[0]];
                    clause_to_probability[idx] = mit->first;
                    clause_to_variable[idx] = v;
                }
            }
            offset += CLAUSES;
        }
        clause_to_variable.resize(nclauses.size());
        clause_to_probability.resize(nclauses.size());
        expr.clauses = nclauses;

        //fix_literal_implications();
    }
}

void cnf::set_filename(char* name){
    filename = strdup(name);
}

void cnf::set_qm_limit(int max){
    QM_LIMIT = max;
}

void cnf::set_optimization(opt_t opt){
    switch(opt){
        case EQUAL_PROBABILITIES:         OPT_EQUAL_PROBABILITIES = true;         break;
        case DETERMINISM:                 OPT_DETERMINISM = true; // note absence of break...
        case DETERMINISTIC_PROBABILITIES: OPT_DETERMINISTIC_PROBABILITIES = true; break;
        case SYMPLIFY:                    OPT_SYMPLIFY = true;                    break;
        case QUINE_MCCLUSKEY:             OPT_QUINE_MCCLUSKEY = true;             break;
        case BOOL:                        OPT_BOOL = true;                        break;
        case SUPRESS_CONSTRAINTS:
            OPT_SUPRESS_CONSTRAINTS = true;
            set_encoding(1);
            break;
        default:
            break;
    }

}

void cnf::set_encoding(int encoding){
    this->encoding = encoding;
}

void cnf::fix_literal_implications(){
    expr.literals.clear();
    expr.literals.resize(LITERALS+1);
    for(unsigned int c = 0; c < expr.clauses.size(); c++){
        clause_t &clause = expr.clauses[c];
        for(unsigned int i = 0; i < clause.literals.size(); i++){
            literal_t &literal = expr.literals[clause.literals[i]];
            unsigned int s = literal.clauses.size();
            literal.clauses.resize(s+1);
            literal.negated.resize(s+1);
            literal.clauses[s] = c;
            literal.negated[s] = signbit(clause.literals[i]);
        }
    }
}

inline probability_t cnf::get_weight(unsigned int i){
    int32_t p = clause_to_probability[i];
    return (p<0?p:probability_to_weight[p]);
}

void cnf::print(){
    for(unsigned int v = 0; v < VARIABLES; v++){
        printf("%u>", v);
        if(OPT_BOOL && values[v] == 2){
            printf("%u\n", variable_to_literal[v]);
        } else {
            for(unsigned int l = 0; l < values[v]; l++){
                if(l > 0)
                    printf(",");
                printf("%u", variable_to_literal[v]+l);
            }
            printf("\n");
        }
    }

    for(unsigned int i = 0; i < expr.clauses.size(); i++){
        printf(" %3u: p%-4d v%-4u %f: ", i, clause_to_probability[i], clause_to_variable[i], get_weight(i));
        for(unsigned int j = 0; j < expr.clauses[i].literals.size(); j++){
            bool pauze = !signbit(expr.clauses[i].literals[j]);
            if(pauze) printf(" ");
            if(!expr.clauses[i].sat[j]){
                if(pauze) printf("%-3d ", expr.clauses[i].literals[j]);
                else printf("%-4d ", expr.clauses[i].literals[j]);

            }
        }
        printf("\n");
    }
}

//void cnf::print(){
//    for(unsigned int i = 0; i < expr.clauses.size(); i++)
//       printf("%3u (v%-3u) p%3u %f\n",i, clause_to_variable[i], clause_to_probability[i], probability_to_weight[clause_to_probability[i]]);
//}


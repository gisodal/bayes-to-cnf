#include "cnf.h"
#include "misc.h"
#include <quine-mccluskey/qm.h>
#include "bayesnet.h"
#include <stack>
#include <array>
#include <string.h>
#include <string>
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

expression::expression(){
    clear();
}

expression& expression::operator=(const expression &e){
    clauses = e.clauses;
    LITERALS = e.LITERALS;
    WEIGHTS = e.WEIGHTS;
    mapped = e.mapped;
    values = e.values;
    literal_to_variable = literal_to_variable;
    variable_to_literal = variable_to_literal;
    clause_to_variable = clause_to_variable;
    literal_to_literal_map = literal_to_literal_map;
    weight_to_weight_map = weight_to_weight_map;
    variable_to_variable_map = variable_to_variable_map;
    return *this;
}

void expression::clear(){
    LITERALS = 0;
    WEIGHTS = 0;
    mapped = false;

    values.clear();
    literal_to_literal_map.clear();
    weight_to_weight_map.clear();
    variable_to_variable_map.clear();
    variable_to_literal.clear();
    clause_to_variable.clear();
}

const std::vector<literal_t>& expression::get_literal_map(){
    return literal_to_literal_map;
}

const std::vector<weight_t>& expression::get_weight_map(){
    return weight_to_weight_map;
}

const std::vector<unsigned int>& expression::get_variable_map(){
    return variable_to_variable_map;
}

const std::vector<uint32_t>& expression::get_values() const {
    return values;
}

const std::vector<uint32_t>& expression::get_clause_to_variable() const {
    return clause_to_variable;
}

const std::vector<uint32_t>& expression::get_variable_to_literal() const {
    return variable_to_literal;
}

const std::vector<uint32_t>& expression::get_literal_to_variable() const {
    return literal_to_variable;
}

unsigned int expression::get_nr_literals() const {
    return LITERALS;
}

unsigned int expression::get_nr_weights() const {
    return WEIGHTS;
}

unsigned int expression::get_nr_variables() const {
    return values.size();
}

unsigned int expression::get_nr_clauses() const {
    return clauses.size();
}

bool expression::is_mapped(){
    return mapped;
}

void expression::map_literals(){
    mapped = true;
    std::set<uliteral_t> literals;
    std::set<uliteral_t> weights;
    for(auto it = clauses.begin(); it != clauses.end(); it++){
        clause_t &c = *it;
        if(c.w >= 0)
            weights.insert(c.w);
        for(auto lit = c.literals.begin(); lit != c.literals.end(); lit++){
           uliteral_t l = abs(*lit);
           literals.insert(l);
        }
    }
    literal_to_literal_map.resize(literals.size()+1);
    weight_to_weight_map.resize(weights.size());

    std::map<weight_t,weight_t> w_to_w;
    std::map<literal_t,literal_t> l_to_l;
    literal_to_literal_map[0] = 0;
    unsigned int i = 1;
    for(auto it = literals.begin(); it != literals.end(); it++){
        literal_to_literal_map[i] = *it;
        l_to_l[*it] = i;
        i++;
    }

    i = 0;
    for(auto it = weights.begin(); it != weights.end(); it++){
        weight_to_weight_map[i] = *it;
        w_to_w[*it] = i;
        i++;
    }

    for(auto it = clauses.begin(); it != clauses.end(); it++){
        clause_t &c = *it;
        if(c.w >= 0)
            c.w = w_to_w[c.w];

        for(auto lit = c.literals.begin(); lit != c.literals.end(); lit++){
            literal_t &l = *lit;
            if(l < 0)
                l = -1 * l_to_l[abs(l)];
            else
                l = l_to_l[l];
        }
    }
}

cnf::cnf(){
    bn = NULL;
    filename = NULL;
    clear();
}

cnf::~cnf(){
    if(filename){
        free(filename);
        filename = NULL;
    }
}

const std::vector<probability_t>& cnf::get_probability_to_weight() const {
    return weight_to_probability;
}

unsigned int cnf::get_nr_literals() const {
    return expr.LITERALS;
}

unsigned int cnf::get_nr_weights() const {
    return expr.WEIGHTS;
}

inline uint32_t cnf::v_to_l(uint32_t variable, uint32_t value){
    return expr.variable_to_literal[variable]+value;
}

probability_t cnf::get_probability(weight_t w){
    return (w<0?w:weight_to_probability[w]);
}

probability_t cnf::get_probability(unsigned int i){
    weight_t w = expr.clauses[i].w;
    return get_probability(w);
}

probability_t cnf::get_probability(unsigned int i, expression &expr){
    if(expr.is_mapped()){
        weight_t w = expr.clauses[i].w;
        if(w < 0)
            return -1;
        w = expr.weight_to_weight_map[w];
        return get_probability(w);
    } else return get_probability(i);
}

void cnf::clear(){
    expr.clear();

    weight_to_probability.clear();

    CONSTRAINTS = 0;
    VARIABLES = 0;
    OPT_SUPPRESS_CONSTRAINTS = false;
    OPT_EQUAL_PROBABILITIES = false;
    OPT_PARTITION = false;
    OPT_DETERMINISTIC_PROBABILITIES = false;
    OPT_SYMPLIFY = false;
    OPT_QUINE_MCCLUSKEY = false;
    OPT_BOOL = false;
    expr.clauses.clear();
    qm_eligible = 0;
    qm_possible = 0;
    qm_variable_count.clear();
    QM_LIMIT = -1;
    encoding = 0; // default encoding containing constraints
    this->~cnf();
}


// int cnf::read(char *name){
//     FILE *file = fopen(name,"r");
//     if(file){
//         char buf[256];
//         unsigned int probabilities = 0;
//         while(fgets(buf, sizeof(buf), file) != NULL){
//             unsigned int v, x = 0;
//             char c = '\0';
//             sscanf(buf, "%u%c", &v, &c);
//             VARIABLES = (v >= VARIABLES?v+1:VARIABLES);
//
//             if(c == '>')
//                 x = 1;
//             else if(c == ':')
//                 x = 2;
//
//             if(x > 0){
//                 char *str = strchr(buf, '>');
//                 if(str == NULL)
//                     str = strchr(buf, ':')+1;
//                 else
//                     str += 1;
//
//                 char *token = strsep(&str, ":");
//                 char *str2, *fstr2;
//                 fstr2 = str2 = strdup(token);
//                 if(x == 2){
//                     char *token = strsep(&str, ":");
//                     unsigned int ctop;
//                     sscanf(token, "%u",&ctop);
//
//                     expr.clause_to_weight.push_back(ctop);
//
//                     float p;
//                     token = strsep(&str, ":");
//                     sscanf(token, "%f",&p);
//                     if(weight_to_probability.size() <= ctop)
//                         weight_to_probability.resize(ctop+1);
//                     weight_to_probability[ctop] = p;
//
//                     expr.clause_to_variable.push_back(v);
//                 }
//
//                 if(x == 2)
//                     expr.clauses.resize(expr.clauses.size()+1);
//
//                 unsigned int count = 0;
//                 token = strsep(&str2, ",");
//                 while( token != NULL && token[0] != '\0'){
//                     literal_t l;
//                     sscanf(token, "%d", &l);
//
//                     expr.LITERALS = (l >= expr.LITERALS?l+1:expr.LITERALS);
//
//                     if(x == 2)
//                         expr.clauses.back().literals.push_back(l);
//
//                     if(expr.literal_to_variable.size() <= l)
//                         expr.literal_to_variable.resize(l+1);
//                     expr.literal_to_variable[l] = v;
//
//                     count++;
//                     token = strsep(&str2, ",");
//                 }
//
//                 if(x == 1){
//                     if(expr.values.size() <= v)
//                         expr.values.resize(v+1);
//                     expr.values[v] = count;
//                 }
//                 free(fstr2);
//             }
//         }
//         //fix_literal_implications(); FIXME: literals to clause "pointers" not implemented
//
//         expr.variable_to_literal.assign(VARIABLES,~0u);
//         for(unsigned int i = 0; i < expr.literal_to_variable.size(); i++)
//             if(expr.variable_to_literal[expr.literal_to_variable[i]] > i)
//                 expr.variable_to_literal[expr.literal_to_variable[i]] = i;
//
//         fclose(file);
//     } else fprintf(stderr, "Could not open file '%s'\n", name);
//     return 0;
// }

int cnf::write(const char* outfile, int i){
    expression_t *tmp;
    if(i < 0)
        tmp = &expr;
    else tmp = &(exprs[i]);
    expression &expr = *tmp;

    FILE *file = fopen(outfile,"w");
    if(file){
        fprintf(file, "c DIMACS CNF Format\n");
        fprintf(file, "c\n");
        stats(file, &expr);
        fprintf(file, "c\n");
        if(i >= 0){
            fprintf(file, "c CNF representation of variable '%s'\n", bn->get_node_name(i).c_str());
            fprintf(file, "c\n");
        }
        fprintf(file, "c ===================================================\n");
        unsigned int counter = 0;
        for(unsigned int i = 0; i < expr.clauses.size(); i++){
            probability_t p = get_probability(i,expr);
            if(!OPT_SYMPLIFY || p != 1)
                counter++;
        }

        fprintf(file, "p cnf %u %u\n", expr.LITERALS+expr.WEIGHTS, counter);
        for(unsigned int i = 0; i < expr.clauses.size(); i++){
            if(!OPT_SYMPLIFY || get_probability(i,expr) != 1){
                clause &c = expr.clauses[i];
                for(unsigned int j = 0; j < c.literals.size(); j++)
                    fprintf(file, "%d ", c.literals[j]);

                if(get_probability(i,expr) != -1 && (!OPT_SYMPLIFY || get_probability(i,expr) != 0))
                    fprintf(file, "%u ", expr.LITERALS+1+c.w);

                fprintf(file,"0\n");
            }
        }

        fprintf(file, "c ===================================================\n");
        //fprintf(file, "c clauses       : %-6u\n", expr.clauses.size());
        //fprintf(file, "c literals      : %-6u (1-%u)\n", expr.LITERALS,expr.LITERALS);
        //fprintf(file, "c probabilities : %-6u (%u-%u)\n", weight_to_probability.size(), expr.LITERALS+1, expr.LITERALS+weight_to_probability.size());
        fprintf(file, "c\n");
        fprintf(file, "c literal-to-real-weight mapping:\n");
        fprintf(file, "c     1-%u = 1\n",expr.LITERALS);

        if(expr.is_mapped()){
            for(unsigned int i = 0; i < expr.weight_to_weight_map.size(); i++)
                fprintf(file, "c     %u = %f\n", expr.LITERALS+1+i, get_probability(expr.weight_to_weight_map[i]));
        } else {
            for(unsigned int i = 0; i < weight_to_probability.size(); i++)
                fprintf(file, "c     %u = %f\n", expr.LITERALS+1+i, weight_to_probability[i]);
        }

        fprintf(file, "c\nc variable-to-literal mapping:\n");
        for(unsigned int v = 0; v < expr.get_nr_variables(); v++){
            fprintf(file, "c     %u = {",v);
            for(unsigned int l = 0; l < expr.values[v]; l++){
                if(l > 0) fprintf(file,",");
                fprintf(file, "%u",expr.variable_to_literal[v]+l);
                if(OPT_BOOL && expr.values[v] == 2)
                    break;
            }
            fprintf(file,"}\n");
        }

        if(bn){
            fprintf(file, "c\nc variable-and-values-to-names mapping:\n");
            fprintf(file, "c     <variable> <nr of values> <variable name>\n");
            fprintf(file, "c          <value literal> <value name>\n");
            fprintf(file, "c         [<value literal> <value name>]\nc\n");
            for(unsigned int v = 0; v < expr.get_nr_variables(); v++){
                unsigned int old_variable = v;
                if(expr.is_mapped())
                    old_variable = expr.variable_to_variable_map[v];
                fprintf(file, "c     %u %u \"%s\"\n", v, expr.values[v], bn->get_node_name(old_variable).c_str());
                for(unsigned int l = 0; l < expr.values[v]; l++)
                    fprintf(file, "c         %u \"%s\"\n", expr.variable_to_literal[v]+l, bn->get_node_value_name(old_variable,l).c_str());
            }
        }
        fclose(file);

    } else fprintf(stderr, "Could not open file '%s'\n", filename);
    return 0;

}

int cnf::write(const char *extra){
    string prefix = bn->get_filename();
    size_t found = prefix.find_last_of(".");
    prefix = prefix.substr(0,found);
    string name = prefix;
    if(extra){
        name += ".";
        name += extra;
    }
    name += ".cnf";

    if(write(name.c_str(),-1) == 0)
        printf("\nDIMACS CNF written to: %s\n\n", name.c_str());
    else {
        return 1;
        printf("Could not write to: %s\n\n", name.c_str());
    }

    if(exprs.size() > 0 && OPT_PARTITION) {
        for(unsigned int v = 0; v < exprs.size(); v++){
                name = prefix + ".";
            if(extra)
                 name += extra;
            name += "." + to_string(v) + ".cnf";
            if(write(name.c_str(), v) != 0){
                printf("Could not write to: %s\n\n", name.c_str());
                return 1;
            }
        }
        printf("Partitioned DIMACS CNF written to: %s.*.cnf\n\n", prefix.c_str());
    }
    return 0;
}

void cnf::stats(FILE *file, expression_t* e){
    if(!e)
        e = &expr;

    char prefix[10] = {0};
    if(file != stdout)
        strcpy(prefix, "c     ");

    fprintf(file,"%sOptimizations:\n",prefix);
    fprintf(file,"%s    Suppress constraints : %s\n", prefix, (OPT_SUPPRESS_CONSTRAINTS?"YES":"NO") );
    fprintf(file,"%s    Boolean recognition  : %s\n", prefix, (OPT_BOOL?"YES":"NO"));
    fprintf(file,"%s    Equal probabilities  : %s\n", prefix, (OPT_EQUAL_PROBABILITIES?"YES":"NO") );
    fprintf(file,"%s    Partition per CPT    : %s\n", prefix, (OPT_PARTITION?"YES":"NO") );
    fprintf(file,"%s    Quine-McCluskey      : %s  ", prefix, (OPT_QUINE_MCCLUSKEY?"YES":"NO"));
    if(QM_LIMIT >= 0)
        fprintf(file," (limit %d)", QM_LIMIT);
    fprintf(file,"\n");
    fprintf(file,"%s    Determinism          : %s\n", prefix, (OPT_DETERMINISTIC_PROBABILITIES?"YES":"NO") );
    fprintf(file,"%s    Simplify             : %s\n", prefix, (OPT_SYMPLIFY?"YES":"NO") );

    fprintf(file,"%s\n",prefix,prefix);
    fprintf(file,"%sVariables       : %d\n", prefix, VARIABLES);
    fprintf(file,"%sProbabilities   : %d\n", prefix, e->WEIGHTS);
    fprintf(file,"%sLiterals        : %d\n", prefix, e->LITERALS);
    fprintf(file,"%sClauses         : %d\n", prefix, e->clauses.size());

    int max = 0;
    int min = -1;
    unsigned long int total = 0;
    for(auto it = e->clauses.begin(); it != e->clauses.end(); it++){
        int size = it->literals.size();
        total += size;
        if(size > max)
            max = size;
        if(min < 0 || size < min)
            min = size;
    }

    vector<uint32_t> sizes(max+1,0);
    for(auto it = e->clauses.begin(); it != e->clauses.end(); it++)
        sizes[it->literals.size()]++;

    fprintf(file,"%sLiteral/clauses : %.2f \n", prefix, (float) total/e->clauses.size());
    fprintf(file,"%sClause sizes    : %d-%d\n", prefix, min, max);
    //printf("clauses/size    : ");
    //for(unsigned int i = 1; i <= max; i++)
    //    printf("%5d ", sizes[i]);
    //printf("\n                  ");
    //for(unsigned int i = 1; i <= max; i++)
    //    printf("%5d ", i);
    //printf("\n\n");

    //printf("#file,encoding,bool,qm_limit,variables,probabilities,literals,clauses,literal/clauses,clause_min_size,clause_max_size");
    //printf("\n");

    //printf("%s,",filename);
    //printf("%d,",encoding);
    //printf("%d,", OPT_BOOL);
    //printf("%d,", QM_LIMIT);
    //printf("X%d,", VARIABLES);
    //printf("P%d,", weight_to_probability.size());
    //printf("L%d,", e->LITERALS);
    //printf("C%d,", e->clauses.size());
    //printf("LC%.4f,", (float) total/e->clauses.size());
    //printf("%d,%d,", min, max);
    //printf("\n");
}

void cnf::apply_optimization(){
    if(OPT_EQUAL_PROBABILITIES)
        encode_equal_probabilities();

    if(OPT_DETERMINISTIC_PROBABILITIES)
        encode_deterministic_probabilities();

    if(OPT_SYMPLIFY)
        encode_determinism();
}

int cnf::encode(bayesnet *bn){
    this->bn = bn;
    if(encoding < 0 || !bn)
        return -1;

    VARIABLES = bn->size;
    expr.values.resize(VARIABLES);
    memcpy(&expr.values.front(), bn->states,sizeof(uint32_t)*VARIABLES);

    expr.LITERALS = 0;
    for(unsigned int i = 0; i < VARIABLES; i++){
        if(OPT_BOOL && bn->states[i] == 2)
            expr.LITERALS += 1;
        else
            expr.LITERALS += bn->states[i];
    }

    expr.variable_to_literal.resize(VARIABLES);
    expr.literal_to_variable.resize(expr.LITERALS+1);
    unsigned int l = 1;
    for(unsigned int i = 0; i < VARIABLES; i++){
        expr.variable_to_literal[i] = l;
        if(OPT_BOOL && expr.values[i] == 2){
            expr.literal_to_variable[l] = i;
            l += 1;
        } else {
            for (unsigned int v=0; v < expr.values[i]; v++)
                expr.literal_to_variable[l+v] = i;
            l += expr.values[i];
        }
    }

    switch(encoding){
        case 0:
            encode_constraints(); // note missing break;
        case 1:
            encode_probabilities();
            break;
        default:
            clear();
            return 1;
    }

    apply_optimization();

    if(OPT_QUINE_MCCLUSKEY){
        encode_prime();
        if(!OPT_SUPPRESS_CONSTRAINTS){
            set_encoding(0);
            encode_constraints();
            apply_optimization();
        }
    }

    if(OPT_PARTITION)
        encode_partitions();

    if(expr.clauses.size() == 0)
        clear();



    return 0;
}

void cnf::init(){
}

void cnf::encode_partitions(){
    exprs.resize(VARIABLES);
    std::vector< std::vector<unsigned int> > variable_to_constraints;
    variable_to_constraints.resize(VARIABLES);

    // identify constraint clauses
    if(!OPT_SUPPRESS_CONSTRAINTS){
        for(unsigned int i = 0; i < CONSTRAINTS; i++){
            unsigned int v = expr.clause_to_variable[i];
            variable_to_constraints[v].push_back(i);
        }
    } else CONSTRAINTS = 0;

    // add constraint clauses
    if(!OPT_SUPPRESS_CONSTRAINTS){
        for(unsigned v = 0; v < VARIABLES; v++){
            // add constraints for variable v
            for(unsigned int i = 0; i < variable_to_constraints[v].size(); i++){
                unsigned int c = variable_to_constraints[v][i];
                exprs[v].clauses.push_back(expr.clauses[c]);
            }
            // add constraint clauses for the parents of v
            uint32_t* parents = bn->get_parent(v);
            for(unsigned int i = 0; i < bn->get_parent_size(v); i++){
                unsigned int vv = parents[i];
                for(unsigned int i = 0; i < variable_to_constraints[vv].size(); i++){
                    unsigned int c = variable_to_constraints[vv][i];
                    exprs[v].clauses.push_back(expr.clauses[c]);
                }
            }
        }
    }

    // divide CPT clauses per variable
    for(unsigned int i = CONSTRAINTS; i < expr.clauses.size(); i++){
        unsigned int v = expr.clause_to_variable[i];
        exprs[v].clauses.push_back(expr.clauses[i]);
    }

    // set other variables of expression
    for(unsigned int i = 0; i < VARIABLES; i++){
        expression &e = exprs[i];
        e.map_literals();
        e.LITERALS = e.literal_to_literal_map.size()-1;
        e.WEIGHTS = e.weight_to_weight_map.size();

        std::map<unsigned int, unsigned int> variables;
        e.literal_to_variable.resize(e.LITERALS+1);
        unsigned int variable = 0;
        for(unsigned int l = 1; l <= e.LITERALS; l++){
            literal_t old_l = e.literal_to_literal_map[l];
            unsigned int old_variable = expr.literal_to_variable[old_l];
            unsigned int old_values = expr.values[old_variable];

            auto hit = variables.find(old_variable);
            unsigned int new_variable = variable;
            if(hit == variables.end())
                variables[old_variable] = variable++;
            else new_variable = hit->second;

            e.values.resize(variables.size());
            e.values[new_variable] = old_values;

            e.variable_to_variable_map.resize(variables.size());
            e.variable_to_variable_map[new_variable] = old_variable;

            e.literal_to_variable[l] = new_variable;
        }
        e.variable_to_literal.resize(e.get_nr_variables());
        e.variable_to_literal[0] = 1;
        for(unsigned int i = 1; i < e.values.size(); i++)
            e.variable_to_literal[i] = e.variable_to_literal[i-1] + e.values[i-1];
    }
}

void expression::print(){
    printf(" =================================================\n");
    for(unsigned int i = 0; i < clauses.size(); i++){
        printf(" %3u: p%-4d ", i, clauses[i].w);
        if(clauses[i].w == -1)
            printf(" NONE ");
        else printf("    ?: ");

        for(unsigned int j = 0; j < clauses[i].literals.size(); j++){
            bool pauze = !signbit(clauses[i].literals[j]);
            if(pauze)
                printf(" ");
            if(pauze)
                printf("%-3d ", clauses[i].literals[j]);
            else
                printf("%-4d ", clauses[i].literals[j]);

        }
        printf("\n");
    }
    if(is_mapped()){
        printf("\n");
        printf("literal map: \n");
        for(unsigned int i = 0; i < literal_to_literal_map.size(); i++)
            printf("  %d -> %d\n", i, literal_to_literal_map[i]);

        printf("\nweight map: \n");
        for(unsigned int i = 0; i < weight_to_weight_map.size(); i++)
            printf("  %d -> %d\n", i, weight_to_weight_map[i]);

        printf("\nvariable map: \n");
        for(unsigned int i = 0; i < variable_to_variable_map.size(); i++)
            printf("  %d -> %d\n", i, variable_to_variable_map[i]);

        printf("\nvalues:\n");
        for(unsigned int i = 0; i < values.size(); i++)
           printf("   %d : %d\n", i, values[i]);
        printf(" array_t literal_to_variable;\n");
        for(unsigned int i = 1; i < literal_to_variable.size(); i++)
           printf("   %d : %d\n", i, literal_to_variable[i]);
        printf(" array_t clause_to_variable;;\n");
        for(unsigned int i = 0; i < clause_to_variable.size(); i++)
           printf("   %d : %d\n", i, clause_to_variable[i]);

        printf("\n");


    }
}

void cnf::encode_constraints(){
    //expr.literals.resize(expr.LITERALS);

    // variable encoding
    for(unsigned int i = 0; i < bn->size; i++){
        if(OPT_BOOL && bn->states[i] == 2)
            continue;

        uint32_t cid = expr.clauses.size();
        expr.clauses.resize(cid+1);
        clause_t &c = expr.clauses.back();
        expr.clause_to_variable.push_back(i);
        //c.w = -1; // expr.clause_to_weight.push_back(-1);
        for(unsigned int v = 0; v < bn->states[i]; v++)
            c.literals.push_back(v_to_l(i,v));
        CONSTRAINTS++;
    }

    // constraint encoding
    for(unsigned int i = 0; i < bn->size; i++){
        if(OPT_BOOL && bn->states[i] == 2)
            continue;

        unsigned int values = bn->states[i];
        uint32_t literal_base = v_to_l(i,0);
        for(unsigned int v = 0; v < values-1; v++){
            for(unsigned int vv = v+1; vv < values; vv++){
                expr.clauses.resize(expr.clauses.size()+1);
                clause &c = expr.clauses.back();
                expr.clause_to_variable.push_back(i);
                //c.w = -1; // clause_to_weight.push_back(-1);

                c.literals.push_back(-1*(literal_base+v));
                c.literals.push_back(-1*(literal_base+vv));
                CONSTRAINTS++;
            }
        }
    }
}

bayesnet* cnf::get_bayesnet() const {
    return bn;
}

unsigned int cnf::get_nr_variables() const {
    return VARIABLES;
}

const uint32_t* cnf::get_states() const {
    return bn->states;
}

void cnf::encode_probabilities(){
    //expr.literals.resize(expr.LITERALS+1);

    // weight encoding
    for(unsigned int i = 0; i < bn->size; i++){
        unsigned int m = bn->get_parent_size(i);
        int *max = (int*) malloc(sizeof(int)*(m+1));
        int *ctr = (int*) malloc(sizeof(int)*(m+1));
        int *variable = (int*) malloc(sizeof(int)*(m+1));
        for(unsigned int j = 0; j < m; j++){
            variable[j] = bn->get_parent(i)[j];
            max[j] = expr.values[variable[j]];
            ctr[j] = 0;
        }
        variable[m] = i;
        max[m] = expr.values[i];
        ctr[m] = 0;

        int q = 0;
        while(true){
            expr.clause_to_variable.push_back(i);

            uint32_t cid = expr.clauses.size();
            expr.clauses.resize(cid+1);
            clause_t &c = expr.clauses.back();

            probability_t p = bn->cpt[bn->cpt_offset[i]+q++];
            //if(p != 0 && p != 1){
                c.w = weight_to_probability.size(); //clause_to_weight.push_back(weight_to_probability.size());
                weight_to_probability.push_back(p);
                expr.WEIGHTS = weight_to_probability.size();
            //} else clause_to_weight.push_back((uint32_t) p);

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
                        for(unsigned int r = q+1; r <= m; r++)
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
    // 1. remove weights that correspond to probability 0
    vector <probability_t> w_to_p;
    map< int, int > w_to_w;
    w_to_w[-1] = -1;
    for(unsigned int i = 0; i < expr.clauses.size(); i++){
        weight_t &w = expr.clauses[i].w;
        probability_t p = get_probability(i);
        if(p == 0)
            w = -1;
        else if(w_to_w.find(w) == w_to_w.end()){
            w_to_w[w] = w_to_p.size();
            w_to_p.push_back(p);
        }
    }
    weight_to_probability = w_to_p;
    for(unsigned int i = 0; i < expr.clauses.size(); i++)
        expr.clauses[i].w = w_to_w[expr.clauses[i].w];

    // 2. remove all clauses with probability 1
    w_to_w.clear(); w_to_p.clear();
    w_to_w[-1] = -1;
    for(unsigned int i = 0; i < expr.clauses.size(); i++){
        weight_t w = expr.clauses[i].w;
        probability_t p = get_probability(i);
        if(p != 1 && w_to_w.find(w) == w_to_w.end()){
            w_to_w[w] = w_to_p.size();
            w_to_p.push_back(p);
        }
    }
    for(unsigned int i = 0; i < expr.clauses.size(); i++){
        if(get_probability(i) == 1){
            expr.clauses.erase(expr.clauses.begin()+i);
            expr.clause_to_variable.erase(expr.clause_to_variable.begin()+i);
            i--;
        } else expr.clauses[i].w = w_to_w[expr.clauses[i].w];
    }
    weight_to_probability = w_to_p;
    expr.WEIGHTS = weight_to_probability.size();
}


void cnf::encode_deterministic_probabilities(){
    vector <probability_t> w_to_p;
    w_to_p.push_back(0);
    w_to_p.push_back(1);

    int d[2] = { 0 };
    map< int, int > w_to_w;
    w_to_w[-1] = -1;
    for(unsigned int i = 0; i < expr.clauses.size(); i++){
        weight_t w = expr.clauses[i].w;
        probability_t p = get_probability(i);

        if(p == 0 || p == 1){
            w_to_w[w] = (weight_t) p;
            d[(weight_t) p] = 1;
        } else if(w_to_w.find(w) == w_to_w.end()){
            w_to_w[w] = w_to_p.size();
            w_to_p.push_back(p);
        }
    }

    if(!d[1]) w_to_p.erase(w_to_p.begin()+1);
    if(!d[0]) w_to_p.erase(w_to_p.begin());
    weight_to_probability = w_to_p;
    for(unsigned int i = 0; i < expr.clauses.size(); i++){
        weight_t &w = expr.clauses[i].w;
        w = w_to_w[w];
        if(w > 0)
            w = w + d[0] + d[1] - 2;
    }
    expr.WEIGHTS = weight_to_probability.size();
}

void cnf::encode_equal_probabilities(){
    map< unsigned, vector<unsigned int> > variable_to_clause;
    for(unsigned int i = 0; i < expr.clause_to_variable.size(); i++)
        variable_to_clause[expr.clause_to_variable[i]].push_back(i);

    vector<probability_t> p_to_w;
    for(unsigned int v = 0; v < VARIABLES; v++){
        vector<unsigned int> &cpt_clause = variable_to_clause[v];

        map< probability_t, vector<int> > probability_to_clause;
        for(unsigned int idx = 0; idx < cpt_clause.size(); idx++){
            unsigned int i = cpt_clause[idx];
            probability_t p = get_probability(i);
            if(p >= 0)
                probability_to_clause[p].push_back(i);
        }
        for(auto it = probability_to_clause.begin(); it != probability_to_clause.end(); it++){
            unsigned int pid = p_to_w.size();
            p_to_w.push_back(it->first);
            for(unsigned int idx = 0; idx < it->second.size(); idx++)
                expr.clauses[it->second[idx]].w = pid;
        }
    }
    weight_to_probability = p_to_w;
    expr.WEIGHTS = weight_to_probability.size();
}

template <class T>
void cnf::reduce(std::vector< uint32_t > &clauses, std::vector<clause> &nclauses, std::map<uint32_t,uint32_t> &l_to_i){
    qm<T> q;

    // craeate variables to literal mapping
    std::map <unsigned int, std::vector <unsigned int> > v_to_l;
    for(auto mlit = l_to_i.begin(); mlit != l_to_i.end(); mlit++){
        q.add_variable(mlit->first, mlit->second);
        v_to_l[expr.literal_to_variable[mlit->first]].push_back(mlit->first);
    }

    // create constraint clauses
    std::vector < cube<T> > constraints;
    for(auto vit = v_to_l.begin(); vit != v_to_l.end(); vit++){
        std::vector <unsigned int> &literals = vit->second;

        if(literals.size() > 1){
            // variable clauses
            {
                constraints.resize(constraints.size()+1);
                cube<T> &m = constraints.back();
                m[0].clear_all();
                m[1].set_lsb(l_to_i.size());
                for(unsigned int i = 0; i < literals.size(); i++)
                    m[1].clear(l_to_i[literals[i]]);
            }

            // constraint clauses
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
            unsigned int idx = l_to_i[abs(clause.literals[i])];
            model[1].clear(idx);
            if(signbit(clause.literals[i]))
                model[0].set(idx);
        }
        q.add_model(model);
    }

    // perform Quine-McCluskey
    q.solve();

    // remove constraint clauses
    // ASSUMPTION: constraint clauses haven't been added before calling this function
    for(auto it = constraints.begin(); it != constraints.end(); it++)
        q.remove_prime(*it);

    printf("    #clauses reduced from %d to %d!\n", clauses.size(), q.get_primes_size());
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
    }
}

void cnf::encode_prime(){
    if(expr.clauses.size() > 0){
        qm_variable_count.clear();
        qm_eligible = 0;
        qm_possible = 0;
        std::vector<weight_t> clause_to_weight;
        for(unsigned int i = 0; i < expr.clauses.size(); i++)
            clause_to_weight.push_back(expr.clauses[i].w);

        std::vector<clause> nclauses;
        std::vector<weight_t> nclause_to_weight;
        std::vector<uint32_t> nclause_to_variable;

        map<unsigned int, vector<uint32_t> > variable_to_clause;
        for(unsigned int c = 0; c < expr.clause_to_variable.size(); c++)
            variable_to_clause[expr.clause_to_variable[c]].push_back(c);

        for(unsigned int v = 0; v < VARIABLES; v++){

            // per variable, group clauses with equal symbolic probability
            map< int, vector<uint32_t> > weight_to_clause;
            for(unsigned int c = 0; c < variable_to_clause[v].size(); c++){
                unsigned int clausenr = variable_to_clause[v][c];
                weight_to_clause[clause_to_weight[clausenr]].push_back(clausenr);
            }

            // per group of clauses apply quine-mccluskey
            for(auto mit = weight_to_clause.begin(); mit != weight_to_clause.end(); mit++){
                vector<uint32_t> &clauses = mit->second;

                // map literals in clause groups to [0-...] range
                map <uint32_t, uint32_t> l_to_i;
                for(auto cit = clauses.begin(); cit != clauses.end(); cit++){
                    clause_t &clause = expr.clauses[*cit];
                    for(auto lit = clause.literals.begin(); lit != clause.literals.end(); lit++){
                        if(l_to_i.find(abs(*lit)) == l_to_i.end())
                            l_to_i[abs(*lit)] = l_to_i.size()-1;
                    }
                }

                // print current clause group
                printf("(%u/%u) probability: ", v, VARIABLES);
                if(mit->first==-1)
                    printf("NONE  probability: NONE   ");
                else printf("%-4d  probability: %-.3f  ", expr.LITERALS+1+mit->first, weight_to_probability[mit->first]);
                printf("literals: %-4d  clauses: %-4d\n", l_to_i.size(), mit->second.size());

                // print participating clauses
                //for(unsigned int k = 0; k < clauses.size(); k++){
                //    int i = clauses[k];
                //    printf("   %3u: v%-4u ", i, expr.clause_to_variable[i]);
                //    if(get_probability(i) == -1)
                //        printf("NONE  ");
                //    else
                //        printf("p%-4d ",expr.LITERALS+1+clause_to_probability[i]);
                //    printf("(%-.3f): ", get_probability(i));
                //    for(unsigned int j = 0; j < expr.clauses[i].literals.size(); j++){
                //        if(!signbit(expr.clauses[i].literals[j]))
                //            printf(" %-3d ", expr.clauses[i].literals[j]);
                //        else
                //            printf("%-4d ", expr.clauses[i].literals[j]);
                //    }
                //    printf("\n");
                //}

                unsigned int offset = nclauses.size();
                unsigned int CLAUSES = clauses.size();
                nclause_to_weight.resize(offset+CLAUSES);
                nclause_to_variable.resize(offset+CLAUSES);
                for(unsigned int i = 0; i < CLAUSES; i++){
                    nclause_to_weight[offset+i] = mit->first;
                    nclause_to_variable[offset+i] = v;
                }

                // perform Quine-McCluskey on clause group
                if(clauses.size() > 1){
                    dynamic_assign(qm_variable_count, l_to_i.size())++;
                    qm_possible++;
                    #if __LP64__
                    if((QM_LIMIT > 0 && l_to_i.size() > (unsigned int) QM_LIMIT) || l_to_i.size() > 128){
                    #else
                    if((QM_LIMIT > 0 && l_to_i.size() > (unsigned int) QM_LIMIT) || l_to_i.size() > 64){
                    #endif
                        printf("  SKIPPED\n");
                        nclauses.resize(offset+CLAUSES);
                        for(unsigned int i = 0; i < CLAUSES; i++)
                            nclauses[offset+i] = expr.clauses[clauses[i]];
                    } else {
                        qm_eligible++;
                        if(l_to_i.size() <= 32)
                            reduce<uint32_t>(clauses, nclauses, l_to_i);
                        else if(l_to_i.size() <= 64)
                            reduce<uint64_t>(clauses, nclauses, l_to_i);
                        #if __LP64__
                        else if(l_to_i.size() <= 128)
                            reduce<uint128_t>(clauses, nclauses, l_to_i);
                        #endif
                    }
                } else if(clauses.size() == 1) {
                    nclauses.resize(offset+1);
                    nclauses[offset] = expr.clauses[clauses[0]];
                }
            }
        }
        expr.clause_to_variable = nclause_to_variable;
        clause_to_weight = nclause_to_weight;
        expr.clauses = nclauses;
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
        case PARTITION:
            if(!OPT_QUINE_MCCLUSKEY) // not implemented when constraints or not at the beginning
                OPT_PARTITION = true;
        case EQUAL_PROBABILITIES:
            OPT_EQUAL_PROBABILITIES = true;
            break;
        case DETERMINISTIC_PROBABILITIES:
            OPT_DETERMINISTIC_PROBABILITIES = true;
            break;
        case SYMPLIFY:
            OPT_DETERMINISTIC_PROBABILITIES = true;
            OPT_SYMPLIFY = true;
            break;
        case QUINE_MCCLUSKEY:
            OPT_QUINE_MCCLUSKEY = true;
            OPT_EQUAL_PROBABILITIES = true;
            OPT_SUPPRESS_CONSTRAINTS = false;
            set_encoding(1);
            break;
        case BOOL:
            OPT_BOOL = true;
            break;
        case SUPPRESS_CONSTRAINTS:
            OPT_SUPPRESS_CONSTRAINTS = true;
            set_encoding(1);
            break;
        default:
            break;
    }
}

void cnf::set_encoding(int encoding){
    this->encoding = encoding;
}

void cnf::print(){
    for(unsigned int v = 0; v < VARIABLES; v++){
        printf("%u>", v);
        if(OPT_BOOL && expr.values[v] == 2){
            printf("%u\n", expr.variable_to_literal[v]);
        } else {
            for(unsigned int l = 0; l < expr.values[v]; l++){
                if(l > 0)
                    printf(",");
                printf("%u", expr.variable_to_literal[v]+l);
            }
            printf("\n");
        }
    }

    for(unsigned int i = 0; i < expr.clauses.size(); i++){
        printf(" %3u: p%-4d v%-4u ", i, expr.clauses[i].w, expr.clause_to_variable[i]);
        if(get_probability(i) == -1)
            printf(" NONE: ");
        else printf("%.3f: ", get_probability(i));

        for(unsigned int j = 0; j < expr.clauses[i].literals.size(); j++){
            bool pauze = !signbit(expr.clauses[i].literals[j]);
            if(pauze)
                printf(" ");
            if(pauze)
                printf("%-3d ", expr.clauses[i].literals[j]);
            else
                printf("%-4d ", expr.clauses[i].literals[j]);

        }
        printf("\n");
    }
}

//void cnf::print(){
//    for(unsigned int i = 0; i < expr.clauses.size(); i++)
//       printf("%3u (v%-3u) p%3u %f\n",i, expr.clause_to_variable[i], clause_to_weight[i], weight_to_probability[clause_to_weight[i]]);
//}


const std::vector<probability_t>& cnf::get_weight_to_probability() const {
    return weight_to_probability;
}

expression_t* cnf::get_expression(int i) const {
    if(i < 0)
        return (expression_t*) &expr;
    else return (expression_t*) &(exprs[i]);
}

unsigned int cnf::get_nr_expressions() const{
    return exprs.size();
}


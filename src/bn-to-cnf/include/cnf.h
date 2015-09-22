#ifndef CNF_H
#define CNF_H

#include <vector>
#include <stdint.h>
#include <stdio.h>
#include <map>
#include <vector>
#include "config.h"

class bayesnet;

typedef int32_t literal_value_t;
typedef uint8_t bool_t;

struct clause {
    literal_value_t unsat;
    std::vector<literal_value_t> literals;
    std::vector<bool_t> sat;

    clause& operator=(const clause &c){
        literals = c.literals;
        return *this;
    };
};

struct literal {
    literal_value_t unsat;
    std::vector<literal_value_t> clauses;
    std::vector<bool_t> negated;
    std::vector<bool_t> sat;

    literal& operator=(const literal &l){
        clauses = l.clauses;
        negated = l.negated;
        return *this;
    };
};

struct expression {
    unsigned int unsat;
    unsigned int empty;

    std::vector<clause> clauses;
    std::vector<literal> literals;

    void finalize();
    void init();

    expression& operator=(const expression &e){
        clauses = e.clauses;
        literals = e.literals;
        return *this;
    };
};

typedef clause clause_t;
typedef literal literal_t;
typedef expression expression_t;

class cnf {
    public:
        enum opt_t {
            EQUAL_PROBABILITIES,
            DETERMINISTIC_PROBABILITIES,
            SYMPLIFY,
            QUINE_MCCLUSKEY,
            SUPPRESS_CONSTRAINTS,
            BOOL
        };

        cnf();
        ~cnf();

        int read(char*);
        int write(char*, bayesnet *bn = NULL);

        void stats(FILE *file = stdout);
        int condition(int);

        int encode(bayesnet *bn = NULL);
        void set_encoding(int);
        void set_optimization(opt_t);
        void set_filename(char*);
        void set_qm_limit(int);

        void print();
    private:
        template <class T> void reduce(std::vector<uint32_t> &, std::vector<clause> &, std::map<uint32_t,uint32_t> &, uint32_t, uint32_t);
        inline uint32_t v_to_l(uint32_t, uint32_t);
        void fix_literal_implications();
        void encode_constraints(bayesnet*);
        void encode_probabilities(bayesnet*);
        void encode_prime(bayesnet*);
        void encode_deterministic_probabilities();
        void encode_determinism();
        void encode_equal_probabilities();
        void apply_optimization();
        void init();
        void clear();
        inline probability_t get_weight(unsigned int);
        expression_t expr;
        unsigned int LITERALS;
        unsigned int VARIABLES;
        int QM_LIMIT;
        std::vector<unsigned int> qm_variable_count;
        unsigned int qm_eligible;
        unsigned int qm_possible;
        int encoding;
        char *filename;
        bool
            OPT_EQUAL_PROBABILITIES,
            OPT_DETERMINISTIC_PROBABILITIES,
            OPT_SUPPRESS_CONSTRAINTS,
            OPT_SYMPLIFY,
            OPT_QUINE_MCCLUSKEY,
            OPT_BOOL;

        std::vector<uint32_t> values;
        std::vector<uint32_t> literal_to_variable;
        std::vector<uint32_t> variable_to_literal;
        std::vector<uint32_t> clause_to_variable;
        std::vector<int32_t> clause_to_probability;
        std::vector<probability_t> probability_to_weight;
};

inline uint32_t cnf::v_to_l(uint32_t variable, uint32_t value){
    return variable_to_literal[variable]+value;
}

#endif


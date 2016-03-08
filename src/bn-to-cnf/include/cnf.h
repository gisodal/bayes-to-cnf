#ifndef CNF_H
#define CNF_H

#include <vector>
#include <stdint.h>
#include <stdio.h>
#include <map>
#include <set>
#include <vector>
#include "config.h"

class bayesnet;

typedef int32_t literal_t;
typedef uint32_t uliteral_t;
typedef uint32_t variable_t;
typedef literal_t weight_t;
typedef uint8_t bool_t;

struct clause {
    clause() { w = -1; };

    inline bool weighted(){ return w >= 0; };
    weight_t w;
    std::vector<literal_t> literals;
    clause& operator=(const clause &c){
        literals = c.literals;
        w = c.w;
        return *this;
    };
};

typedef std::vector<uint32_t> array_t;

class cnf;

class expression {
    friend class cnf;
    public:
        expression();
        std::vector<clause> clauses;

        void clear();
        unsigned int get_nr_variables() const;
        unsigned int get_nr_weights() const;
        unsigned int get_nr_literals() const;
        unsigned int get_nr_clauses() const;
        void map_literals();
        bool is_mapped();

        const array_t& get_values() const;
        const array_t& get_variable_to_literal() const;
        const array_t& get_literal_to_variable() const;
        const array_t& get_clause_to_variable() const;
        expression& operator=(const expression &e);

        const std::vector<literal_t>& get_literal_map();
        const std::vector<weight_t>& get_weight_map();
        const std::vector<unsigned int>& get_variable_map();
    private:
        void print();

        unsigned int LITERALS;
        unsigned int WEIGHTS;
        bool mapped;
        array_t values;
        array_t literal_to_variable;
        array_t variable_to_literal;
        array_t clause_to_variable;
        std::vector<literal_t> literal_to_literal_map;
        std::vector<weight_t> weight_to_weight_map;
        std::vector<unsigned int> variable_to_variable_map;
};

typedef clause clause_t;
typedef expression expression_t;



class cnf {
    public:
        enum opt_t {
            PARTITION,
            EQUAL_PROBABILITIES,
            DETERMINISTIC_PROBABILITIES,
            SYMPLIFY,
            QUINE_MCCLUSKEY,
            SUPPRESS_CONSTRAINTS,
            BOOL
        };

        cnf();
        ~cnf();

        // int read(char*);
        int write(const char *extra = NULL);
        void stats(FILE *file = stdout, expression_t *e = NULL);

        int encode(bayesnet *bn);
        void set_encoding(int);
        void set_optimization(opt_t);
        void set_filename(char*);
        void set_qm_limit(int);

        void print();

        const uint32_t* get_states() const;
        const std::vector<probability_t>& get_weight_to_probability() const;
        expression_t* get_expression(int i = -1) const;
        unsigned int get_nr_expressions() const;
        const std::vector<probability_t> & get_probability_to_weight() const;
        unsigned int get_nr_variables() const;
        unsigned int get_nr_literals() const;
        unsigned int get_nr_weights() const;
        bayesnet* get_bayesnet() const;
    private:
        int write(const char*, int i);
        template <class T> void reduce(std::vector<uint32_t> &, std::vector<clause> &, std::map<uint32_t,uint32_t> &);
        inline uint32_t v_to_l(uint32_t, uint32_t);
        probability_t get_probability(unsigned int);
        probability_t get_probability(unsigned int, expression &expr);
        probability_t get_probability(weight_t);

        void encode_partitions();
        void encode_constraints();
        void encode_probabilities();
        void encode_prime();
        void encode_deterministic_probabilities();
        void encode_determinism();
        void encode_equal_probabilities();
        void apply_optimization();

        void init();
        void clear();

        std::vector<unsigned int> qm_variable_count;
        unsigned int qm_eligible;
        unsigned int qm_possible;
        int encoding;
        char *filename;
        bool
            OPT_PARTITION,
            OPT_EQUAL_PROBABILITIES,
            OPT_DETERMINISTIC_PROBABILITIES,
            OPT_SUPPRESS_CONSTRAINTS,
            OPT_SYMPLIFY,
            OPT_QUINE_MCCLUSKEY,
            OPT_BOOL;

        int QM_LIMIT;
        unsigned int CONSTRAINTS;
        unsigned int VARIABLES;
        expression_t expr;
        std::vector<expression> exprs;
        std::vector< std::map<unsigned int, unsigned int> > variable_expr_map;

        std::vector<probability_t> weight_to_probability;
        bayesnet *bn;
};

#endif


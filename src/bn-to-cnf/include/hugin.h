#ifndef HUGIN_H
#define HUGIN_H

#include "reader.h"
#include <string>
#include <vector>
#include <map>
#include <stdarg.h>
#include "bayesnet.h"
#include "error.h"

typedef throw_string_error hugin_error;

namespace HUGIN {

    // GRAMMAR:
    //
    // <domain definition>
    // <domain definition>  -> <domain header> <domain element>*
    // <domain header>      -> net { <attributes>* }
    // <domain element>     -> <basic node> | <potential>
    // <attribute>          -> <attribute anme> = <attribute value>;
    // <class definition>   -> class <class name> { <class element>* }
    // <class element>      -> <domain element> | attribute> | <class instance> | <temporal clone>

    enum domain_element_type {
        node_domain_element,
        potential_domain_element
    };

    enum subtype_type {
        label_subtype_type,
        boolean_subtype_type,
        number_subtype_type,
        interval_subtype_type
    };

    enum node_type {
        simple_node_type,
        decision_node_type,
        utility_node_type,
        function_node_type,
        temporal_node_type
    };

    enum class_element_type {
        domain_class_element,
        attribute_class_element,
        instance_class_element,
        temporal_class_clone_element
    };

    enum variable_type {
        undefined_variable,
        float_variable,
        string_variable
    };

    struct variable {
        void print();
        variable();
        void parse(reader&, unsigned int d = 0);
        variable_type type;
        unsigned int dimensions;
        std::vector<unsigned int> dim;
        std::vector<std::string> value;
    };

    struct attribute {
        void print();
        void parse(reader&);

        std::string name;
        variable value;
    };

    struct domain_element {
        void print();
        void parse(reader&);
        attribute* get_attribute(std::string);
        domain_element_type type;
        std::string name;
        std::vector<attribute> attr;
    };

    struct domain_header {
        void print();
        void parse(reader&);
        std::vector <attribute> attr;
    };

    struct domain_definition {
        void print();
        void parse(reader&);
        domain_header header;
        std::vector<domain_element> elements;
    };

}

struct hugin {
    hugin();

    bool process(std::string);
    HUGIN::domain_definition definition;
    void parse(reader&);
    void print();
    reader input;
    bayesnet* get_bayesnet();
    std::string filename;
};

#endif

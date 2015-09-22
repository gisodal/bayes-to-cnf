#include "qm.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

using namespace std;
#if __LP64__
const int MAX_QM = 128;
typedef uint128_t T;
#else
const int MAX_QM = 64;
typedef uint64_t T;
#endif

void help(){
    fprintf (stderr, "Usage: bin/quine-mccluskey -v <#variables> -o <model1[,model2[,...]]>\n");
}

int isnumber (const char * s){
    if (s == NULL || *s == '\0' || isspace(*s))
      return 0;
    char * p;
    strtod (s, &p);
    return *p == '\0';
}

int main (int argc, char **argv){
    qm<T> q;

    opterr = 0;
    int i,c,index;
    char *opt;
    bool req[2] = { 0 };
    while ((c = getopt (argc, argv, "v:o:")) != -1){
        switch (c){
            case 'v':
                if(isnumber(optarg)){
                    if(atoi(optarg) > MAX_QM){
                        fprintf(stderr, "Cannot handle more than %d variables\n", MAX_QM);
                        exit(1);
                    }
                    for(i = 0; i < atoi(optarg); i++)
                        q.add_variable(i);
                    req[0] = 1;
                } else fprintf(stderr, "Argument to -v (%s) is not a number\n", optarg);
                break;
            case 'o':
                opt = strtok (optarg,",");
                req[1] = 1;
                while(opt != NULL){
                    if(isnumber(opt)){
                        q.add_model(atoi(opt));
                        opt = strtok (NULL, ",");
                    } else {
                        fprintf(stderr, "Argument to -o (%s) is not a number\n", opt);
                        req[1] = 0;
                        break;
                    }
                }
                break;
            case '?':
                if (optopt == 'o')
                    fprintf (stderr, "Option -%c requires comma separated list of models.\n", optopt);
                else if (optopt == 'v')
                    fprintf (stderr, "Option -%c requires #variables as argument.\n", optopt);
                else if (isprint (optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);

                help();
                return 1;
            default:
                help();
                return 1;
        }
    }
    for (index = optind; index < argc; index++)
        fprintf(stderr, "Ingoring argument '%s'\n", argv[index]);


    if(req[0] && req[1]){
        q.solve();
        q.print(true);
    } else help();

    return 0;
}


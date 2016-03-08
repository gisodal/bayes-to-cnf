#include <stdio.h>
#include <string>
#include <unistd.h>
#include <ctype.h>
#include "parser.h"
#include "bayesnet.h"
#include "cnf.h"
#include "misc.h"

int isnumber (const char * s){
    if (s == NULL || *s == '\0' || isspace(*s))
      return 0;
    char * p;
    strtod (s, &p);
    return *p == '\0';
}

void help(){
    fprintf(stderr, "\nUsage:\n   ./bn-to-cnf -i <filename> [option] [...]\n\n");
    fprintf(stderr, "   Options:\n");
    fprintf(stderr, "      optimizations:\n");
    fprintf(stderr, "         -p: Partition cnf per CPT\n");
    fprintf(stderr, "         -c: Constraints are supressed\n");
    fprintf(stderr, "         -e: Equal probabilities are encoded\n");
    fprintf(stderr, "         -d: Determinism is encoded\n");
    fprintf(stderr, "         -a: Apply boolean symplification\n");
    fprintf(stderr, "         -b: Boolean variables are not mapped\n");
    fprintf(stderr, "         -q: Quine-McCluskey (QM)\n");
    fprintf(stderr, "         -l <limit>: Limit problem size for QM\n");
    fprintf(stderr, "      other:\n");
    fprintf(stderr, "         -i <filename>: Input (HUGIN .net file)\n");
    fprintf(stderr, "         -w: Write CNF in DIMACS format to file\n");
    fprintf(stderr, "         -s: Show stats\n");
    fprintf(stderr, "         -h: Help\n");
}

int main(int argc, char **argv){
    cnf f;
    int c;
    char infile[100] = {0};
    char outfile[100];
    char ext[20] = {0};

    bool write = false, stats = false;
    while ((c = getopt(argc, argv, "i:adecswbhpql:")) != -1){
        switch (c){
            case 'p': // partitioned
                f.set_optimization(cnf::opt_t::PARTITION);
                break;
            case 'd': // determinism
                f.set_optimization(cnf::opt_t::DETERMINISTIC_PROBABILITIES);
                break;
            case 'e': // equal probabilities recoginized
                f.set_optimization(cnf::opt_t::EQUAL_PROBABILITIES);
                break;
            case 'c': // constraint clauses are suppressed
                f.set_optimization(cnf::opt_t::SUPPRESS_CONSTRAINTS);
                break;
            case 'b': // bool variables are not mapped
                f.set_optimization(cnf::opt_t::BOOL);
                break;
            case 'a': // symplify cnf using identity property
                f.set_optimization(cnf::opt_t::SYMPLIFY);
                break;
            case 'q': // bool variables are not mapped
                f.set_optimization(cnf::opt_t::QUINE_MCCLUSKEY);
                break;
            case 'l': // bool variables are not mapped
                if(isnumber(optarg))
                    f.set_qm_limit(atoi(optarg));
                else {
                    fprintf(stderr, "Argument to option -l (%s) is not a number\n", optarg);
                    return 1;
                }
                break;
            case 'w':
                write = true;
                break;
            case 's':
                stats = true;
                break;
            case 'i': // provide input
                strcpy(infile,optarg);
                strcpy(ext, get_filename_ext(infile));
                strcpy(outfile,infile);
                remove_ext(outfile);
                if(strcmp(ext, "net") != 0){
                    fprintf(stderr, "Unknown file extension, a '*.net' HUGIN file is required\n");
                    return 1;
                }
                break;
            case '?':
                help();
                return 1;
            default:
                help();
                return 1;
        }
    }
    for (int index = optind; index < argc; index++)
        printf ("Non-option argument %s\n", argv[index]);

    if(infile[0] == 0){
        help();
        fprintf(stderr, "Specify input file with -i <filename>\n");
        return 1;
    }

    parser<hugin> net;
    if(!net.process(infile))
        fprintf(stderr, "FAILED\n");
    else {
        f.set_filename(outfile);
        bayesnet *bn = NULL;
        try {
            bn = net.get_bayesnet();
            if(bn == NULL)
                fprintf(stderr, "FAILED\n");
        } catch(throw_string_error &e){
            fprintf(stderr, "error: %s\n", e.what());
            fprintf(stderr, "FAILED\n");
            return -1;
        }

        f.encode(bn);
        if(write)
            f.write();

        if(stats)
            f.stats();

        // f.print();
        delete bn;
    }

    return 0;
}

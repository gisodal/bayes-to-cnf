#include <stdio.h>
#include <string>
#include <unistd.h>
#include "parser.h"
#include "bayesnet.h"
#include "cnf.h"
#include "misc.h"

void help(){
    fprintf(stderr, "\nUsage:\n   ./bn-to-cnf -i <filename> [option] [...]\n\n");
    fprintf(stderr, "   Options:\n");
    fprintf(stderr, "      -c: Constraints are supressed\n");
    fprintf(stderr, "      -e: Equal probabilities is encoded\n");
    fprintf(stderr, "      -d: Determinism is encoded\n");
    fprintf(stderr, "      -s: Symplify encoding\n");
    fprintf(stderr, "      -i: Input file\n");
    fprintf(stderr, "      -w: Write CNF in DIMACS format to file\n");
    fprintf(stderr, "      -p: Print stats to stdout\n");
    fprintf(stderr, "      -h: Help\n");
}

int main(int argc, char **argv){
    cnf f;
    int c;
    char infile[100] = {0};
    char outfile[100];
    char ext[20] = {0};

    char *opt;
    bool write = false, stats = false;
    while ((c = getopt(argc, argv, "i:decswbhp")) != -1){
        switch (c){
            case 'd': // determinism
                f.set_optimization(cnf::opt_t::DETERMINISM);
                break;
            case 'e': // equal probabilities recoginized
                f.set_optimization(cnf::opt_t::EQUAL_PROBABILITIES);
                break;
            case 'c': // constraint clauses are suppressed
                f.set_optimization(cnf::opt_t::SUPRESS_CONSTRAINTS);
                break;
            case 'b': // bool variables are not mapped
                f.set_optimization(cnf::opt_t::BOOL);
                break;
            case 's': // bool variables are not mapped
                f.set_optimization(cnf::opt_t::SYMPLIFY);
                break;
            case 'w':
                write = true;
                break;
            case 'p':
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
        fprintf(stderr, "Specify input file with -i <filename>\n");
        return 1;
    }

    parser<hugin> net;
    if(!net.process(infile))
        fprintf(stderr, "FAILED\n");
    else {
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
        f.set_filename(outfile);

        f.encode(bn);
        if(write)
            f.write(outfile,true, bn);

        if(stats)
            f.stats();

        //f.print();
        delete bn;
    }

    return 0;
}

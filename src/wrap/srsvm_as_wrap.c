#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(__unix__)
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

#include "srsvm/config.h"
#include "srsvm/program.h"
#include "srsvm/impl.h"

#define PROG_NAME "srsvm_as"

bool srsvm_debug_mode = false;

void write_error(const char* message)
{
    fprintf(stderr, "Error: %s\n", message);
}

void show_usage(const char* error)
{
    if(error != NULL){
        write_error(error);
    }

    fprintf(stderr, "Usage: %s [optional args] program_name.svm\n", PROG_NAME);
    fprintf(stderr, "\n");
    fprintf(stderr, "    optional arguments:\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "      -ws <WORD_SIZE>       : specify target output word size (16/32/64/128)\n");
    fprintf(stderr, "      -o output_file.svm    : specify output filename\n");
    fprintf(stderr, "      -A <alignment>        : specify target output alignment (default: 0)\n");
    fprintf(stderr, "      -L <mod_search_path>  : specify module search path (semicolon separated)\n");
    fprintf(stderr, "      -D  |  --debug        : enable srsvm debug messages\n");
    fprintf(stderr, "      -h  |  --help         : show this help information\n");

    if(error != NULL){
        exit(1);
    } else exit(0);
}

char* search_prog(const char* prog_name)
{
    char *prog_path = NULL;

    char *subdir = NULL;

#if defined(__unix__)
#if defined(SRSVM_LIBEXEC_DIR)
    if(srsvm_directory_exists(SRSVM_LIBEXEC_DIR))
    {
        prog_path = srsvm_path_combine(SRSVM_LIBEXEC_DIR, prog_name);

        if(prog_path != NULL){
            if(! srsvm_file_exists(prog_name)){
                free(prog_path);
                prog_path = NULL;
            }
        }
    }
#endif
#if defined(SRSVM_USER_HOME_LIBEXEC_DIR)
    if(prog_path == NULL){
        const char* home_dir = getenv("HOME");

        if(home_dir == NULL){
            size_t pw_size_max = sysconf(_SC_GETPW_R_SIZE_MAX);
            if(pw_size_max == -1){
                pw_size_max = 0x4000;
            }

            struct passwd pwd, *r;
            void *buf = malloc(pw_size_max);
            if(buf != NULL){
                getpwuid_r(geteuid(), &pwd, buf, pw_size_max, &r);

                if(r != NULL){
                    home_dir = r->pw_dir;

                    subdir = srsvm_path_combine(home_dir, SRSVM_USER_HOME_LIBEXEC_DIR);

                    if(subdir != NULL){
                        prog_path = srsvm_path_combine(subdir, prog_name);

                        free(subdir);
                        if(prog_path != NULL){
                            if(!srsvm_file_exists(prog_path)){
                                free(prog_path);
                                prog_path = NULL;
                            }
                        }
                    }
                }

                free(buf);
            }
        } else {
            subdir = srsvm_path_combine(home_dir, SRSVM_USER_HOME_LIBEXEC_DIR);

            if(subdir != NULL){
                prog_path = srsvm_path_combine(subdir, prog_name);

                free(subdir);
                if(prog_path != NULL){
                    if(!srsvm_file_exists(prog_path)){
                        free(prog_path);
                        prog_path = NULL;
                    }
                }
            }
        }
    }
#endif
#endif
    return prog_path;
}

void run_arch(int argc, char *argv[], uint8_t word_size)
{
    const char* prog_name = NULL;

    switch(word_size){
        case 16:
            prog_name = PROG_NAME "_16";
            break;

        case 32:
            prog_name = PROG_NAME "_32";
            break;

        case 64:
            prog_name = PROG_NAME "_64";
            break;

        case 128:
            prog_name = PROG_NAME "_128";
            break;
    }

    argv[0] = strdup(prog_name);

    char* prog_path = search_prog(prog_name);

    if(prog_path == NULL){
        write_error("unable to locate arch-specific loader");
        exit(1);
    }

    if(execvp(prog_path, argv) == -1){
        write_error("execvp failed");
        exit(1);
    }
}


int main(int argc, char* argv[]){
    char err_buf[1024] = { 0 };

    unsigned word_size = 0;

    bool options_done = false;

    for(int i = 1; i < argc; i++){
        switch(argv[i][0])
        {
            case '-':
                if(! options_done){
                    if(strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0){
                        if(srsvm_debug_mode){
                            show_usage("debug flag may only be specified once");
                        } else {
                            srsvm_debug_mode = true;
                        }
                    } else if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0){
                        show_usage(NULL);
                    } else if(strcmp(argv[i], "-ws") == 0){
                        if(word_size != 0){
                            show_usage("duplicate -ws flag");
                        } else if(i >= argc - 1){
                            show_usage("-ws specified with no argument");
                        } else if(! sscanf(argv[++i], "%u", &word_size)){
                            show_usage("failed to parse -ws argument as unsigned integer");        
                        }
                    } else if(strcmp(argv[i], "--") == 0){
                        options_done = true;
                    }
                }
                break;
        }
    }

    if(word_size == 0){
    }

    switch(word_size)
    {
        case 0:
            show_usage("no -ws argument supplied; this wrapper does not know what word size to use");
            break;

        case 16:
        case 32:
        case 64:
        case 128:
            run_arch(argc, argv, word_size);
            break;

        default:
            snprintf(err_buf, sizeof(err_buf), "invalid program word size: %u", word_size);
            write_error(err_buf);

            exit(1);
    }

    return 1;
}

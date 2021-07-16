#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "srsvm/config.h"
#include "srsvm/debug.h"
#include "srsvm/mmu.h"
#include "srsvm/vm.h"

#define STR(x) #x
#define STR_HELPER(x) STR(x)

#define PROG_NAME "srsvm_" STR_HELPER(WORD_SIZE)

bool srsvm_debug_mode;

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
    fprintf(stderr, "      -d  |  --debug        : enable srsvm debug messages\n");
    fprintf(stderr, "      -h  |  --help         : show this help information\n");

    if(error != NULL){
        exit(1);
    } else exit(0);
}

int main(int argc, char* argv[]){
    int exit_status;

    srsvm_debug_mode = false;

    char err_buf[1024] = { 0 };
    
    char* program_name = NULL;

    for(int i = 1; i < argc; i++){
        switch(argv[i][0])
        {
            case '-':
                if(strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0){
                    if(srsvm_debug_mode){
                        show_usage("debug flag may only be specified once");
                    } else {
                        srsvm_debug_mode = true;
                    }
                } else if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0){
                    show_usage(NULL);
                } else {
                    snprintf(err_buf, sizeof(err_buf), "unrecognized flag: '%s'", argv[i]);
                    show_usage(err_buf);
                }
                break;

            default:
                if(program_name != NULL){
                    show_usage("only one program may be specified");
                } else {
                    program_name = argv[i];
                }

                break;
        }
    }

    if(program_name == NULL){
        show_usage("no program specified");
    } else if(! srsvm_file_exists(program_name)){
        snprintf(err_buf, sizeof(err_buf), "file %s not found", program_name);
        show_usage(err_buf);
    }
    
    srsvm_vm *vm = NULL;
    srsvm_program *program = NULL;
    srsvm_thread *main_thread = NULL;

    if((vm = srsvm_vm_alloc()) == NULL){
        write_error("failed to allocate memory for virtual machine");

        exit_status = 1;
        goto cleanup;
    } else if((program = srsvm_program_deserialize(program_name)) == NULL){
        snprintf(err_buf, sizeof(err_buf), "failed to deserialize program '%s'", program_name);
        write_error(err_buf);

        exit_status = 1;
        goto cleanup;
    } else if(! srsvm_vm_load_program(vm, program)){
        snprintf(err_buf, sizeof(err_buf), "failed to load program '%s' into virtual machine", program_name);
        write_error(err_buf);
        
        exit_status = 1;
        goto cleanup;
    }

    const char* mod_path = getenv(SRSVM_MOD_PATH_ENV_NAME);

	srsvm_vm_set_module_search_path(vm, mod_path);

    main_thread = vm->main_thread;

    srsvm_vm_start_thread(vm, main_thread->id);

    srsvm_vm_join_thread(vm, main_thread->id);

    exit_status = (int) main_thread->exit_status;

cleanup:
    if(main_thread != NULL) srsvm_thread_free(vm, main_thread);
    if(program != NULL) srsvm_program_free(program);
    if(vm != NULL) srsvm_vm_free(vm);

    return exit_status;
}

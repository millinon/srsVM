#include <stdlib.h>
#include <string.h>

#if defined(__unix__)
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include "srsvm/config.h"

#endif

#include "srsvm/module.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

static bool load_opcode(void* arg, srsvm_opcode* opcode)
{
    srsvm_module *mod = arg;

    bool success = false;

    if(strlen(opcode->name) > 0 && opcode_lookup_by_name(mod->opcode_map, opcode->name) != NULL){

    } else if(opcode_lookup_by_code(mod->opcode_map, opcode->code) != NULL){

    } else if(! opcode_map_insert(mod->opcode_map, opcode)) {

    } else {
        return true;
    }

    return success;
}

srsvm_module *srsvm_module_alloc(const char* name, const char* filename, srsvm_word id)
{
    srsvm_module *mod = NULL;

    mod = malloc(sizeof(srsvm_module));

    if(mod != NULL){
        mod->ref_count = 1;
        mod->id = id;

        mod->tag = NULL;

        if(strlen(name) < SRSVM_MODULE_MAX_NAME_LEN){
            strncpy(mod->name, name, sizeof(mod->name));

            if(srsvm_native_module_load(&mod->handle, filename)){
                if(! srsvm_native_module_supports_word_size(&mod->handle, WORD_SIZE)){
                    goto error_cleanup;
                }

                mod->opcode_map = srsvm_opcode_map_alloc();
                
                if(mod->opcode_map == NULL){
                    goto error_cleanup;
                } else if(! srsvm_native_module_load_opcodes(&mod->handle, load_opcode, mod)){
                    goto error_cleanup;
                }
            } else {
                goto error_cleanup;
            }
        } else {
            goto error_cleanup;
        }
    }

    return mod;

error_cleanup:
    if(mod != NULL){
        srsvm_module_free(mod);
    }

    return NULL;
}

void srsvm_module_free(srsvm_module *mod)
{
    if(mod != NULL){
        if(mod->opcode_map != NULL) srsvm_opcode_map_free(mod->opcode_map);
        srsvm_native_module_unload(mod->handle);
        free(mod);
    }
}

char* srsvm_module_find(const char* module_name, const char* prog_cwd, char** search_path, bool search_multilib)
{
    char* module_path = NULL;

    char* mod_filename = srsvm_module_name_to_filename(module_name);

    char* subdir;

    if(module_name != NULL && mod_filename != NULL){
        if(search_path != NULL){
            for(size_t i = 0; search_path[i] != NULL; i++){
                if(srsvm_directory_exists(search_path[i])){
                    
                    if(search_multilib){
                        subdir = srsvm_path_combine(search_path[i], "multilib");

                        if(srsvm_directory_exists(subdir)){
                            module_path = srsvm_path_combine(subdir, mod_filename);

                            free(subdir);
                            if(! srsvm_file_exists(module_path)){
                                free(module_path);
                                module_path = NULL;
                            } else {
                                break;
                            }
                        } else {
                            free(subdir);
                        }
                    }

                    if(module_path == NULL){
                        subdir = srsvm_path_combine(search_path[i], STR_HELPER(WORD_SIZE));
                        if(srsvm_directory_exists(subdir)){
                            module_path = srsvm_path_combine(subdir, mod_filename);

                            free(subdir);
                            if(! srsvm_file_exists(module_path)){

                                free(module_path);
                                module_path = NULL;
                            } else {
                                break;
                            }
                        } else {
                            free(subdir);
                        }
                    }

                    if(module_path == NULL){
                        module_path = srsvm_path_combine(search_path[i], mod_filename);

                        if(! srsvm_file_exists(module_path)){
                            free(module_path);
                            module_path = NULL;
                        } else {
                            break;
                        }
                    }
                }
            }
        }

#if defined(__unix__)
#if defined(SRSVM_LIB_DIR)
        if(module_path == NULL){
            if(srsvm_directory_exists(SRSVM_LIB_DIR)){
                if(search_multilib){
                    char *subdir = srsvm_path_combine(SRSVM_LIB_DIR, "multilib");

                    if(srsvm_directory_exists(subdir)){
                        module_path = srsvm_path_combine(subdir, mod_filename);

                        free(subdir);
                        if(! srsvm_file_exists(module_path)){
                            free(module_path);
                            module_path = NULL;
                        }
                    } else {
                        free(subdir);
                    }
                }

                if(module_path == NULL){
                    subdir = srsvm_path_combine(SRSVM_LIB_DIR, STR_HELPER(WORD_SIZE));
                    if(srsvm_directory_exists(subdir)){
                        module_path = srsvm_path_combine(subdir, mod_filename);

                        free(subdir);
                        if(! srsvm_file_exists(module_path)){
                            free(module_path);
                            module_path = NULL;
                        }
                    } else {
                        free(subdir);
                    }
                }

                if(module_path == NULL){
                    module_path = srsvm_path_combine(SRSVM_LIB_DIR, mod_filename);

                    if(! srsvm_file_exists(module_path)){
                        free(module_path);
                        module_path = NULL;
                    }
                }
            }
        }
#endif
#if defined(SRSVM_USER_HOME_LIB_DIR)
        if(module_path == NULL){
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

                        if(srsvm_directory_exists(home_dir)){
                            subdir = srsvm_path_combine(home_dir, SRSVM_USER_HOME_LIB_DIR);

                            if(srsvm_directory_exists(subdir)){
                                char* subdir2 = NULL;

                                if(search_multilib){
                                    subdir2 = srsvm_path_combine(subdir, "multilib");

                                    if(srsvm_directory_exists(subdir2)){
                                        module_path = srsvm_path_combine(subdir, mod_filename);

                                        free(subdir2);
                                        if(! srsvm_file_exists(module_path)){
                                            free(module_path);
                                            module_path = NULL;
                                        }
                                    } else {
                                        free(subdir2);
                                    }
                                }

                                if(module_path == NULL){
                                    subdir2 = srsvm_path_combine(subdir, STR_HELPER(WORD_SIZE));
                                    if(srsvm_directory_exists(subdir2)){
                                        module_path = srsvm_path_combine(subdir2, mod_filename);

                                        free(subdir2);
                                        if(! srsvm_file_exists(module_path)){
                                            free(module_path);
                                            module_path = NULL;
                                        }
                                    } else {
                                        free(subdir2);
                                    }
                                }
                                
                                if(module_path == NULL){
                                    module_path = srsvm_path_combine(subdir, mod_filename);

                                    if(! srsvm_file_exists(module_path)){
                                        free(module_path);
                                        module_path = NULL;
                                    }
                                }
                            }
                        }
                    }
                    free(buf);
                }
            } else {
                if(srsvm_directory_exists(home_dir)){
                    subdir = srsvm_path_combine(home_dir, SRSVM_USER_HOME_LIB_DIR);

                    if(srsvm_directory_exists(subdir)){
                        char* subdir2 = NULL;

                        if(search_multilib){
                            subdir2 = srsvm_path_combine(subdir, "multilib");

                            if(srsvm_directory_exists(subdir2)){
                                module_path = srsvm_path_combine(subdir, mod_filename);

                                free(subdir2);
                                if(! srsvm_file_exists(module_path)){
                                    free(module_path);
                                    module_path = NULL;
                                }
                            } else {
                                free(subdir2);
                            }
                        }

                        if(module_path == NULL){
                            subdir2 = srsvm_path_combine(subdir, STR_HELPER(WORD_SIZE));
                            if(srsvm_directory_exists(subdir2)){
                                module_path = srsvm_path_combine(subdir2, mod_filename);

                                free(subdir2);
                                if(! srsvm_file_exists(module_path)){
                                    free(module_path);
                                    module_path = NULL;
                                }
                            } else {
                                free(subdir2);
                            }
                        }

                        if(module_path == NULL){
                            module_path = srsvm_path_combine(subdir, mod_filename);

                            if(! srsvm_file_exists(module_path)){
                                free(module_path);
                                module_path = NULL;
                            }
                        }
                    }
                }
            }
        }
#endif
#endif

        if(module_path == NULL && prog_cwd != NULL){
            char *lib_subdir = srsvm_path_combine(prog_cwd, "lib");

            char* subdir = srsvm_path_combine(lib_subdir, "multilib");

            if(srsvm_directory_exists(subdir)){
                module_path = srsvm_path_combine(subdir, mod_filename);

                free(subdir);
                if(! srsvm_file_exists(module_path)){

                    free(module_path);
                    module_path = NULL;
                }
            } else {
                free(subdir);
            }

            if(module_path == NULL){
                subdir = srsvm_path_combine(lib_subdir, STR(WORD_SIZE));
                if(srsvm_directory_exists(subdir)){
                    module_path = srsvm_path_combine(subdir, mod_filename);

                    free(subdir);
                    if(! srsvm_file_exists(module_path)){

                        free(module_path);
                        module_path = NULL;
                    }
                } else {
                    free(subdir);
                }
            }

            free(lib_subdir);

            if(module_path == NULL && prog_cwd != NULL){
                module_path = srsvm_path_combine(prog_cwd, mod_filename);

                if(! srsvm_file_exists(module_path)){
                    free(module_path);
                    module_path = NULL;
                }
            }
        }
    }

    return module_path;
}

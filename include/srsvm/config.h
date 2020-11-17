#pragma once

#if defined(__unix__)

#define PREFIX "/usr/local"

#ifndef PREFIX
#error "PREFIX not defined"
#endif

#define SRSVM_LIBEXEC_DIR PREFIX "/libexec/srsvm"
#define SRSVM_LIB_DIR PREFIX "/lib/srsvm"

#define SRSVM_USER_HOME_LIBEXEC_DIR ".local/libexec/srsvm"
#define SRSVM_USER_HOME_LIB_DIR ".local/lib/srsvm"


#endif
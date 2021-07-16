#pragma once

/* https://stackoverflow.com/a/5920028 */

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	#define SRSVM_BUILD_TARGET_WINDOWS

   //define something for Windows (32-bit and 64-bit, this part is common)
   #ifdef _WIN64
      //define something for Windows (64-bit only)
   #else
      //define something for Windows (32-bit only)
   #endif
#elif __APPLE__
	#error "Unsupported platform"

    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR
         // iOS Simulator
    #elif TARGET_OS_IPHONE
        // iOS device
    #elif TARGET_OS_MAC
        // Other kinds of Mac OS
    #else
    #   error "Unknown Apple platform"
    #endif
#elif __linux__
	#define SRSVM_BUILD_TARGET_LINUX
    // linux
#elif __unix__ // all unices not caught above
    // Unix
	#error "Unsupported platform"
#elif defined(_POSIX_VERSION)
    // POSIX
	#error "Unsupported platform"
#else
	#error "Unsupported platform"
#endif
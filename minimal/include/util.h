#pragma once

#include <microkit.h>
#include <stdint.h>
#include <stddef.h>
#include <printf.h>

static void assert_fail(
    const char  *assertion,
    const char  *file,
    unsigned int line,
    const char  *function)
{
    printf("Failed assertion '%s' at %s:%u in function %s\n", assertion, file, line, function);
    __builtin_trap();
}

#ifndef assert
#ifndef CONFIG_DEBUG_BUILD

#define _unused(x) ((void)(x))
#define assert(expr) _unused(expr)

#else

#define assert(expr) \
    do { \
        if (!(expr)) { \
            assert_fail(#expr, __FILE__, __LINE__, __FUNCTION__); \
        } \
    } while(0)

#endif
#endif

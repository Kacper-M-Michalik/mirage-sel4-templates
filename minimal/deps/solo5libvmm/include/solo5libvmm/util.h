#pragma once

extern int printf(const char *fmt, ...);
extern void _assert_fail(const char  *assertion, const char  *file, unsigned int line, const char  *function);

#define PRINT_VMM(...) do{ printf(__VA_ARGS__); }while(0)
#define LOG_VMM(...) do{ printf("SOLO5VMM| "); printf(__VA_ARGS__); }while(0)

#ifndef assert
#ifndef CONFIG_DEBUG_BUILD
#define _unused(x) ((void)(x))
#define assert(expr) _unused(expr)
#else
#define assert(expr) \
    do { \
        if (!(expr)) { \
            _assert_fail(#expr, __FILE__, __LINE__, __FUNCTION__); \
        } \
    } while(0)
#endif
#endif
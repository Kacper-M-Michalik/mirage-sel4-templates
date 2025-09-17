#pragma once

// Includes needed to provide assert() implementation for solo5libvmm
#include <sddf/util/util.h>
#include <sddf/util/printf.h>

// Provide printf() for solo5libvmm
int printf(const char *fmt, ...) 
{
    va_list args;
    va_start(args, fmt);
    int ret = sddf_vprintf(fmt, args);
    va_end(args);
    return ret;
}
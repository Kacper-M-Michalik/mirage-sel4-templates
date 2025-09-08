#pragma once

#include <sddf/util/util.h>
#include <sddf/util/printf.h>

//#define printf(...) do{ sddf_printf_(__VA_ARGS__); }while(0)

int printf(const char *fmt, ...) 
{
    va_list args;
    va_start(args, fmt);
    int ret = sddf_vprintf(fmt, args);
    va_end(args);
    return ret;
}
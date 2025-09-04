#pragma once

// TODO: Move this to solo5libvmm, have it include stdio, and only keep printf = sddf here 

#include <sddf/util/util.h>
#include <sddf/util/printf.h>

#include <hvt_abi.h>

#define printf(...) do{ sddf_printf_(__VA_ARGS__); }while(0)
#define LOG_VMM(...) do{ printf("MIRAGEVMM|INFO: "); printf(__VA_ARGS__); }while(0)
#define LOG_VMM_ERR(...) do{ printf("MIRAGEVMM|ERROR: "); printf(__VA_ARGS__); }while(0)
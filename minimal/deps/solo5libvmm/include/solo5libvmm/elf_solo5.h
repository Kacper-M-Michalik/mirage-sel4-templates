#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

bool elf_load(uint8_t* elf_ptr, size_t elf_size, uint8_t* guest_mem, size_t guest_mem_size, uint64_t p_min_loadaddr, uint64_t* p_entry, uint64_t* p_end);
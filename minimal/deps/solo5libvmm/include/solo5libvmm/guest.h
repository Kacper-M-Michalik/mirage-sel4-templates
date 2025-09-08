#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Sets up memory and VCpu registers of virtual guest
/*  
    boot_vcpu_id - ID of VCpu for guest to run on, should always be 0 as a VMM only runs 1 guest, but left as param for future changes
    kernel - Pointer to guest image
    kernel_size - Size of guest image in bytes
    virtual_memory_offset - VMM can map in guest memory at a different address than what guest sees (from guests perspective memory always starts from address 0), this is the difference in bytes
    memory_size - Total memory given to guest, this includes space for guest image, stack and heap, should be page (4k) aligned, error if not
    max_stack_size - Maximum stack size for guest, if exceeds available memory after image load it will cause error, if set to 0 no limit is set, uses 4k of memory due to setup of protection page
    cmdline - Command line arg passed to guest as per HVT spec
    mft - Mft structure passed to guest, represents available devices as per HVT spec
*/
bool guest_setup(size_t vcpu_id, uint8_t* kernel, size_t kernel_size, uint8_t* mem, size_t mem_size, size_t max_stack_size, char* cmdline, size_t cmdline_len);

// Start guest execution from current PC value, need to figure out if pc points to next or last executed
void guest_resume(size_t vcpu_id);

// Pauses guest, gonna need to figure out how pc is setup
void guest_stop(size_t vcpu_id);

// Clears guest registers and memory, allows for setting up new guest image after 
void guest_clear(size_t vcpu_id, uint8_t* guest_mem, size_t guest_mem_size);
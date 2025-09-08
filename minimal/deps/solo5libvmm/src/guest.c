#include <string.h>
#include <microkit.h>
#include <solo5libvmm/guest.h>
#include <solo5libvmm/elf_solo5.h>
#include <solo5libvmm/aarch64/vcpu.h>
#include <solo5libvmm/hvt_abi.h>
#include <solo5libvmm/util.h>

void guest_resume(size_t vcpu_id) 
{
    LOG_VMM("Resuming guest\n");
    seL4_Error err;
    seL4_UserContext ctxt = {0};
    err = seL4_TCB_WriteRegisters(
        BASE_VM_TCB_CAP + vcpu_id,
        seL4_True,
        0,
        0,
        &ctxt
    );
    assert(err == seL4_NoError);
    LOG_VMM("Resumed guest!\n");
}

void guest_stop(size_t vcpu_id) 
{
    LOG_VMM("Stopping guest\n");
    microkit_vcpu_stop(vcpu_id);
    LOG_VMM("Stopped guest\n");
}

void guest_clear(size_t vcpu_id, uint8_t* guest_mem, size_t guest_mem_size) 
{
    LOG_VMM("Clearing guest\n");
    microkit_vcpu_stop(vcpu_id);
    LOG_VMM("Stopped guest\n");

    LOG_VMM("Clearing guest RAM\n");
    memset((uint8_t*)guest_mem, 0, guest_mem_size);

    LOG_VMM("Resetting guest registers\n");
    vcpu_reset_regs(vcpu_id);  

    LOG_VMM("Guest reset\n");
}

bool guest_setup(size_t vcpu_id, uint8_t* kernel, size_t kernel_size, uint8_t* mem, size_t mem_size, size_t max_stack_size, char* cmdline, size_t cmdline_len, void* mft, size_t mft_len)
{
    //TODO: Add max cmd len check
    //TODO: Add mem_size alignment fix
    //TODO: Check max stack is reasonable and doesnt overlap text/min heap
    //TODO: Check if mem_size big enough for kernel
    if (vcpu_id != 0) 
    {
        LOG_VMM("Invalid vcpu_id, solo5 is single-threaded and only 1 VM allowed per VMM, vcpu_id should be 0\n");
        return false;
    }
    //TODO: MOVE CONSTANTS
    const uint64_t BOOT_INFO_ADDR = 0x10000;
    const uint64_t MIN_TEXT_ADDR = 0x100000;

    //TODO: load abi note

    //TODO: load mft note
    //struct mft testmft;
    //testmft.version =

    //TODO: Add protection propagation
    uint64_t p_entry;
    uint64_t p_end;
    if (!elf_load(kernel, kernel_size, mem, mem_size, MIN_TEXT_ADDR, &p_entry, &p_end))
    {
        LOG_VMM("Failed to load HVT file (incompatible or invalid)\n");
        return false;
    }

    LOG_VMM("p_entry: %zu\n", p_entry);
    LOG_VMM("p_end: %zu\n", p_end);

    // Allocate boot info in guest memory, and verify alignment
    struct hvt_boot_info* info = (struct hvt_boot_info*)((uint64_t)mem + BOOT_INFO_ADDR);
    assert(((uint64_t)info % _Alignof(struct hvt_boot_info)) == 0);  

    info->mem_size = mem_size; //Do we subtract p_end?
    info->cpu_cycle_freq = aarch64_get_counter_frequency();
    info->kernel_end = p_end; //Do we have to increment p_end by 1 instruction?
    
    uint64_t arg_ptr = (uint64_t)info + sizeof(struct hvt_boot_info);
    memcpy((void*)arg_ptr, cmdline, cmdline_len);
    *((char*)(arg_ptr + cmdline_len)) = '\0';
    info->cmdline = arg_ptr - (uint64_t)mem; 
    arg_ptr += cmdline_len + 1;        
    //TODO: PROPER MFT COPY
    memcpy((void*)arg_ptr, mft, mft_len);     
    *((char*)(arg_ptr + mft_len)) = '\0';     
    info->mft = arg_ptr - (uint64_t)mem;
    arg_ptr += mft_len + 1;

    // Check arguments fit in space and don't overlap text
    if (arg_ptr - (uint64_t)mem > MIN_TEXT_ADDR) 
    {
        LOG_VMM("cmdline + mft args too long, they overwrite program text\n");
        return false;
    }

    LOG_VMM("mem addr : %zu\n", mem);  
    LOG_VMM("mem_size: %zu\n", info->mem_size);
    LOG_VMM("boot_info vmm addr: %zu\n", (uint64_t)info);
    LOG_VMM("boot_info guest addr: %zu\n", (uint64_t)(info) - (uint64_t)mem);
    LOG_VMM("cpu_cycle_freq: %zu\n", info->cpu_cycle_freq);
    LOG_VMM("kernel_end guest addr: %zu\n", info->kernel_end);
    LOG_VMM("cmdline guest addr: %zu\n", info->cmdline);
    LOG_VMM("mft guest addr: %zu\n", info->mft);

    // Add arch IFDEFS here, if you want to support more archs in the future

    //TODO: Add stack protection based on max stack
    setup_memory_mapping(mem, mem_size);

    setup_system_registers(vcpu_id, mem_size);
    setup_tcb_registers(vcpu_id, p_entry, BOOT_INFO_ADDR);

    return true;
}
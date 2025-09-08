#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <microkit.h>
#include <solo5libvmm/aarch64/vcpu.h>
#include <solo5libvmm/util.h>
#include <solo5libvmm/hvt_abi.h>

char* fault_to_string(seL4_Word fault_label)
{
    switch (fault_label) 
    {
        case seL4_Fault_VMFault:
            return "Virtual memory";
        case seL4_Fault_UnknownSyscall:
            return "Unknown syscall";
        case seL4_Fault_UserException:
            return "User exception";
        case seL4_Fault_VGICMaintenance:
            return "VGIC maintenance";
        case seL4_Fault_VCPUFault:
            return "VCPU";
        case seL4_Fault_VPPIEvent:
            return "VPPI event";
        default:
            return "Unknown fault";
    }
}

static seL4_Word id_to_reg_val(seL4_Word reg_id, seL4_UserContext* regs)
{
    switch (reg_id) 
    {
        case 0:
            return regs->x0;
        case 1:
            return regs->x1;
        case 2:
            return regs->x2;
        case 3:
            return regs->x3;
        case 4:
            return regs->x4;
        case 5:
            return regs->x5;
        case 6:
            return regs->x6;
        case 7:
            return regs->x7;
        case 8:
            return regs->x8;
        case 9:
            return regs->x9;
        case 10:
            return regs->x10;
        case 11:
            return regs->x11;
        case 12:
            return regs->x12;
        case 13:
            return regs->x13;
        case 14:
            return regs->x14;
        case 15:
            return regs->x15;
        case 16:
            return regs->x16;
        case 17:
            return regs->x17;
        case 18:
            return regs->x18;
        case 19:
            return regs->x19;
        case 20:
            return regs->x20;
        case 21:
            return regs->x21;
        case 22:
            return regs->x22;
        case 23:
            return regs->x23;
        case 24:
            return regs->x24;
        case 25:
            return regs->x25;
        case 26:
            return regs->x26;
        case 27:
            return regs->x27;
        case 28:
            return regs->x28;
        case 29:
            return regs->x29;
        case 30:
            return regs->x30;
        case 31:
            // WZR
            return 0;
        default:
            LOG_VMM("Failed to decode register id, attempted to access invalid register index 0x%lx\n", reg_id);
            assert(0);
    }
}

static inline void advance_vcpu(size_t vcpu_id, seL4_UserContext* regs)
{
    regs->pc += 4;
    seL4_Error err = seL4_TCB_WriteRegisters(BASE_VM_TCB_CAP + vcpu_id, seL4_False, 0, 1, regs);
    assert(err == seL4_NoError);
} 

static seL4_Bool fault_handle_vm_exception(size_t vcpu_id)
{
    uint64_t addr = (uint64_t)microkit_mr_get(seL4_VMFault_Addr);
    uint64_t fsr = (uint64_t)microkit_mr_get(seL4_VMFault_FSR);
    seL4_Word ip = microkit_mr_get(seL4_VMFault_IP);
    seL4_Word is_prefetch = seL4_GetMR(seL4_VMFault_PrefetchFault);

    seL4_UserContext regs;
    seL4_Error err = seL4_TCB_ReadRegisters(BASE_VM_TCB_CAP + vcpu_id, false, 0, sizeof(seL4_UserContext) / sizeof(seL4_Word), &regs);
    assert(err == seL4_NoError);

    //LOG_VMM("VM FAULT\n");
    //LOG_VMM("addr: %ld\n", addr);
    //LOG_VMM("fsr: %ld\n", fsr);
    //LOG_VMM("ip: %ld\n", ip);
    //LOG_VMM("is_prefetch: %ld\n", is_prefetch);
    //LOG_VMM("hypercall number: %ld\n", HVT_HYPERCALL_NR(addr));#

    // add checks        
    uint8_t* mem = (uint8_t*)1073741824;
    uint8_t isv = (fsr >> 25) & 1;
    uint8_t il = (fsr >> 26) & 1;
    uint64_t src_reg = (fsr >> 16) & 31;
    uint64_t reg_data = (uint64_t)id_to_reg_val(src_reg, &regs);
    enum hvt_hypercall hc = HVT_HYPERCALL_NR(addr);

    if (hc == HVT_HYPERCALL_PUTS)
    {      
        struct hvt_hc_puts puts;        
        memcpy(&puts, mem + reg_data, sizeof(struct hvt_hc_puts));

        //LOG_VMM("valid isv: %ld\n", isv);
        //LOG_VMM("valid il: %ld\n", il);
        //LOG_VMM("src reg: %ld\n", src_reg);
        //LOG_VMM("reg value: %ld\n", reg_data);        
        //LOG_VMM("data ptr: %ld\n", puts.data);
        //LOG_VMM("data len: %ld\n", puts.len);
        for (uint64_t i = 0; i < puts.len; i++)
        {
            uint64_t data = *(mem + puts.data + i);
            PRINT_VMM("%c", (char)data);
            //PRINT_VMM("%c - %ld, ", (char)data, data);
        }
        //LOG_VMM("\n");

        advance_vcpu(vcpu_id, &regs);
        
        return seL4_True;
    } 
    
    if (hc == HVT_HYPERCALL_WALLTIME)
    {        
        struct hvt_hc_walltime* walltime = (struct hvt_hc_walltime*)(mem + reg_data);
        walltime->nsecs = 0;

        advance_vcpu(vcpu_id, &regs);

        return seL4_True;
    }
    
    if (hc == HVT_HYPERCALL_POLL)
    {
        struct hvt_hc_poll* poll = (struct hvt_hc_poll*)(mem + reg_data);
        poll->ready_set = 0;
        poll->ret = 0;

        //struct hvt_hc_poll pollcpy;
        //memcpy(&pollcpy, mem + reg_data, sizeof(struct hvt_hc_puts));        
        //LOG_VMM("Poll_cpy nsecs: %ld\n", pollcpy.timeout_nsecs);
        //LOG_VMM("Poll nsecs: %ld\n", poll->timeout_nsecs);        

        advance_vcpu(vcpu_id, &regs);

        return seL4_True;
    }
    
    if (hc == HVT_HYPERCALL_HALT)
    {
        struct hvt_hc_halt* poll = (struct hvt_hc_halt*)(mem + reg_data);

        LOG_VMM("Guest exited with code: %ld\n", poll->exit_status);

        microkit_vcpu_stop(vcpu_id);

        return seL4_True;
    }

    LOG_VMM("Hypercall number: %ld\n", (uint64_t)hc);
    LOG_VMM("Unexpected memory fault on address: 0x%lx, FSR: 0x%lx, IP: 0x%lx, is_prefetch: %s\n", addr, fsr, ip, is_prefetch ? "true" : "false");
    return seL4_False;
}

static seL4_Bool fault_handle_user_exception(size_t vcpu_id)
{
    size_t fault_ip = microkit_mr_get(seL4_UserException_FaultIP);
    size_t number = microkit_mr_get(seL4_UserException_Number);
    
    LOG_VMM("Invalid instruction fault at IP: 0x%lx, number: 0x%lx", fault_ip, number);
    microkit_vcpu_stop(vcpu_id);
    vcpu_print_tcb_regs(vcpu_id);
    vcpu_print_sys_regs(vcpu_id);

    return seL4_True;
}

seL4_Bool fault_handle(size_t vcpu_id, microkit_msginfo msginfo)
{
    seL4_Word label = microkit_msginfo_get_label(msginfo);
    seL4_Bool success = seL4_False;

    switch (label) 
    {
        case seL4_Fault_VMFault:
            success = fault_handle_vm_exception(vcpu_id);
            break;
        case seL4_Fault_UserException:
            success = fault_handle_user_exception(vcpu_id);
            break;
        default:
            LOG_VMM("Unexpected fault: %s / 0x%lx\n", fault_to_string(label), label);
    }

    if (!success) 
    {
        LOG_VMM("Failed to handle %s fault, stopping VCPU (ID 0x%lx)\n", fault_to_string(label), vcpu_id);
        microkit_vcpu_stop(vcpu_id);
        vcpu_print_tcb_regs(vcpu_id);
        vcpu_print_sys_regs(vcpu_id);
    }

    return success;
}
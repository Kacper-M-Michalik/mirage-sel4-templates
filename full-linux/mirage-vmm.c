#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <solo5libvmm/guest.h>
#include <solo5libvmm/fault.h>
#include <solo5libvmm/util.h>
#include <solo5libvmm/solo5/hvt_abi.h>
#include <util.h>

// Data for the guest's kernel image
extern uint8_t _binary_guest_start[];
extern uint8_t _binary_guest_end[];
extern size_t _binary_guest_size[]; // Doesn't work without [], even if set as size_t*

// Linked by microkit using system xml
uint8_t* guest_ram_vaddr;
size_t guest_ram_size;

void init(void)
{
    printf("Guest memory addr: %zu\n", guest_ram_vaddr);
    printf("Guest image addr: %ld\n", _binary_guest_start);
    printf("Guest image size linked: %ld\n", (size_t)_binary_guest_size);
    printf("Starting bootup\n");

    char cmdline[] = "";

    bool success = guest_setup(0, _binary_guest_start, (size_t)_binary_guest_size, guest_ram_vaddr, guest_ram_size, 0, cmdline, strlen(cmdline));
    printf("Load success: %d\n", success);

    if (success) guest_resume(0);
    else printf("Failed to load guest image\n");
}

void notified(microkit_channel ch)
{
    printf("Unexpected channel, ch: 0x%lx\n", ch);
}

seL4_Bool fault(microkit_child child, microkit_msginfo msginfo, microkit_msginfo *reply_msginfo) 
{    
    enum hvt_hypercall hc; 
    void* hc_data;

    // Handle the fault, if an unexpected fault has occured (i.e. anything other than a hypercall), the function will print appropriate error plus return false - which means the vcpu is not resumable
    // Your hypercall handler needs to seperately call guest_resume()
    bool was_handled = fault_handle(child, msginfo, guest_ram_vaddr, &hc, &hc_data, NULL);

    switch (hc)
    {
        case HVT_HYPERCALL_PUTS:
            struct hvt_hc_puts* puts = (struct hvt_hc_puts*)hc_data;     
            for (uint64_t i = 0; i < puts->len; i++)
            {
                uint64_t data = *(guest_ram_vaddr + puts->data + i);
                printf("%c", (char)data);
            }
            guest_resume(child);
            break;  
        case HVT_HYPERCALL_WALLTIME:
            struct hvt_hc_walltime* walltime = (struct hvt_hc_walltime*)hc_data;
            walltime->nsecs = 0;
            guest_resume(child);
            break;  
        case HVT_HYPERCALL_POLL:
            struct hvt_hc_poll* poll = (struct hvt_hc_poll*)hc_data;
            poll->ready_set = 0;
            poll->ret = 0;
            guest_resume(child);
            break;   
        case HVT_HYPERCALL_HALT:
            struct hvt_hc_halt* halt = (struct hvt_hc_halt*)hc_data;            
            LOG_VMM("Guest exited with code: %ld\n", halt->exit_status);
            break;     
        default:
            LOG_VMM("Reached impossible hypercall");
            assert(0);
            break;
    }

    // Reply that the fault was handled successfully, this is not the same as resuming the VCPU in our case, fault_handle stops the VCPU in the case of a valid hypercall, your hypercall handlers need to manually call guest_resume() at the end
    if (was_handled)
    {
        *reply_msginfo = microkit_msginfo_new(0, 0);
        return seL4_True;
    }
    return seL4_False;    
}
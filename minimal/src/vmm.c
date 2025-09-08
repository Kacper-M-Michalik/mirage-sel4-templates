#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <solo5libvmm/guest.h>
#include <solo5libvmm/fault.h>
#include <util.h>

// Data for the guest's kernel image
extern uint8_t _binary_guest_start[];
extern uint8_t _binary_guest_end[];
extern size_t _binary_guest_size[]; // Doesn't work without []

// Linked by microkit using system xml
uint8_t* guest_ram_vaddr;
size_t guest_ram_size;

void init(void)
{
    printf("Guest Memory Address: %zu\n", guest_ram_vaddr);
    printf("Guest image size linked: %ld\n", _binary_guest_size);

    char cmdline[] = "";

    bool success = guest_setup(0, _binary_guest_start, (size_t)_binary_guest_size, guest_ram_vaddr, guest_ram_size, 0, cmdline, strlen(cmdline));
    printf("Load success: %d\n", success);

    if (success)
    {
        guest_resume(0);
    }   
}

void notified(microkit_channel ch)
{
    printf("Unexpected channel, ch: 0x%lx\n", ch);
}

seL4_Bool fault(microkit_child child, microkit_msginfo msginfo, microkit_msginfo *reply_msginfo) 
{    
    seL4_Bool success = fault_handle(child, msginfo);
    
    //hvt_hypercall hc_id;
    //void* hc_data;
    //if (!fault_handle(child, msginfo, &hc_id, &hc_data)) return seL4_False;

    /*
    //Decode here

    switch ()
    {
    }
    */
    
    // Reply message to resume the guest, is sent depending on return boolean
    *reply_msginfo = microkit_msginfo_new(0, 0);
    return success;
}

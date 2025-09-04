#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <solo5libvmm/elf_solo5.h>
#include <solo5libvmm/guest.h>
#include <util.h>

/* Data for the guest's kernel image. */
extern uint8_t _binary_guest_start[];
extern uint8_t _binary_guest_end[];
// BE WARNED - removing [] gives incorrect value
extern size_t _binary_guest_size[];

uint8_t* guest_ram_vaddr;
size_t guest_ram_size = 0x10000000;

void init(void)
{
    printf("Hello, world\n");
    printf("Guest Memory Address: %zu\n", guest_ram_vaddr);
    printf("Guest image size linked: %zd\n", _binary_guest_size);
    printf("Guest image size calculated: %zd\n", _binary_guest_end - _binary_guest_start);

    size_t guest_size = (size_t)(*_binary_guest_size);
    
    //int *test = (int*)guest_ram_vaddr;
    ////*test = 100;
    printf("Test write: %zu\n", *test);
    
    uint64_t p_entry;
    uint64_t p_end;
    char cmdline[] = "cmdlinetest";
    char mft[] = "";

    bool success = guest_setup(0, _binary_guest_start, guest_size, guest_ram_vaddr, guest_ram_size, 0, cmdline, strlen(cmdline), mft, strlen(mft));
    printf("Load success: %d\n", success);

    guest_resume(0);
}

void notified(microkit_channel ch)
{
    printf("Unexpected channel, ch: 0x%lx\n", ch);
}

seL4_Bool fault(microkit_child child, microkit_msginfo msginfo, microkit_msginfo *reply_msginfo) 
{
    /*
    bool success = fault_handle(child, msginfo);
    if (!success) return seL4_False;

    //Decode here

    switch ()
    {
    }

    */
    printf("CAUGHT FAULT!\n");
    *reply_msginfo = microkit_msginfo_new(0, 0);
    return seL4_False;
    //return seL4_True;
}

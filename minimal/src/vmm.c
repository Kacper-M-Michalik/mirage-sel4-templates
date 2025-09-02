#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <printf.h>
#include <elf_solo5.h>

/* Data for the guest's kernel image. */
extern uint8_t _binary_guest_start[];
extern uint8_t _binary_guest_end[];

uint8_t* _guest_ram_vaddr;
size_t _guest_ram_size = 0x10000000;

void init(void)
{
    size_t guest_elf_size = _binary_guest_end - _binary_guest_start;

    printf("Hello, world\n");
    printf("Guest Memory Address: %zu\n", _guest_ram_vaddr);
    printf("Guest image size: %zu\n", guest_elf_size);
    
    int *test = (int*)_guest_ram_vaddr;
    *test = 100;
    printf("Test write: %zu\n", *test);
    
    uint64_t p_entry;
    uint64_t p_end;
    bool success = elf_load(_binary_guest_start, guest_elf_size, _guest_ram_vaddr, _guest_ram_size, 0x10000, &p_entry, &p_end);
    printf("Load success: %d\n", success);
    printf("p_entry: %zu\n", p_entry);
    printf("p_end: %zu\n", p_end);

    //solo5_guest_setup();
    //guest_start();
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

    *reply_msginfo = microkit_msginfo_new(0, 0);
    return seL4_True;
}

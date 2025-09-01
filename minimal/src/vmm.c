#include <stdint.h>
#include <microkit.h>

#include <printf.h>

/* Data for the guest's kernel image. */
extern char _binary_guest_start[];
extern char _binary_guest_end[];

void init(void)
{
    printf("Hello, world\n");
    printf("Guest image size: %zu", _binary_guest_end - _binary_guest_start);
}

void notified(microkit_channel ch)
{
    printf("Unexpected channel, ch: 0x%lx\n", ch);
}
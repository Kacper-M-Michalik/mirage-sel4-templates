#include <microkit.h>
#include <printf.h>

/* This is required to use the printf library. */
void _putchar(char character)
{
    microkit_dbg_putc(character);
}
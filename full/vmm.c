#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <sddf/util/util.h>
#include <sddf/util/printf.h>
#include <sddf/serial/queue.h>
#include <sddf/serial/config.h>
#include <sddf/timer/client.h>
#include <sddf/timer/config.h>
#include <solo5libvmm/guest.h>
#include <solo5libvmm/fault.h>
#include <solo5libvmm/solo5/hvt_abi.h>
#include <solo5libvmm/util.h>

// SDDF provides driver configs through named sections
__attribute__((__section__(".serial_client_config"))) serial_client_config_t serial_config;
__attribute__((__section__(".timer_client_config"))) timer_client_config_t timer_config;

// Handles to serial read and write queues respectively
serial_queue_handle_t rx_queue_handle;
serial_queue_handle_t tx_queue_handle;

// Data for the guest's kernel image
extern uint8_t _binary_guest_start[];
extern uint8_t _binary_guest_end[];
extern size_t _binary_guest_size[]; // Doesn't work without []

// Guest machine information
uint32_t guest_vcpu_id = 0;
uint8_t* guest_ram_vaddr = (uint8_t*)0x30000000;
size_t guest_ram_size= 0x10000000;
bool waiting_for_timeout = false;

// Provide printf() for solo5libvmm
int printf(const char *fmt, ...) 
{
    va_list args;
    va_start(args, fmt);
    int ret = sddf_vprintf(fmt, args);
    va_end(args);
    return ret;
}

void init(void)
{    
    // Initialise serial read and write queues
    assert(serial_config_check_magic(&serial_config));
    serial_queue_init(&rx_queue_handle, serial_config.rx.queue.vaddr, serial_config.rx.data.size, serial_config.rx.data.vaddr);
    serial_queue_init(&tx_queue_handle, serial_config.tx.queue.vaddr, serial_config.tx.data.size, serial_config.tx.data.vaddr);
    // Initialise serial write function
    serial_putchar_init(serial_config.tx.id, &tx_queue_handle);

    // Initialise timer
    assert(timer_config_check_magic(&timer_config));

    // Boot up guest
    sddf_printf("Guest memory addr: %zu\n", guest_ram_vaddr);
    sddf_printf("Guest image addr: %ld\n", _binary_guest_start);
    sddf_printf("Guest image size linked: %ld\n", _binary_guest_size);
    sddf_printf("Starting bootup\n");

    char cmdline[] = "";
    bool success = guest_setup(guest_vcpu_id, _binary_guest_start, (size_t)_binary_guest_size, guest_ram_vaddr, guest_ram_size, 0, cmdline, strlen(cmdline));
    sddf_printf("Load success: %d\n", success);

    if (success) guest_resume(guest_vcpu_id);
    else sddf_printf("Failed to load guest image\n");
}

void notified(microkit_channel ch)
{
    if (ch == serial_config.tx.id) return; // Write interrupt, we get this one time when we initialise the serial, but never see it again
    if (ch == serial_config.rx.id) return; // We got input from serial, ignore as solo5 doesn't support interrupt based input
    
    if (ch == timer_config.driver_id) // We got a notification about an elapsed timer, which we use to implement the poll hypercall
    {
        if (waiting_for_timeout) 
        {
            waiting_for_timeout = false;
            guest_resume(guest_vcpu_id);
        }
        else
        {
            LOG_VMM("Reached undefined state!\n"); 
        }        

        return;
    }
    
    LOG_VMM("Unexpected channel, ch: 0x%lx\n", ch);  
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
        case HVT_HYPERCALL_HALT:
            struct hvt_hc_halt* halt = (struct hvt_hc_halt*)hc_data;            
            LOG_VMM("Guest exited with code: %ld\n", halt->exit_status);
            break;    
                   
        case HVT_HYPERCALL_WALLTIME:
            struct hvt_hc_walltime* walltime = (struct hvt_hc_walltime*)hc_data;
            uint64_t time = sddf_timer_time_now(timer_config.driver_id);
            walltime->nsecs = time;
            guest_resume(guest_vcpu_id);
            break;  

        case HVT_HYPERCALL_POLL:
            struct hvt_hc_poll* poll = (struct hvt_hc_poll*)hc_data;
            poll->ready_set = 0;
            poll->ret = 0;
            waiting_for_timeout = true;
            sddf_timer_set_timeout(timer_config.driver_id, poll->timeout_nsecs);  
            break;   

        case HVT_HYPERCALL_PUTS:
            struct hvt_hc_puts* puts = (struct hvt_hc_puts*)hc_data;     
            for (uint64_t i = 0; i < puts->len; i++)
            {
                uint64_t data = *(guest_ram_vaddr + puts->data + i);
                sddf_printf("%c", (char)data);
            }
            guest_resume(guest_vcpu_id);
            break;  

        case HVT_HYPERCALL_BLOCK_READ:
        case HVT_HYPERCALL_BLOCK_WRITE:
        case HVT_HYPERCALL_NET_READ:
        case HVT_HYPERCALL_NET_WRITE:
        default:
            LOG_VMM("Reached unimplemented hypercall");
            was_handled = false;
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
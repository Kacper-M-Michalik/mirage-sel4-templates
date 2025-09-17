#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <sddf/util/printf.h>
#include <sddf/serial/queue.h>
#include <sddf/serial/config.h>
//#include <solo5libvmm/guest.h>
//#include <solo5libvmm/fault.h>
//#include <solo5libvmm/hvt_abi.h>
//#include <solo5libvmm/util.h>
#include <util.h>

// Data for the guest's kernel image
extern uint8_t _binary_guest_start[];
extern uint8_t _binary_guest_end[];
extern size_t _binary_guest_size[]; // Doesn't work without []

// Linked by microkit using system xml
uint8_t* guest_ram_vaddr;
size_t guest_ram_size;

// SDDF provides configs through named sections
__attribute__((__section__(".serial_client_config"))) serial_client_config_t config;

// Handles to serial read and write queues respectively
serial_queue_handle_t rx_queue_handle;
serial_queue_handle_t tx_queue_handle;

void init(void)
{    
    // Initialise read and write queues
    assert(serial_config_check_magic(&config));
    serial_queue_init(&rx_queue_handle, config.rx.queue.vaddr, config.rx.data.size, config.rx.data.vaddr);
    serial_queue_init(&tx_queue_handle, config.tx.queue.vaddr, config.tx.data.size, config.tx.data.vaddr);

    // Initialise serial write function
    serial_putchar_init(config.tx.id, &tx_queue_handle);

    // Boot up guest
    sddf_printf("Guest memory addr: %zu\n", guest_ram_vaddr);
    sddf_printf("Guest image addr: %ld\n", _binary_guest_start);
    sddf_printf("Guest image size linked: %ld\n", _binary_guest_size);
    sddf_printf("Starting bootup\n");

    char cmdline[] = "";
    bool success = 1; //guest_setup(0, _binary_guest_start, (size_t)_binary_guest_size, guest_ram_vaddr, guest_ram_size, 0, cmdline, strlen(cmdline));
    sddf_printf("Load success: %d\n", success);

    if (success) sddf_printf("\n");//guest_resume(0);
    else sddf_printf("Failed to load guest image\n");
}

void notified(microkit_channel ch)
{
    sddf_printf("Unexpected channel, ch: 0x%lx\n", ch);
}

seL4_Bool fault(microkit_child child, microkit_msginfo msginfo, microkit_msginfo *reply_msginfo) 
{    
    /*
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
            //LOG_VMM("Poll_cpy nsecs: %ld\n", pollcpy.timeout_nsecs);
            //LOG_VMM("Poll nsecs: %ld\n", poll->timeout_nsecs);        
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
        */
    return seL4_False;    
}

/*
char c;
    while (!serial_dequeue(&rx_queue_handle, &c)) {
        if (c == '\r') {
            sddf_putchar_unbuffered('\\');
            sddf_putchar_unbuffered('r');
        } else {
            sddf_putchar_unbuffered(c);
        }
        char_count++;
        if (char_count % 10 == 0) {
            sddf_printf("\n%s has received %u characters so far!\n", microkit_name, char_count);
        }
    }
*/
#include <stdint.h>
#include "hvt_abi.h"

// The hypercall structs pass pointers to necessary data, so you also need to create an additional method
// to access said data, either through mapping guests into the driver memory space, or adding a copy buffer
struct vm_request_buffer
{
    uint32_t reserved;
    uint32_t read_ready;
    enum hvt_hypercall hypercall_id;
    union hypercall_info
    {
        struct hvt_hc_walltime;
        struct hvt_hc_puts;
        struct hvt_hc_block_write;
        struct hvt_hc_block_read;
        struct hvt_hc_net_write;
        struct hvt_hc_net_read;
        struct hvt_hc_poll;
        struct hvt_hc_halt;
    };
};

void handle_hypercall(void);
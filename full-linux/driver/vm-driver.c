#include <stdint.h>
#include <stdbool.h>
#include "ipc.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/memremap.h>

// Module metadata
MODULE_AUTHOR("Kacper Michalik");
MODULE_DESCRIPTION("Driver for solo5 VM hypercalls");
//MODULE_LICENSE("");
MODULE_VERSION("1");

// IRQ signalling a VM has queued another hypercall to handle, can be set at module load time only
static int irq = 1;
module_param(irq, int, 0);


static irqreturn_t irq_handler(int irq, void *dev_id)
{
    printk(KERN_INFO "Interrupt %d triggered!\n", irq);

    // run through each request buffer, start thread for each with handle_hypercall, 
    // guests will just spin lock

    return IRQ_HANDLED;
}

static int __init driver_init(void) 
{
    dev_info(&pdev->dev, "VM hypercall driver loaded\n");

    struct device_node *rmem_np;
    struct reserved_mem *rmem;

    // Find hypercall request buffer
    rmem_np = of_find_node_by_name(NULL, "solo5-hypercall-buffer");
    if (!rmem_np) return -ENODEV;
    rmem = of_reserved_mem_lookup(rmem_np);
    of_node_put(rmem_np);
    if (!rmem) return -ENODEV;
    dev_info(&pdev->dev, "Found hypercall request buffer: base=%pa size=%pa\n", &rmem->base, &rmem->size);

    uint8_t* request_base = memremap(rmem->base, rmem->size, MEMREMAP_WB);
    if (!base) return -ENOMEM;
    
    vm_request_buffer ; 
    
    // Guest hypercalls pass around unknown lengths of data from their address space (puts or net or block requests), so we need a
    // to map in guest memory too
    // map in guests here?
    
    // Assign IRQ handler
    int ret = request_irq(irq, irq_handler, IRQF_SHARED, "irq_handler", (void *)(irq_handler));
    if (ret)
    {
        printk(KERN_ERR "Failed to request IRQ %d\n", irq);
        return ret;
    }

    return 0;
}

static void __exit driver_exit(void) 
{
    dev_info(&pdev->dev, "Exiting VM hypercall driver\n");
}

module_init(driver_init);
module_exit(driver_exit);
#include <asm/io.h>
#include "../../exp_trigger.h"

#undef pr_fmt
#define pr_fmt(fmt) "exp_trigger: " fmt

extern unsigned long secmon_start_virt;
extern unsigned long secmon_end_virt;

int do_sync_ext_abort(struct exp_item *item)
{
        void __iomem  *secmem_address;
        /*
         * check whether secmon driver is initialized
         */
        if (secmon_start_virt == 0x00) {
                return -EFAULT;
        }
        secmem_address = (void __iomem *)(secmon_start_virt +
					 ((secmon_end_virt - secmon_start_virt)/3)*2);

        pr_err("try to read secure memorry address: %p\n", secmem_address);
	
	/*
	 * 1. secure memory is external to MMU
	 * 2. this very addresss shall never be accessed by kernel before,
	 * aka. no cache in MMU, which means reading from this address will
	 * go directly into the main memory and this is synchronous
	 *
	 * condition 1 && 2 make it synchronous external abort
	 */
        readl(secmem_address);

        return 0;
}

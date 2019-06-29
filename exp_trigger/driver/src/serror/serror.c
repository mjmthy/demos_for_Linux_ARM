#include <asm/io.h>
#include "../exp_trigger.h"

#undef pr_fmt
#define pr_fmt(fmt) "exp_trigger: " fmt

extern unsigned long secmon_start_virt;
extern unsigned long secmon_end_virt;

int do_serror(void)
{
	void __iomem  *secmem_address;
	unsigned int  value = 0x99;
	/*
	 * check whether secmon driver is initialized
	 */
	if (secmon_start_virt == 0x00) {
		return -EFAULT;
	}
	secmem_address = (void __iomem *)(secmon_start_virt +
			 		 ((secmon_end_virt-secmon_start_virt)*2)/3);

	pr_err("try to write 0x%x to secure memorry address: %p\n", value, secmem_address);

	/*
	 * 1. for MMU, secure memory is external
	 * 2. the mapping for secure memory is cacheable, which
	 *    means the updating of memory area pointed to by
	 *    secmem_address will first take effect in the cache,
	 *    and late flushed to memory, aka. asynchronous
	 * 
	 *    condition 1 && 2 makes it SError
	 */
	writel(value, secmem_address);

	return 0;
}

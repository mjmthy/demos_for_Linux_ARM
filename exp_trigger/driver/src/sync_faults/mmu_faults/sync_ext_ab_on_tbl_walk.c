#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/cpu.h>
#include <asm/pgtable.h>
#include <asm/tlbflush.h>
#include <asm/io.h>
#include <linux/amlogic/iomap.h>
#include "../../exp_trigger.h"
#include "translation_table_helper.h"

extern void __iomem *meson_reg_map[IO_BUS_MAX];
/*
 * Need to update the definition for CPU series other
 * than gxl
 */
#define SEC_AO_RTI_STATUS_REG0 (0xda100000 + (0x00 << 2))

int do_sync_ext_ab_on_tbl_walk(struct exp_item *item)
{
	pmd_t *pmd;
	pmd_t new_pmd;
	unsigned long addr;
	unsigned int val;
	struct mm_struct *mm = &init_mm;
	int i;

	/*
	 * shut down all other cpus other than cpu 0,
	 * so that we will not be disturbed by
	 * other cpus
	 */
	for (i=1; i<num_possible_cpus(); i++)
		cpu_down(i);

	addr = (unsigned long)(meson_reg_map[IO_VAPB_BUS_BASE] + 0x470);
	pmd = get_tbl_ent(mm, addr, TABLE_ENTRY_PMD);
        if (pmd == NULL) {
		pr_err("no valid pmd found\n");
		for (i=1; i<num_possible_cpus(); i++)
			cpu_up(i);
		return -EFAULT;
        }
        pr_info("pmd value: 0x%llx\n", pmd_val(*pmd));

	/*
	 * force to access secure register when doing 
	 * level 2 translation table walk
	 * note: this access is on ao bus, outside of MMU,
	 *       which makes it an external access
	 */
	new_pmd.pmd = (pmd_val(*pmd) & 0xFFFF000000000FFF) | SEC_AO_RTI_STATUS_REG0;
	/*
	 * force to access secure memory has the same effect
	 */
	//new_pmd.pmd = (pmd_val(*pmd) & 0xFFFF000000000FFF) | 0x5200000;
	local_irq_disable();
	set_pmd(pmd, new_pmd);
        pr_info("new pmd value: 0x%llx\n", pmd_val(*pmd));
	flush_tlb_mm(mm);

	pr_info("try to read data from addr: 0x%lx\n", addr);
	val = readl((void __iomem *)addr);

	/*
	 * actually we'll never reach here
	 */
	local_irq_enable();

	for (i=1; i<num_possible_cpus(); i++)
		cpu_up(i);

	return 0;
}

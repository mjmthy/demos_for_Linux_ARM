#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include <asm/tlbflush.h>
#include <asm/io.h>
#include "../../exp_trigger.h"
#include "../../sysreg.h"

#undef pr_fmt
#define pr_fmt(fmt) "exp_trigger: " fmt

void __iomem *d_mem = NULL;

static int do_user_DA_alignment_fault(void)
{
	return -EINVAL;
}

static int do_user_PC_alignment_fault(void)
{
	return -EINVAL;
}

static int do_user_SP_alignment_fault(void)
{
	return -EINVAL;
}

static int do_kernel_DA_alignment_fault(void)
{
	/*
	 * From ARM Architecture Reference Manual
	 *     ARMv8, for ARMv8-A architecture profile
	 *     B2.4 Alignment support
	 * We know
	 *     1. An unaligned access to any type of Device memory causes an 
	 *        alignment fault
	 *     2. For normal memoey, SCTLR_EL1.A == 1 and the address that is
	 *        accessed is not aligned to the size of the data element being
	 *        accessed
	 */

	/*
	 * memory-mapped peripherals and I/O locations are typical examples of
	 * memory area that has device attribute, which is sutiable for triggering
	 * data access alignment fault
	 */
	void __iomem *unalign_d_mem = NULL;

	if (!d_mem) {
		pr_err("no suitable memory with device attribute\n");
		return -EFAULT;
	}
	pr_info("d_mem: %p\n", d_mem);
	unalign_d_mem = (unsigned char __iomem *)d_mem + 1;
	pr_info("unalign_d_mem: %p\n", unalign_d_mem);

	readl(unalign_d_mem);
	return 0;
}

static noinline void test_func(void)
{
        asm("nop;");
        asm("nop;");
        asm("nop;");
        asm("nop;");
}

static int do_kernel_PC_alignment_fault(void)
{
	/*
	 * From ARM Architecture Reference Manual
	 *     ARMv8, for ARMv8-A architecture profile
	 *     B2.4 Alignment support
	 * We know
	 *     Attempting to execute an A64 instructon that is not
	 *     word-aligned generates an PC alignment fault
	 */
	void *test_addr = test_func;
	void *unalign_pc;
	
	pr_info("test_addr: %p\n", test_addr);
	unalign_pc = (unsigned char *)test_addr + 0x10 + 0x1;
	pr_info("unalign_pc: %p\n", unalign_pc);

	asm("blr %0"::"r"(unalign_pc));
	
	return 0;
}

extern void sp_alignment_fault(void);

static int do_kernel_SP_alignment_fault(void)
{
	/*
	 * From ARM Architecture Reference Manual
	 *     ARMv8, for ARMv8-A architecture profile
	 *     D1.8.2 Stack alignment checking
	 * We know
	 *     1. SCTLR_EL1.SA == 0
	 *     2. The bits[3:0] of stack pointer is not not 0b0000, aka.
	 *        not quad-word aligned
	 *     3. This very unaligned stack pointer is used as the base
	 *        address of calculation
	 */
#if 0
	unsigned long spsel;
	unsigned long sp;

	spsel = read_sysreg(spsel);
	pr_info("spsel: 0x%lx\n", spsel);
	if ((spsel & SPSEL_SP) == SPSEL_SP) {
		pr_info("Use SP_ELx for Exception level ELx\n");
		asm ("mrs %0, sp_el1" : "=r" (sp));;
	}else {
		asm ("mrs %0, sp_el0" : "=r" (sp));
		pr_info("Use SP_EL0 at all Exception levels\n");
	}
	pr_info("current sp: 0x%lx\n", sp);
#endif
	//local_irq_disable();
	pr_info("!!! --------attention------ !!!\n");
	pr_info("  when SP alignment happens, if CONFIG_AMLOGIC_VMAP is not\n");
	pr_info("enabled, the exception entry still uses the original stack,\n");
	pr_info("Which means the unaligned sp is still used as the base sp\n");
	pr_info("for calculation, causing iteration of SP alignment faults.\n");
	pr_info("And the OS will never have the chance to execute do_sp_pc_abort,\n");
	pr_info("which means we cannot see any meaningful logs shows up at terminal.\n");
	pr_info("If you really wanna to verify do_kernel_SP_alignment_fault, please use ICE :-)\n");
	pr_info("!!! ----------------------- !!!\n");
	sp_alignment_fault();
	//local_irq_enable();

	return 0;
}

int do_alignment_fault(struct exp_item *item)
{
	unsigned int  mode;
	unsigned int  sub_type;
	unsigned long sctlr_el1;

	sctlr_el1 = read_sysreg(sctlr_el1);
        pr_info("sctlr_el1: 0x%lx\n", sctlr_el1);

	if ((sctlr_el1 & SCTLR_ELx_A) == SCTLR_ELx_A)
		pr_info("data access aligment checking for normal memory is enabled\n");
	else
		pr_info("data access aligment checking for normal memory is disabled\n");
	if ((sctlr_el1 & SCTLR_ELx_SA) == SCTLR_ELx_SA)
		pr_info("SP aligment checking for EL1 is enabled\n");
	else
		pr_info("SP aligment checking for EL1 is disabled\n");
	if ((sctlr_el1 & SCTLR_EL1_SA0) == SCTLR_EL1_SA0)
		pr_info("SP aligment checking for EL0 is enabled\n");
	else
		pr_info("SP aligment checking for EL0 is disabled\n");

	mode = item->e_case.mode;
	sub_type = item->e_case.sub_t;
	
	if (mode >= EXP_CASE_M_MAX)
		return -EINVAL;

	switch (sub_type) {
	case EXP_CASE_SUBT_DATA_UNALIGN:
		if (mode == EXP_CASE_M_KERNEL)
			return do_kernel_DA_alignment_fault();
		else
			return do_user_DA_alignment_fault();
	case EXP_CASE_SUBT_INSTR_UNALIGN:
		if (mode == EXP_CASE_M_KERNEL)
			return do_kernel_PC_alignment_fault();
		else
			return do_user_PC_alignment_fault();
	case EXP_CASE_SUBT_SP_UNALIGN:
		if (mode == EXP_CASE_M_KERNEL)
			return do_kernel_SP_alignment_fault();
		else
			return do_user_SP_alignment_fault();
	default:
		return -EINVAL;
	}

	return 0;
}

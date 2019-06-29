#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include <asm/pgtable.h>
#include <asm/tlbflush.h>
#include "../../exp_trigger.h"
#include "../../sysreg.h"
#include "translation_table_helper.h"

#undef pr_fmt
#define pr_fmt(fmt) "exp_trigger: " fmt

#if 0
/*
 * make this code page aligned and contaning exactly a whole page
 */
static noinline void __attribute__ ((aligned (PAGE_SIZE))) test_func(void)
{
	/*
	 * TBD: add more nop calling
	 *      to fulfill one page
	 */
	asm("nop;");
	asm("nop;");
	asm("nop;");
}
#endif

u32 test_func[] = {
	0x52800260, /* mov w0, #0x13 */
	0xd65f03c0, /* ret */
	0xd503201f, /* nop */
	0xd503201f  /* nop */
};

#if 0
static int do_user_i_permission_fault(int lvl, union extra_data *data)
{
	pr_info("does not support user space instruction fectching permission fault\n");

	return -EINVAL;
}
#endif

/*
 * Actually changing the AP domain int PTE alone(without updating VMA
 * R/W permission property), can trigger user level 3 permission fault(
 * via checking ESR_EL1, can assure this), but do_page_fault can not
 * handle this(it regards this as a normal access) as expected. And the
 * permission fault will be triggered without being handled. To avoid 
 * such situation, We have to make AP domain and VMA right consistent.
 *
 * Considering user space text section are born with READ ONLY AP domain
 * and VMA permission property, we can use a much easier way to trigger
 * such exception: try to write to user space text section in user space 
 */
static int do_user_d_permission_fault(int lvl, union extra_data *data)
{
	int ret;
	union extra_data e_data;
	pte_t *pte = NULL;
	pte_t new_pte;
	struct mm_struct *mm = current->mm;

	pr_info("current task: %s\n", current->comm);
	ret = copy_from_user(&e_data, data, sizeof(e_data));
	if (ret !=0)
		return ret;
	pr_info("addr_in: 0x%lx\n", e_data.addr_in);

	switch (lvl) {
	case 0:
	case 1:
	case 2:
		pr_err("does not support user space level 0~2 permission fault\n");
		return -EINVAL;
	case 3:
		pte = get_tbl_ent(mm, e_data.addr_in, TABLE_ENTRY_PTE);
		if (pte == NULL) {
			pr_err("not a valid pte\n");
			return -EFAULT;
		}
		pr_info("pte value: 0x%llx\n", pte_val(*pte));
		new_pte.pte = pte_val(*pte)|PTE_RDONLY;
                set_pte(pte, new_pte);
		pr_info("new pte value: 0x%llx\n", pte_val(*pte));
		break;
	default:
		return -EINVAL;
	}

	/*
	 * flush the current task related TLB entries,
	 * to make sure the table entry we accessed
	 * comes directly from translation table, not
	 * from TLB
	 */
	flush_tlb_mm(mm);
	barrier();

	return 0;
}

static int do_kernel_i_permission_fault(int lvl)
{
	unsigned char *p = NULL;
        struct mm_struct *mm = &init_mm;
        pte_t *pte;
	pte_t new_pte;
	pmd_t *pmd;
	int test_val;
	int ret;

	switch (lvl) {
	case 0:
	case 1:
		pr_err("does not support kernel level 0~1 instruction fetching permission fault\n");
		return -EINVAL;
	case 2:
		p = kmalloc(PTRS_PER_PTE*PAGE_SIZE, GFP_KERNEL);
		if (!p) {
			pr_err("fail to allocate one level 2 block\n");
			return -ENOMEM;
		}
		pmd = get_tbl_ent(mm, (unsigned long)p, TABLE_ENTRY_PMD);
		if (!pmd) {
			pr_err("no valid pmd found\n");
			kfree(p);
			return -EFAULT;
		}
                pr_info("pmd value: 0x%llx\n", pmd_val(*pmd));

		/*
		 * Fill it with instructions from test_func
		 */
		memset(p, 0x00, PTRS_PER_PTE*PAGE_SIZE);
		memcpy(p, test_func, sizeof(test_func));
	
		/*
		 * by default, the memory allocated by kmalloc is UNX and PNX,
		 * we make it PX first to verify the executable level 2 memory
		 * block is really executable
		 */
		ret = set_mem_x(mm, (unsigned long)p, TABLE_ENTRY_PMD, 1);
		if (ret) {
			pr_err("fail to make this memory block executable\n");
			kfree(p);
			return ret;
		}
		flush_tlb_all();
		test_val = ((int (*)(void))p)();
		if (test_val != 0x13) {
			pr_err("Please check fail to execute the executable memory block\n");
			kfree(p);
			return -EFAULT;
		}
		
		/*
		 * Make it unexecutable again
		 */
		ret = set_mem_x(mm, (unsigned long)p, TABLE_ENTRY_PMD, 0);
		if (ret) {
			pr_err("fail to make this memory block unexecutable\n");
			kfree(p);
			return ret;
		}
		flush_tlb_all();

		pr_info("execute memory block starting at: %p\n", p);
		test_val = ((int (*)(void))p)();
		//pr_info("test_value: %d\n", test_val);

		kfree(p);
		break;
	case 3:
		p = vmalloc_exec(PAGE_SIZE);
		if (!p) {
			pr_err("fail to allocate one executable page\n");
			return -ENOMEM;
		}
                pte = get_tbl_ent(mm, (unsigned long)p, TABLE_ENTRY_PTE);
                if (pte == NULL) {
                        pr_err("no valid pte found\n");
                        vfree(p);
                        return -EFAULT;
                }
                pr_info("pte value: 0x%llx\n", pte_val(*pte));
	
		/*
		 * Fill the executable page with instructions from
		 * test_func
		 */
		memset(p, 0x00, PAGE_SIZE);
		memcpy(p, test_func, sizeof(test_func));

		new_pte.pte = pte_val(*pte)|PTE_PXN;
		set_pte(pte, new_pte);
                pr_info("new pte value: 0x%llx\n", pte_val(*pte));
		flush_tlb_all();

		/*
	 	 * Execute the instructions in this very
	 	 * executable page
	 	 */
		pr_info("execute executable page starting at: %p\n", p);
		test_val = ((int (*)(void))p)();
		//pr_info("test_value: %d\n", test_val);

		vfree(p);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int do_kernel_d_permission_fault(int lvl)
{
	unsigned char *p = NULL;
        struct mm_struct *mm = &init_mm;
        pte_t *pte;
	pte_t new_pte;

	switch (lvl) {
	case 0:
	case 1:
	case 2:
		pr_err("does not support kernel level 0~2 data accessing permission fault\n");
		return -EINVAL;
	case 3:
		p = vmalloc(PAGE_SIZE);
                if (p == NULL) {
                        pr_err("fail to alloc 1 Page via vmalloc\n");
                        return -ENOMEM;
                }
                pte = get_tbl_ent(mm, (unsigned long)p, TABLE_ENTRY_PTE);
                if (pte == NULL) {
                        pr_err("no valid pte found\n");
                        vfree(p);
                        return -EFAULT;
                }
                pr_info("pte value: 0x%llx\n", pte_val(*pte));

		/*
		 * from ARM Architecture Reference Manual
		 *     ARMv8, for ARMv8-A architecture profile
		 *     D5.4.4 Data access permission controls
		 *
		 *     AP[2:1] Access from higher
		 *     	       Exception Level     Access from EL0
		 *     00      read/write          none
		 *     01      read/write          read/write
		 *     10      read-only           none
		 *     11      read-only           read-only
		 */
		new_pte.pte = pte_val(*pte)|PTE_RDONLY;
		set_pte(pte, new_pte);
                pr_info("new pte value: 0x%llx\n", pte_val(*pte));
		flush_tlb_all();

		pr_err("The start address of allocated page: %p\n", p);
		p[0] = 0x12;
		vfree(p);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int do_permission_fault(struct exp_item *item)
{
        int ret = 0;
        int mode;
        int lvl;
	unsigned char trigg_src;
	unsigned long sctlr_el1;

        ret = check_param(item);
        if (ret != 0)
                return ret;

	/*
	 * Make sure the WXN bit of SCTLR_EL1 is 0,
	 * so that it will not overwrite the permission
	 * control related bits in block/page descriptor
	 */
	sctlr_el1 = read_sysreg(sctlr_el1);
	pr_info("sctlr_el1: 0x%lx\n", sctlr_el1);
	if ((sctlr_el1 & (1 << SCTLR_EL1_WXN_OFFSET)) != 0)
		write_sysreg(sctlr_el1 | (1 << SCTLR_EL1_WXN_OFFSET), sctlr_el1);

	/*
	 * Improper data accessing and instruction fetching
	 * can both trigger permission fault. So here we just
	 * flip a coin to decide the triggering source
	 */
	get_random_bytes(&trigg_src, sizeof(trigg_src));
	if (trigg_src % 2 == 0) {
		pr_info("triggering source is data accessing\n");
		trigg_src = 0;
	} else {
		pr_info("triggering source is instruction fetching\n");
		trigg_src= 1;
	}
	
        mode = item->e_case.mode;
        lvl = item->e_case.lvl;
        if (mode == EXP_CASE_M_KERNEL) {
		if (trigg_src)
                	return do_kernel_i_permission_fault(lvl);
		else
			return do_kernel_d_permission_fault(lvl);
        } else {
			return do_user_d_permission_fault(lvl, item->data);
        }
}

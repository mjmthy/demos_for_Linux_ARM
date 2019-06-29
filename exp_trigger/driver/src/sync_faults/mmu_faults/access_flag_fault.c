#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <asm/pgtable.h>
#include <asm/tlbflush.h>
#include "../../exp_trigger.h"
#include "translation_table_helper.h"

#undef pr_fmt
#define pr_fmt(fmt) "exp_trigger: " fmt

static int do_user_access_flag_fault(int lvl, union extra_data *data)
{
	int ret;
	union extra_data e_data;
	pte_t *pte = NULL;
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
		pr_err("does not support user space level 0~2 access flag fault\n");
		return -EINVAL;
	case 3:
		pte = get_tbl_ent(mm, e_data.addr_in, TABLE_ENTRY_PTE);
		if (pte == NULL) {
			pr_err("not a valid pte\n");
			return -EFAULT;
		}
		pr_info("pte value: 0x%llx\n", pte_val(*pte));
		/*
		 * make sure this page is unaccessed(old)
		 */
		if (pte_young(*pte))
			set_pte(pte, pte_mkold(*pte));
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

static int do_kernel_access_flag_fault(int lvl)
{
	unsigned char *p = NULL;
	struct mm_struct *mm = &init_mm;
	pte_t *pte;
	pmd_t *pmd;

	switch (lvl) {
	case 0:
	case 1:
		pr_err("does not support kernel space level 0~1 access flag fault\n");
		return -EINVAL;
	case 2:
		/*
		 * To allocate a Level 2 memory block
		 */
		p = kmalloc(PTRS_PER_PTE*PAGE_SIZE, GFP_KERNEL);
		if (p == NULL) {
			pr_err("fail to alloc 1 mem block via kmalloc\n");
			return -ENOMEM;
		}
		pmd = get_tbl_ent(mm, (unsigned long)p, TABLE_ENTRY_PMD);
		if (pmd == NULL) {
			pr_err("no valid pmd found\n");
			kfree(p);
			return -EFAULT;
		}
		pr_info("pmd value: 0x%llx\n", pmd_val(*pmd));
		/*
		 * make sure this block is unaccessed(old)
		 */
		if (pmd_young(*pmd))
			set_pmd(pmd, pmd_mkold(*pmd));
		/*
		 * trigger kernel level 2 access flag fault
		 */
		pr_err("The start address of allocated block: %p\n", p);
		p[0] = 0x12;
		kfree(p);
		break;
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
		 * make sure this page is unaccessed(old)
		 */
		if (pte_young(*pte))
			set_pte(pte, pte_mkold(*pte));
		/*
		 * trigger kernel level 3 access flag fault
		 */
		pr_err("The start address of allocated page: %p\n", p);
		p[0] = 0x12;
		vfree(p);
		break;
	default:
		return -EINVAL;	
	}

	return 0;
}

int do_access_flag_fault(struct exp_item *item)
{
	int ret = 0;
	int mode;
	int lvl;

	ret = check_param(item);
	if (ret != 0)
		return ret;

	mode = item->e_case.mode;
	lvl = item->e_case.lvl;
	if (mode == EXP_CASE_M_KERNEL) {
		return do_kernel_access_flag_fault(lvl);
	} else {
		return do_user_access_flag_fault(lvl, item->data);
	}
}

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <asm/pgtable.h>
#include "../../exp_trigger.h"
#include "translation_table_helper.h"

#undef pr_fmt
#define pr_fmt(fmt) "exp_trigger: " fmt

static int do_kernel_translation_fault(int lvl)
{
	pte_t *pte = NULL;
	pmd_t *pmd = NULL;
	pud_t *pud = NULL;
	struct index_g ig;
	unsigned long start;
	unsigned long end;
	unsigned long fault_trigger_address;

	switch (lvl) {
	case 0:
		if (get_ttbr_range(1, &start, &end) != 0)
			return -EFAULT;
		pr_info("start: 0x%lx, end: 0x%lx\n", start, end);
		fault_trigger_address = start - 0x1000;
		break;
	case 1:
		pud = first_cond_pud(&init_mm, 0, &ig);
		if (!pud)
			return -EFAULT;
#if CONFIG_PGTABLE_LEVELS > 3
		fault_trigger_address = 0xffffff8000000000 |
					(((unsigned long)ig.pgd_i) << PGDIR_SHIFT) |
					(((unsigned long)ig.pud_i) << PUD_SHIFT);
#else
		fault_trigger_address = 0xffffff8000000000 |
					(((unsigned long)ig.pud_i) << PUD_SHIFT);
#endif
		break;
	case 2:
		pmd = first_cond_pmd(&init_mm, 0, &ig);
		if (!pmd)
			return -EFAULT;
#if CONFIG_PGTABLE_LEVELS > 3
		fault_trigger_address = 0xffffff8000000000 |
					(((unsigned long)ig.pgd_i) << PGDIR_SHIFT) |
					(((unsigned long)ig.pud_i) << PUD_SHIFT) |
					(((unsigned long)ig.pmd_i) << PMD_SHIFT);
#else
		fault_trigger_address = 0xffffff8000000000 |
					(((unsigned long)ig.pgd_i) << PGDIR_SHIFT) |
					(((unsigned long)ig.pmd_i) << PMD_SHIFT);
#endif
		break;
	case 3:
		pte = first_cond_pte(&init_mm, 0, &ig);
		if (!pte)
			return -EFAULT;
#if CONFIG_PGTABLE_LEVELS > 3
		fault_trigger_address = 0xffffff8000000000 |
					(((unsigned long)ig.pgd_i) << PGDIR_SHIFT) |
					(((unsigned long)ig.pud_i) << PUD_SHIFT) |
					(((unsigned long)ig.pmd_i) << PMD_SHIFT) |
					(((unsigned long)ig.pte_i) << PAGE_SHIFT);
#else
		fault_trigger_address = 0xffffff8000000000 |
					(((unsigned long)ig.pgd_i) << PGDIR_SHIFT) |
					(((unsigned long)ig.pmd_i) << PMD_SHIFT) |
					(((unsigned long)ig.pte_i) << PAGE_SHIFT);
#endif

		break;
	default:
		return -EINVAL;
	}

	pr_info("fault trigger address: 0x%lx\n", fault_trigger_address);
	*((unsigned long *)fault_trigger_address) = 0x1234;

	return 0;
}

static int do_user_translation_fault(int lvl, union extra_data *data)
{
	int ret = 0;
	pte_t *pte = NULL;
	pmd_t *pmd = NULL;
	pud_t *pud = NULL;
	struct index_g ig;
	unsigned long start;
	unsigned long end;
	unsigned long fault_trigger_address = 0x00;
	union extra_data e_data;
	struct mm_struct *mm = current->mm;

	switch (lvl) {
	case 0:
		if (get_ttbr_range(0, &start, &end) != 0)
                        return -EFAULT;
                pr_info("start: 0x%lx, end: 0x%lx\n", start, end);
                fault_trigger_address = end + 0x1000;
                break;
	case 1:
		pud = first_cond_pud(mm, 0, &ig);
		if (!pud)
			return -EFAULT;
#if CONFIG_PGTABLE_LEVELS > 3
		fault_trigger_address = 0x0000000000000000 |
					(((unsigned long)ig.pgd_i) << PGDIR_SHIFT) |
					(((unsigned long)ig.pud_i) << PUD_SHIFT);
#else
		fault_trigger_address = 0x0000000000000000 |
					(((unsigned long)ig.pud_i) << PUD_SHIFT);
#endif
		break;
	case 2:
		pmd = first_cond_pmd(mm, 0, &ig);
		if (!pmd)
			return -EFAULT;
#if CONFIG_PGTABLE_LEVELS > 3
		fault_trigger_address = 0x0000000000000000 |
					(((unsigned long)ig.pgd_i) << PGDIR_SHIFT) |
					(((unsigned long)ig.pud_i) << PUD_SHIFT) |
					(((unsigned long)ig.pmd_i) << PMD_SHIFT);
#else
		fault_trigger_address = 0x0000000000000000 |
					(((unsigned long)ig.pgd_i) << PGDIR_SHIFT) |
					(((unsigned long)ig.pmd_i) << PMD_SHIFT);
#endif
		break;
	case 3:
		pte = first_cond_pte(mm, 0, &ig);
		if (!pte)
			return -EFAULT;
#if CONFIG_PGTABLE_LEVELS > 3
		fault_trigger_address = 0x0000000000000000 |
					(((unsigned long)ig.pgd_i) << PGDIR_SHIFT) |
					(((unsigned long)ig.pud_i) << PUD_SHIFT) |
					(((unsigned long)ig.pmd_i) << PMD_SHIFT) |
					(((unsigned long)ig.pte_i) << PAGE_SHIFT);
#else
		fault_trigger_address = 0x0000000000000000 |
					(((unsigned long)ig.pgd_i) << PGDIR_SHIFT) |
					(((unsigned long)ig.pmd_i) << PMD_SHIFT) |
					(((unsigned long)ig.pte_i) << PAGE_SHIFT);
#endif
		break;
	default:
		return -EINVAL;
	}

	pr_info("fault trigger address: 0x%lx\n", fault_trigger_address);
	e_data.addr_out = fault_trigger_address;
	ret = copy_to_user(data, &e_data, sizeof(e_data));
	if (!ret)
		return ret;

	return 0;
}

int do_translation_fault(struct exp_item *item)
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
		return do_kernel_translation_fault(lvl);
	} else {
		return do_user_translation_fault(lvl, item->data);
	}
}

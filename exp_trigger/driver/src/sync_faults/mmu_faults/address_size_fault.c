#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/cpu.h>
#include <asm/tlbflush.h>
#include "../../exp_trigger.h"
#include "translation_table_helper.h"

#undef pr_fmt
#define pr_fmt(fmt) "exp_trigger: " fmt

static int do_user_address_size_fault(int lvl, union extra_data *data)
{
	int ret;
        union extra_data e_data;
        pte_t *pte = NULL;
	pmd_t *pmd = NULL;
	int oas;
	unsigned long ttbr0_el1;
	unsigned long ttbr0_el1_tmp;
        struct mm_struct *mm = current->mm;

	oas = get_output_address_size();
	pr_info("output address size: %d\n", oas);
	if (oas == 48) {
		pr_info("we cannot trigger address size fault when the output \
			 address size is 48(the maximum output address size supported)\n");
		return -EINVAL;
	}

        pr_info("current task: %s\n", current->comm);
        ret = copy_from_user(&e_data, data, sizeof(e_data));
        if (ret !=0)
                return ret;
        pr_info("addr_in: 0x%lx\n", e_data.addr_in);

	switch (lvl) {
	case 0:
		ttbr0_el1 = read_sysreg(ttbr0_el1);
		pr_info("ttbr0_el1: 0x%lx\n", ttbr0_el1);
		ttbr0_el1_tmp = ttbr0_el1 | ((unsigned long)1 << (oas+2));
		pr_info("new ttbr0_el1: 0x%lx\n", ttbr0_el1_tmp);
		write_sysreg(ttbr0_el1_tmp, ttbr0_el1);
		break;
	case 1:
		pr_info("!!! Does not support triggring user space level 1 address size fault !!! \n");
		return -EINVAL;
	case 2:
		pmd = get_tbl_ent(mm, e_data.addr_in, TABLE_ENTRY_PMD);
		if (pmd == NULL) {
			pr_err("not a valid pmd\n");
			return -EFAULT;
		}
		pr_info("pmd value: 0x%llx\n", pmd_val(*pmd));
		pmd->pmd = pmd_val(*pmd) | ((unsigned long)1 << (oas+2));
		pr_info("new pmd value: 0x%llx\n", pmd_val(*pmd));
		break;
	case 3:
		pte = get_tbl_ent(mm, e_data.addr_in, TABLE_ENTRY_PTE);
		if (pte == NULL) {
			pr_err("not a valid pte\n");
			return -EFAULT;
		}
		pr_info("pte value: 0x%llx\n", pte_val(*pte));
		pte->pte = pte_val(*pte) | ((unsigned long)1 << (oas+2));
                //smp_mb();
		pr_info("new pte value: 0x%llx\n", pte_val(*pte));

		break;
	default:
		return -EINVAL;
	}

	flush_tlb_mm(mm);
	//barrier();

	return 0;
}

static int do_kernel_address_size_fault(int lvl)
{
	unsigned char *p = NULL;
        struct mm_struct *mm = &init_mm;
	int oas;
	pte_t *pte;
	pmd_t *pmd;
	unsigned long ttbr1_el1;
	unsigned long ttbr1_el1_tmp;
	unsigned long fault_trigger_var;

	oas = get_output_address_size();
	pr_info("output address size: %d\n", oas);
	if (oas == 48) {
		pr_info("we cannot trigger address size fault when the output \
			 address size is 48(the maximum output address size supported)\n");
		return -EINVAL;
	}

	switch (lvl) {
	case 0:
		ttbr1_el1 = read_sysreg(ttbr1_el1);
		pr_err("ttbr1_el1: 0x%lx\n", ttbr1_el1);
		ttbr1_el1_tmp = ttbr1_el1 | ((unsigned long)1 << (oas+2));
		pr_err("new ttbr1_el1: 0x%lx\n", ttbr1_el1_tmp);
		write_sysreg(ttbr1_el1_tmp, ttbr1_el1);
		smp_mb();
		fault_trigger_var = 0x1234;
		break;
	case 1:
		pr_info("!!! Does not support triggring kernel space level 1 address size fault !!! \n");
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

		pmd->pmd = pmd_val(*pmd) | ((unsigned long)1 << (oas+2));
		//smp_mb();
                pr_info("new pmd value: 0x%llx\n", pmd_val(*pmd));
		flush_tlb_mm(mm);

		pr_info("try to access %p\n", &p[0]);
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

		pte->pte = pte_val(*pte) | ((unsigned long)1 << (oas+2));
		//smp_mb();
                pr_info("new pte value: 0x%llx\n", pte_val(*pte));
		flush_tlb_mm(mm);

		pr_info("try to access %p\n", &p[0]);
		p[0] = 0x12;

		vfree(p);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int do_address_size_fault(struct exp_item *item)
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
		return do_kernel_address_size_fault(lvl);
	} else {
		return do_user_address_size_fault(lvl, item->data);
	}
}

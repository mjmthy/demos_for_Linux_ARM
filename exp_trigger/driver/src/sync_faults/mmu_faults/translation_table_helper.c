#include <linux/types.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include "translation_table_helper.h"
#include "../../sysreg.h"

void *get_tbl_ent(struct mm_struct *mm, unsigned long addr, int type)
{
        pgd_t *pgd = NULL;
        pud_t *pud = NULL;
        pmd_t *pmd = NULL;
        pte_t *pte = NULL;

        if (!mm)
                mm = &init_mm;

        pgd = pgd_offset(mm, addr);
        if (pgd_none(*pgd) || pgd_bad(*pgd))
                goto done;

        pud = pud_offset(pgd, addr);
        if (pud_none(*pud) || pud_bad(*pud))
                goto done;

        pmd = pmd_offset(pud, addr);
        if (pmd_none(*pmd) || pmd_bad(*pmd))
                goto done;

        pte = pte_offset_map(pmd, addr);
        pte_unmap(pte);

done:
        switch (type) {
                case TABLE_ENTRY_PGD:
                        return pgd;
                case TABLE_ENTRY_PUD:
                        return pud;
                case TABLE_ENTRY_PMD:
                        return pmd;
                case TABLE_ENTRY_PTE:
                        return pte;
                default:
                        return NULL;
        }
}

/*
 * a valid translation table entry cound be
 * a table/block descriptor(for level 0~2)
 * or a page descriptor(for level 3)
 */
pgd_t *first_cond_pgd(struct mm_struct *mm, bool valid, struct index_g *ig)
{
	pgd_t *pgd_base_virt = NULL;
	int i;

	if(!ig)
		return NULL;

	if (!mm)
	       mm = &init_mm;

	pgd_base_virt = mm->pgd;
	for (i=0; i<PTRS_PER_PGD; i++) {
		pr_info("pgd[%d]: 0x%llx\n", i, pgd_val(pgd_base_virt[i]));
		if ((valid && !pgd_none(pgd_base_virt[i]) && !pgd_bad(pgd_base_virt[i])) ||
		    (!valid && pgd_invalid(pgd_base_virt[i])))
			break;
	}

	if (i < PTRS_PER_PGD) {
		ig->pgd_i = i;
		return &pgd_base_virt[i];
	}
	return NULL;
}

pud_t *first_cond_pud(struct mm_struct *mm, bool valid, struct index_g *ig)
{
	pud_t *pud = NULL;
	
	if (!ig)
		return NULL;

#if CONFIG_PGTABLE_LEVELS > 3
	pgd_t *pgd = NULL;
	pud_t *pud_base_virt = NULL;
	int i;

	pgd = first_cond_pgd(mm, 1, ig);
	if (!pgd)
		return NULL;
	pud_base_virt = (pud_t *)phys_to_virt(pgd_val(*pgd) &
					      PHYS_MASK & (s32)PAGE_MASK);

	for (i=0; i<PTRS_PER_PUD; i++) {
		pr_info("pud[%d]: 0x%llx\n", i, pud_val(pud_base_virt[i]));
		if ((valid && !pud_none(pud_base_virt[i]) && !pud_bad(pud_base_virt[i])) || 
		    (!valid && pud_invalid(pud_base_virt[i])))
			break;
	}
	
	if (i < PTRS_PER_PUD) {
		ig->pud_i = i;
		return &pud_base_virt[i];
	}
	return NULL;
#else
	pud = (pud_t *)first_cond_pgd(mm, valid, ig);
	ig->pud_i = ig->pgd_i;
	return pud;
#endif
}

pmd_t *first_cond_pmd(struct mm_struct *mm, bool valid, struct index_g *ig)
{
	pud_t *pud = NULL;
	pmd_t *pmd_base_virt = NULL;
	int i;

	if (!ig)
		return NULL;

	pud = first_cond_pud(mm, 1, ig);
	if (!pud)
		return NULL;
	pmd_base_virt = (pmd_t *)phys_to_virt(pud_val(*pud) &
                                              PHYS_MASK & (s32)PAGE_MASK);

	for (i=0; i<PTRS_PER_PMD; i++) {
		pr_info("pmd[%d]: 0x%llx\n", i, pmd_val(pmd_base_virt[i]));
		if ((valid && !pmd_none(pmd_base_virt[i]) && !pmd_bad(pmd_base_virt[i])) ||
		    (!valid && pmd_invalid(pmd_base_virt[i])))
			break;
	}

	if (i < PTRS_PER_PMD) {
		ig->pmd_i = i;
		return &pmd_base_virt[i];
	}
	return NULL;
}

pte_t *first_cond_pte(struct mm_struct *mm, bool valid, struct index_g *ig)
{
	pmd_t *pmd = NULL;
	pte_t *pte_base_virt = NULL;
	int i;

	if (!ig)
		return NULL;

	pmd = first_cond_pmd(mm, 1, ig);
	if (!pmd) {
		pr_err("no %s pmd found\n", valid?"valid":"invalid");
		return NULL;
	}
	pte_base_virt = (pte_t *)phys_to_virt(pmd_val(*pmd) &
                                              PHYS_MASK & (s32)PAGE_MASK);

	for (i=0; i<PTRS_PER_PTE; i++) {
		pr_info("pte[%d]: 0x%llx\n",i, pte_val(pte_base_virt[i]));
		if ((valid && !pte_invalid(pte_base_virt[i])) ||
		    (!valid && pte_invalid(pte_base_virt[i]))) {
			break;
		}
	}

	if (i < PTRS_PER_PTE) {
		ig->pte_i = i;
		return &pte_base_virt[i];
	}
	return NULL;
}

/*
 * Permission related
 */
int set_mem_x(struct mm_struct *mm, unsigned long p, int type, bool with_x)
{
	pud_t *pud;
	pud_t new_pud;
	pmd_t *pmd;
	pmd_t new_pmd;
	pte_t *pte;
	pte_t new_pte;

	switch (type) {
	case TABLE_ENTRY_PUD:
		pud = get_tbl_ent(mm, p, TABLE_ENTRY_PUD);
		if (!pud)
			return -EFAULT;
		if (!pud_is_block(*pud))
			return -EINVAL;
		break;
		if (with_x)
			new_pud = clear_pud_bit(*pud, __pgprot(PUD_PXN));
		else
			new_pud = set_pud_bit(*pud, __pgprot(PUD_PXN));
		set_pud(pud, new_pud);
	case TABLE_ENTRY_PMD:
		pmd = get_tbl_ent(mm, p, TABLE_ENTRY_PMD);
		if (!pmd)
			return -EFAULT;
		if (!pmd_is_block(*pmd))
			return -EINVAL;
		if (with_x)
			new_pmd = clear_pmd_bit(*pmd, __pgprot(PMD_PXN));
		else
			new_pmd = set_pmd_bit(*pmd, __pgprot(PMD_PXN));
		set_pmd(pmd, new_pmd);
		break;
	case TABLE_ENTRY_PTE:
		pte = get_tbl_ent(mm, p, TABLE_ENTRY_PTE);
		if (!pte)
			return -EFAULT;
		if (!pte_is_page(*pte))
			return -EINVAL;
		if (with_x)
			new_pte = clear_pte_bit(*pte, __pgprot(PTE_PXN));
		else
			new_pte = set_pte_bit(*pte, __pgprot(PTE_PXN));
		set_pte(pte, new_pte);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int get_ttbr_range(bool is_kernel, unsigned long *start, unsigned long *end)
{
	if (!start || !end)
		return -EINVAL;

	if (is_kernel) {
		*start = 0xffffffffffffffff - (((unsigned long)1 << VA_BITS) - 1);
		*end = 0xffffffffffffffff;
	} else {
		*start = 0x0000000000000000;
		*end = ((unsigned long)1 << VA_BITS) - 1;
	}

	return 0;
}

/*
 * TCR_EL1 related
 * from ARM Architecture Reference Manual
 *     ARMv8, for ARMv8-A architecture profile
 * The Output address size session
 */
int get_output_address_size(void)
{
	unsigned long tcr_el1;
	unsigned long ips;

	tcr_el1 = read_sysreg(tcr_el1);
	ips = tcr_el1 >> TCR_EL1_IPS_OFFSET & 0x7;

	switch (ips) {
	case 0x0:
		return 32;
	case 0x1:
		return 36;
	case 0x2:
		return 40;
	case 0x3:
		return 42;
	case 0x4:
		return 44;
	case 0x5:
		return 48;
	default:
		BUG();
	}
}

#ifndef _TRANSLATION_TABLE_HELPER_H
#define _TRANSLATION_TABLE_HELPER_H

#include <linux/mm_types.h>

enum {
        TABLE_ENTRY_PGD = 0,
        TABLE_ENTRY_PUD,
        TABLE_ENTRY_PMD,
        TABLE_ENTRY_PTE
};

struct index_g {
	int pgd_i;
	int pud_i;
	int pmd_i;
	int pte_i;
};

#define PUD_PXN			(_AT(pteval_t, 1) << 53)
#define PMD_PXN			(_AT(pteval_t, 1) << 53)

#define pud_is_block(pud)   ((pud_val(pud) & 0x3) == 0x1)
#define pmd_is_block(pmd)   ((pmd_val(pmd) & 0x3) == 0x1)
#define pte_is_page(pte)    ((pte_val(pte) & 0x3) == 0x3)
#define pgd_invalid(pgd) ((pgd_val(pgd) & 0x1) == 0x0)
#define pud_invalid(pud) ((pud_val(pud) & 0x1) == 0x0)
#define pmd_invalid(pmd) ((pmd_val(pmd) & 0x1) == 0x0)
#define pte_invalid(pte) (((pte_val(pte) & 0x1) == 0x0) ||\
			  ((pte_val(pte) & 0x3) == 0x1))

void *get_tbl_ent(struct mm_struct *mm, unsigned long addr, int type);

pgd_t *first_cond_pgd(struct mm_struct *mm, bool valid, struct index_g *ig);
pud_t *first_cond_pud(struct mm_struct *mm, bool valid, struct index_g *ig);
pmd_t *first_cond_pmd(struct mm_struct *mm, bool valid, struct index_g *ig);
pte_t *first_cond_pte(struct mm_struct *mm, bool valid, struct index_g *ig);

int get_ttbr_range(bool is_kernel, unsigned long *start, unsigned long *end);
int get_output_address_size(void);


/*
 * Memory block behaves almost the same as page,
 * and they share most of the control bits, so
 * we add corresponding APIs here
 */
static inline pud_t clear_pud_bit(pud_t pud, pgprot_t prot)
{
	pud_val(pud) &= ~pgprot_val(prot);
	return pud;
}

static inline pmd_t clear_pmd_bit(pmd_t pmd, pgprot_t prot)
{
	pmd_val(pmd) &= ~pgprot_val(prot);
	return pmd;
}

static inline pud_t set_pud_bit(pud_t pud, pgprot_t prot)
{
	pud_val(pud) |= pgprot_val(prot);
	return pud;
}

static inline pmd_t set_pmd_bit(pmd_t pmd, pgprot_t prot)
{
	pmd_val(pmd) |= pgprot_val(prot);
	return pmd;
}

int set_mem_x(struct mm_struct *mm, unsigned long p, int type, bool with_x);

#endif

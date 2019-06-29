#ifndef _SYSREG_H_
#define _SYSREG_H_

/*
 * Some sysreq offset definition is not covered
 * by include/asm/sysreg.h
 */

/*
 * TCR_EL1 specific flags
 */
#define TCR_EL1_IPS_OFFSET 32

/* SCTLR_EL1 specific flags. */
#define SCTLR_EL1_SA0	(1 << 4)

#define SCTLR_EL1_WXN_OFFSET 19

/*
 * SPSel specific flags
 */
#define SPSEL_SP BIT(0)

#endif

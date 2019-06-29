#ifndef _EXP_TRIGGER_DEMO_H
#define _EXP_TRIGGER_DEMO_H

#define EXP_TRIGGER         _IO('f', 0x99)

enum {
        /*
         * Cases of MMU faults
         */
        EXP_CASE_T_ADDRESS_SIZE_FAULT = 0,
        EXP_CASE_T_TRANSLATION_FAULT,
        EXP_CASE_T_ACCESS_FLAG_FAULT,
	EXP_CASE_T_PERMISSION_FAULT,
	EXP_CASE_T_ALIGNMENT_FAULT,
	EXP_CASE_T_SYNC_EXT_AB_ON_TBL_WALK,
	/*
	 * Cases of other sync exceptions
	 */
	EXP_CASE_T_SYNC_EXT_ABORT,
	/*
	 * Cases of SError
	 */
	EXP_CASE_T_SERROR,
        EXP_CASE_T_MAX
};

enum {
        EXP_CASE_M_USER = 0,
        EXP_CASE_M_KERNEL,
        EXP_CASE_M_MAX
};

enum {
        EXP_CASE_SUBT_DATA_UNALIGN = 0,
        EXP_CASE_SUBT_INSTR_UNALIGN,
        EXP_CASE_SUBT_SP_UNALIGN,
        EXP_CASE_SUBT_MAX
};

/*
 * sometimes we need kernel to
 * help generate an address that
 * can trigger user space exception
 */
union extra_data {
	unsigned long addr_in;
	unsigned long addr_out;
};

struct exp_case {
        int type;
        int mode;
	union {
        	unsigned int lvl;
		unsigned int sub_t;
	};
};

struct exp_item {
        struct exp_case e_case;
	void *data;
};

int user_address_size_fault(int fp, struct exp_item *item);
int user_translation_fault(int fp, struct exp_item *item);
int user_access_flag_fault(int fp, struct exp_item *item);
int user_permission_fault(int fp, struct exp_item *item);

#endif

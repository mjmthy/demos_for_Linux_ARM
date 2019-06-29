#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "exp_trigger_demo.h"

char *exp_type[] = {
			"address_size_fault",
			"translation_fault",
			"access_flag_fault",
			"permission_fault",
			"alignment_fault",
			"sync_ext_ab_on_tbl_walk",
			"sync_ext_abort",
			"serror",
			NULL
		   };
char *exp_mode[] = {
			"user",
			"kernel",
			NULL
		   };
char *exp_level[] = {
			"level 0",
			"level 1",
			"level 2",
			"level 3",
			NULL
		     };
char *exp_sub_type[] = {
			"data access alignment fault",
			"pc alignment fault",
			"sp alignment fault",
			NULL
};

#define EXP_TRIGGER_PATH    "/proc/demo/exp_trigger"

int main (int argc, char *argv[])
{
	int i;
	int t_index = EXP_CASE_T_MAX;
	int m_index = EXP_CASE_M_MAX;
	int l_index = -1;
	int subt_index = EXP_CASE_SUBT_MAX;
	struct exp_item item;
	int   fp;
	int ret;

	printf("please select the exception type you wanna trigger: \n");
	i = 0;
	while (exp_type[i] != NULL) {
		printf("%d. %s\n", i, exp_type[i]);
		i++;
	}
	printf("type index: ");
	scanf("%d", &t_index);

	if ((t_index == EXP_CASE_T_SYNC_EXT_AB_ON_TBL_WALK) ||
	    (t_index == EXP_CASE_T_SERROR) ||
	    (t_index == EXP_CASE_T_SYNC_EXT_ABORT))
		goto done_select;

	printf("\ntrigger the exception in kernel or user mode: \n");
	i = 0;
	while (exp_mode[i] != NULL) {
		printf("%d. %s\n", i, exp_mode[i]);
		i++;
	}
	printf("mode index: ");
	scanf("%d", &m_index);

	if (t_index <= EXP_CASE_T_PERMISSION_FAULT) {
		printf("\ntrigger the exception in which level: \n");
		i = 0;
		while (exp_level[i] != NULL) {
			printf("%d. %s\n", i, exp_level[i]);
			i++;
		}
		printf("level index: ");
		scanf("%d", &l_index);
	} else {
		if (t_index == EXP_CASE_T_ALIGNMENT_FAULT) {
			i = 0;
			while (exp_sub_type[i] != NULL) {
				printf("%d. %s\n", i, exp_sub_type[i]);
				i++;
			}
			printf("sub type index: ");
			scanf("%d", &subt_index);
		}
		l_index = EXP_CASE_T_MAX;
	}

done_select:
#if 0
	printf("t_index: %d\n", t_index);
	printf("m_index: %d\n", m_index);
	printf("l_index: %d\n", l_index);
	printf("subt_index: %d\n", subt_index);
#endif
	item.e_case.type = t_index;
	item.e_case.mode = m_index;
	if (t_index == EXP_CASE_T_ALIGNMENT_FAULT)
		item.e_case.sub_t = subt_index;
	else
		item.e_case.lvl = l_index;
	item.data = NULL;

	fp  = open(EXP_TRIGGER_PATH, O_RDWR);
        if (fp < 0) {
                printf("fail to open %s\n", EXP_TRIGGER_PATH);
                return -1;
        }

	if (item.e_case.mode != EXP_CASE_M_USER)
		return ioctl(fp, EXP_TRIGGER, &item);
	/*
	 * For user mode exceptions, we may have 
	 * extra preparations
	 */
	switch (item.e_case.type) {
	case EXP_CASE_T_ADDRESS_SIZE_FAULT:
		return user_address_size_fault(fp, &item);
	case EXP_CASE_T_TRANSLATION_FAULT:
		return user_translation_fault(fp, &item);
	case EXP_CASE_T_ACCESS_FLAG_FAULT:
		return user_access_flag_fault(fp, &item);
	case EXP_CASE_T_PERMISSION_FAULT:
		return user_permission_fault(fp, &item);
	default:
		return -1;
	}
}


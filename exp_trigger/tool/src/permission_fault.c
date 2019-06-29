#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "exp_trigger_demo.h"

static int test_func(void)
{
	return 0;
}


/*
 * For user space text section, the default VMA and PTE R/W
 * properity are all READ ONLY. So trying to write this memory
 * area will definately trigger user space level 3 permission
 * fault.
 */
int user_permission_fault(int fp, struct exp_item *item)
{
	int *p = NULL;

	switch (item->e_case.lvl) {
	case 3:
		p = (int *)test_func;

		*p = 0x1234;
	default:
		p = malloc(sizeof(union extra_data));
		if (p == NULL) {
			printf("fail to allocate extra_data\n");
			return -1;
		}
		item->data = p;

		ioctl(fp, EXP_TRIGGER, item);
	}

	return 0;
#if 0
	union extra_data *p;
	unsigned char *phead = NULL;
	int ret;

	p = malloc(sizeof(union extra_data));
	if (p == NULL) {
		printf("fail to allocate extra_data\n");
		return -1;
	}

	switch (item->e_case.lvl) {
	case 3:
		phead = sbrk(0);
		p->addr_in = (unsigned long)sbrk(4 * 1024);
		if (!p->addr_in)
			goto err_exit;
		printf("p->addr_in: 0x%lx\n", p->addr_in);
		break;
	default:
		ret = -1;
		goto err_exit;		
	}

	item->data = p;
	((int *)(p->addr_in))[0] = 0x1234;
	ret = ioctl(fp, EXP_TRIGGER, item);
	printf("try to access 0x%lx\n", p->addr_in);

	sleep(1);
	((int *)(p->addr_in))[0] = 0x4321;

	return 0;

err_exit:
	if (phead != NULL)
		brk(phead);
	free(p);
	return ret;
#endif
}

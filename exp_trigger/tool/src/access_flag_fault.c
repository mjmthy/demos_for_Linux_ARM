#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "exp_trigger_demo.h"

int user_access_flag_fault(int fp, struct exp_item *item)
{
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

	/*
	 * make the first access, so that the mapping of
	 * virtual memory to physical memory will be
	 * created.
	 */
	((int *)(p->addr_in))[0] = 0x1234;
	ret = ioctl(fp, EXP_TRIGGER, item);
	printf("try to access 0x%lx\n", p->addr_in);
	/*
	 * wait for 1s, so that the kernel message will
	 * not show up too early
	 */
	sleep(1);
	((int *)(p->addr_in))[0] = 0x4321;

	if (phead != NULL)
		brk(phead);
	free(p);
	return ret;

err_exit:
	if (phead != NULL)
		brk(phead);
	free(p);
	return ret;
}

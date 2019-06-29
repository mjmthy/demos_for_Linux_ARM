#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "exp_trigger_demo.h"

int user_translation_fault(int fp, struct exp_item *item)
{
	union extra_data *p;
        int ret;
	unsigned long fault_trigger_address;

        p = malloc(sizeof(union extra_data));
        if (p == NULL) {
                printf("fail to allocate extra_data\n");
                return -1;
        }

	switch (item->e_case.lvl) {
	case 0:
	case 1:
	case 2:
	case 3:
		item->data = p;
		ret = ioctl(fp, EXP_TRIGGER, item);
		if (ret != 0)
			goto err_exit;
		fault_trigger_address = p->addr_out;
		break;
	default:
		ret = -1;
		goto err_exit;
	}

	printf("fault_trigger_address: 0x%lx\n", fault_trigger_address);
	sleep(1);
	*((unsigned long *)fault_trigger_address) = 0x1234;
	free(p);
	return 0;

err_exit:
	free(p);
	return ret;
}

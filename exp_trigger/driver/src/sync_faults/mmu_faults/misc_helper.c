#include <linux/module.h>
#include <linux/kernel.h>
#include "../../exp_trigger.h"

int check_param(struct exp_item *item)
{
        int mode;
        int lvl;

        if (!item)
                return -EINVAL;

        mode = item->e_case.mode;
        lvl = item->e_case.lvl;
        if ((mode < 0 || mode >= EXP_CASE_M_MAX) ||
            (lvl < 0 || lvl > 3) ||
            (mode != EXP_CASE_M_KERNEL && item->data == NULL))
                return -EINVAL;

        return 0;
}


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "exp_trigger.h"

#undef pr_fmt
#define pr_fmt(fmt) "exp_trigger: " fmt

static struct proc_dir_entry *demo_dir;
static struct proc_dir_entry *exp_trigger_entry;

extern int show_unhandled_signals;

static int exp_trigger_show(struct seq_file *seq, void *v)
{
	seq_printf(seq, "\nWe can trigger all kinds of exceptions ARMv8 aarch64 mode supported :-)\n");
	seq_printf(seq, "How to Use: \n\n");
	return 0;
}

static ssize_t exp_trigger_write(struct file *file, const char __user *buffer,
size_t count, loff_t *ppos)
{
	return count;
}

static int exp_trigger_open(struct inode *inode, struct file *file)
{
	return single_open(file, exp_trigger_show, PDE_DATA(inode));
}

static long exp_trigger_do_ioctl(struct file *file, unsigned int cmd,
	    void __user *p)
{
	struct exp_item *item = NULL;
	int ret;

	item = kmalloc(sizeof(*item), GFP_KERNEL);
	if (!item)
		return -ENOMEM;
	
	ret = copy_from_user(item, p, sizeof(*item));
	if (ret != 0)
		goto err_exit;

#if 0
	if (item->e_case.type == EXP_CASE_T_ALIGNMENT_FAULT)
		pr_info("type: %d, mode: %d, sub_t: %d\n", item->e_case.type,
			item->e_case.mode, item->e_case.sub_t);
	else
		pr_info("type: %d, mode: %d, lvl: %d\n", item->e_case.type,
			item->e_case.mode, item->e_case.lvl);
#endif

	switch (item->e_case.type) {
	case EXP_CASE_T_ADDRESS_SIZE_FAULT:
		ret = do_address_size_fault(item);
		if (ret != 0)
			goto err_exit;
		break;
	case EXP_CASE_T_TRANSLATION_FAULT:
		ret = do_translation_fault(item);
		if (ret != 0)
			goto err_exit;
		break;
	case EXP_CASE_T_ACCESS_FLAG_FAULT:
		ret = do_access_flag_fault(item);
		if (ret != 0)
			goto err_exit;
		break;
	case EXP_CASE_T_PERMISSION_FAULT:
		ret = do_permission_fault(item);
		if (ret != 0)
			goto err_exit;
		break;
	case EXP_CASE_T_ALIGNMENT_FAULT:
		ret = do_alignment_fault(item);
		if (ret != 0)
			goto err_exit;
		break;
	case EXP_CASE_T_SYNC_EXT_AB_ON_TBL_WALK:
		ret = do_sync_ext_ab_on_tbl_walk(item);
		if (ret != 0)
			goto err_exit;
		break;
	case EXP_CASE_T_SYNC_EXT_ABORT:
		ret = do_sync_ext_abort(item);
		if (ret != 0)
			goto err_exit;
		break;
	case EXP_CASE_T_SERROR:
		ret = do_serror();
		if (ret != 0)
			goto err_exit;
		break;
	default:
		ret = -EINVAL;
		goto err_exit;
	}

	ret = copy_to_user(p, item, sizeof(*item));
	if (ret != 0)
		goto err_exit;
	kfree(item);
	return 0;

err_exit:
	kfree(item);
	return ret;
}

static long exp_trigger_ioctl(struct file *file, unsigned int cmd,
	   unsigned long arg)
{
	return exp_trigger_do_ioctl(file, cmd, (void __user *)arg);
}

#ifdef CONFIG_COMPAT
static long exp_trigger_compat_ioctl(struct file *file, unsigned int cmd,
	    unsigned long arg)
{
	return exp_trigger_do_ioctl(file, cmd, compat_ptr(arg));
}
#endif

static const struct file_operations exp_trigger_fops =
{
	.owner = THIS_MODULE,
	.open = exp_trigger_open,
	.read = seq_read,
	.write = exp_trigger_write,
	.llseek = seq_lseek,
	.release = single_release,
	.unlocked_ioctl = exp_trigger_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = exp_trigger_compat_ioctl,
#endif
};

static __init int exp_trigger_init(void)
{
	demo_dir = proc_mkdir("demo", NULL);
	if (!demo_dir)
		return -ENOMEM;

	exp_trigger_entry = proc_create("exp_trigger",0666, demo_dir, &exp_trigger_fops);
	if (!exp_trigger_entry) {
		remove_proc_entry("demo_dir", NULL);
		return -ENOMEM;
	}

	/*
	 * we need show_unhandled_signals to be 1, otherwise
	 * no exception infos show up when user fault happens
	 */
	if (show_unhandled_signals == 0)
		show_unhandled_signals = 1;

	return 0;
}
module_init(exp_trigger_init);

static __exit void exp_trigger_cleanup(void)
{
	remove_proc_entry("exp_trigger", demo_dir);
	remove_proc_entry("demo", NULL);
}
module_exit(exp_trigger_cleanup);

MODULE_DESCRIPTION("Trigger all kinds of exceptions that ARMv8 AARCH64 supported\n");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jiamin Ma<mjmthy@163.com>");

#
# Makefile for Amlogic Kernel Exception demo
#

obj-$(CONFIG_EXP_TRIGGER) += exp_trigger.o
obj-y += sync_faults/mmu_faults/translation_table_helper.o \
	 sync_faults/mmu_faults/misc_helper.o \
	 sync_faults/mmu_faults/access_flag_fault.o \
	 sync_faults/mmu_faults/translation_fault.o \
	 sync_faults/mmu_faults/address_size_fault.o \
	 sync_faults/mmu_faults/permission_fault.o \
	 sync_faults/mmu_faults/alignment_fault.o sync_faults/mmu_faults/sp_alignment_fault.o \
	 sync_faults/mmu_faults/sync_ext_ab_on_tbl_walk.o

obj-y += sync_faults/others/sync_ext_abort.o
obj-y += serror/serror.o

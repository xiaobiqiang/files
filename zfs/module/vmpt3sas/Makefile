src = /root/workspace/zfsonlinux/zfs/module/vmpt3sas
obj = /root/workspace/zfsonlinux/zfs/module/vmpt3sas

MODULE := vmpt3sas

EXTRA_CFLAGS = $(ZFS_MODULE_CFLAGS)  -Wno-unused-but-set-variable -Wno-bool-compare -DHAVE_SPL -D_KERNEL -DTEXT_DOMAIN=\"zfs-linux-kernel\" -DNDEBUG 

obj-$(CONFIG_ZFS) := $(MODULE).o

$(MODULE)-objs = vmpt3sas_scsih.o


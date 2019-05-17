# $FreeBSD$

.PATH: ${SRCTOP}/sys/compat/lindebugfs

KMOD=	lindebugfs
SRCS=	vnode_if.h \
	device_if.h bus_if.h  pci_if.h \
	lindebugfs.c

CFLAGS+= -I${SYSDIR}/compat/linuxkpi/common/include

.if ${MACHINE_CPUARCH} == "powerpc"
CFLAGS+= -DSMP
CFLAGS+= -DAIM
.endif

.include <bsd.kmod.mk>

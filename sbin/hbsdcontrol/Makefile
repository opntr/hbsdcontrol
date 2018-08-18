PROG=hbsdcontrol

BINDIR?=   /sbin

HBSDCONTROL_DIR= ${.CURDIR}/../../contrib/hardenedbsd/hbsdcontrol

CFLAGS+= -I${HBSDCONTROL_DIR}
LIBADD+= sbuf
LDADD+= -lsbuf

SRCS= ${HBSDCONTROL_DIR}/main.c ${HBSDCONTROL_DIR}/libhbsdcontrol.c
MAN= ${HBSDCONTROL_DIR}/hbsdcontrol.8

.include <bsd.prog.mk>

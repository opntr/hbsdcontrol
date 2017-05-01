.include <bsd.own.mk>

PACKAGE=	libhbsdcontrol
LIB=	hbsdcontrol
SHLIB_MAJOR=	0

HBSDCONTROL_DIR= ${.CURDIR}/../../contrib/hardenedbsd/hbsdcontrol

CFLAGS+= ${HBSDCONTROL_DIR}

SRCS=	${HBSDCONTROL_DIR}/hbsdcontrol.c
INCS=	${HBSDCONTROL_DIR}/hbsdcontrol.h
MAN+=	${HBSDCONTROL_DIR}/hbsdcontrol.3
MLINKS+=	hbsdcontrol.3	hbsdcontrol_set_extattr.3
MLINKS+=	hbsdcontrol.3	hbsdcontrol_rm_extattr.3
MLINKS+=	hbsdcontrol.3	hbsdcontrol_feature_state.3

.include <bsd.lib.mk>

.include <bsd.own.mk>

PACKAGE=	libhbsdcontrol
LIB=	hbsdcontrol
SHLIB_MAJOR=	0

HBSDCONTROL_DIR= ${.CURDIR}/../../contrib/hardenedbsd/hbsdcontrol

#CFLAGS+= ${HBSDCONTROL_DIR}

SRCS=	${HBSDCONTROL_DIR}/libhbsdcontrol.c
INCS=	${HBSDCONTROL_DIR}/libhbsdcontrol.h
MAN+=	${HBSDCONTROL_DIR}/libhbsdcontrol.3
MLINKS+=	libhbsdcontrol.3	hbsdcontrol_set_extattr.3
MLINKS+=	libhbsdcontrol.3	hbsdcontrol_rm_extattr.3
MLINKS+=	libhbsdcontrol.3	hbsdcontrol_set_feature_state.3
MLINKS+=	libhbsdcontrol.3	hbsdcontrol_rm_feature_state.3

.include <bsd.lib.mk>

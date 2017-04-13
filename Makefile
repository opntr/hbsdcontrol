PROG=	hbsdcontrol
MAN=	hbsdcontrol.8

SRCS=	main.c hbsdcontrol.c

LIBADD=	sbuf
LDADD=  -lsbuf

.include <bsd.prog.mk>

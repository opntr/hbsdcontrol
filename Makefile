PROG=	hbsdcontrol
MAN=	hbsdcontrol.8

LIBADD=	sbuf
LDADD=  -lsbuf

.include <bsd.prog.mk>

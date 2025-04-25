#	$Id: Makefile,v 1.8 2022/11/13 14:09:33 ragge Exp $
#
# Makefile.in for cpp
#

CFLAGS = -g -Wall -Wmissing-prototypes -I../objdump
DEST=as

OBJS=	as.o directives.o nd100.o relocate.o aslex.o delay.o \
	segments.o symbol.o tempfile.o expr.o ofile.o aout16.o parse.o

HDRS=	as.h nd100.h instr.h

EXTRACLEAN = instr.h

all: $(DEST) $(OBJS)

$(OBJS): $(HDRS)

$(DEST): instr.h $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

instr.h: nd100-instr.awk
	cat nd100-instr.awk | awk -v flavor=AS -f nd100-instr.awk > instr.h



clean:
	/bin/rm -f $(OBJS) $(DEST) $(EXTRACLEAN)

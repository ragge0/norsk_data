#	$Id: Makefile,v 1.5 2023/12/22 13:43:20 ragge Exp $
#
#
#SYSINC=/opt/local/share/verilator/include
SYSINC=/opt/pkg/share/verilator/include
VERCPP=${SYSINC}/verilated.cpp
SUBDIR=nord10sdir
TSUBDIR=nord10stracedir
ANAME=${SUBDIR}/Vnord10s__ALL.a
TANAME=${TSUBDIR}/Vnord10s__ALL.a
VPATH=..
DEST=nord10s
TDEST=nord10s_trace
OBJS=nord10s.o verilated.o ttitest.o # ttotest.o sdramtest.o disasm.o
CFLAGS=-g -std=c++11 -Wno-deprecated-declarations
VFLAGS=-Wno-fatal -Wall # -DDBGCNT

TOP=nord10s.v
SRCS=nord10s.v timing.v prom4k.v or_logic.v regs.v decoding.v int_regs.v \
	arithmetic.v

all:  ${DEST} # ${TDEST}

${DEST}: ${OBJS} ${ANAME}
	clang++ -g ${OBJS} ${ANAME} -o ${DEST}

verilated.o: ${SYSINC}/verilated.cpp
	clang++ -I ${SUBDIR} -I ${SYSINC} ${CFLAGS} -c $<

.cpp.o:
	clang++ -I ${SUBDIR} -I ${SYSINC} ${CFLAGS} -c $<

${OBJS}: ${ANAME}

${TDEST}: nord10s.cpp ${TANAME}
	clang++ -DUSETRACE -I ${TSUBDIR} -I ${SYSINC} nord10s.cpp ${VERCPP} ${TANAME} \
	${SYSINC}/verilated_vcd_c.cpp -o ${TDEST}

${ANAME}: ${SRCS}
	verilator ${VFLAGS} -Mdir ${SUBDIR} --prefix Vnord10s -cc -y .. ${TOP}
	(cd ${SUBDIR}; make -f Vnord10s.mk)

${TANAME}: ${SRCS}
	verilator --trace ${VFLAGS} -Mdir ${TSUBDIR} --prefix Vnord10s -cc -y .. ${TOP}
	(cd ${TSUBDIR}; make -f Vnord10s.mk)

clean:
	/bin/rm -rf ${DEST} ${TDEST} ${SUBDIR} ${TSUBDIR} ${OBJS}

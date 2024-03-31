#
#
OBJS=testepg.o epg.o nd10uc.o timing.o dismac.o
CFLAGS=-g

ALL: epgtest nd10uc timing dismac

epgtest: testepg.o epg.o
	cc -o epgtest testepg.o epg.o

nd10uc: nd10uc.o epg.o
	cc -o nd10uc nd10uc.o epg.o

timing: timing.o
	cc -o timing timing.o

dismac: dismac.o
	cc -o dismac dismac.o

test: dismac
	./dismac prom.hex > prom.test1
	./micmac.awk prom.test1 > prom.test2
	@if cmp prom.hex prom.test2 ; then	\
		echo "files identical";		\
	fi

clean:
	/bin/rm -f ${OBJS} epgtest nd10uc timing dismac prom.test1 prom.test2

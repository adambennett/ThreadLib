# Makefile for UD CISC user-level thread library

CC = gcc
CFLAGS = -g

LIBOBJS = t_lib.o

TSTOBJS = test00.o T1.o T1x.o T1a.o T2.o T4.o T4a.o T7.o T8.o T6.o T5.o

# specify the executable 

EXECS = test00 T1 T1x T1a T2 T4 T4a T7 T8 T6 T5

# specify the source files

LIBSRCS = t_lib.c 

TSTSRCS = test00.c T1.c T1x.c T1a.c T2.c T4.c T4a.c T7.c T8.c T6.c T5.c

# ar creates the static thread library

t_lib.a: ${LIBOBJS} Makefile
	ar rcs t_lib.a ${LIBOBJS}

# here, we specify how each file should be compiled, what
# files they depend on, etc.

t_lib.o: t_lib.c t_lib.h Makefile
	${CC} ${CFLAGS} -c t_lib.c

test00.o: test00.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c test00.c

test00: test00.o t_lib.a Makefile
	${CC} ${CFLAGS} test00.o t_lib.a -o test00
	
T1.o: T1.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c T1.c

T1: T1.o t_lib.a Makefile
	${CC} ${CFLAGS} T1.o t_lib.a -o T1
	
T1x.o: T1x.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c T1x.c

T1x: T1x.o t_lib.a Makefile
	${CC} ${CFLAGS} T1x.o t_lib.a -o T1x
	
T1a.o: T1a.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c T1a.c

T1a: T1a.o t_lib.a Makefile
	${CC} ${CFLAGS} T1a.o t_lib.a -o T1a
	
T2.o: T2.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c T2.c

T2: T2.o t_lib.a Makefile
	${CC} ${CFLAGS} T2.o t_lib.a -o T2
	
T4.o: T4.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c T4.c

T4: T4.o t_lib.a Makefile
	${CC} ${CFLAGS} T4.o t_lib.a -o T4
	
T4a.o: T4a.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c T4a.c

T4a: T4a.o t_lib.a Makefile
	${CC} ${CFLAGS} T4a.o t_lib.a -o T4a
	
T7.o: T7.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c T7.c

T7: T7.o t_lib.a Makefile
	${CC} ${CFLAGS} T7.o t_lib.a -o T7
	
T5.o: T5.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c T5.c

T5: T5.o t_lib.a Makefile
	${CC} ${CFLAGS} T5.o t_lib.a -o T5
	
T6.o: T6.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c T6.c

T6: T6.o t_lib.a Makefile
	${CC} ${CFLAGS} T6.o t_lib.a -o T6
	
T8.o: T8.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c T8.c

T8: T8.o t_lib.a Makefile
	${CC} ${CFLAGS} T8.o t_lib.a -o T8

clean:
	rm -f t_lib.a ${EXECS} ${LIBOBJS} ${TSTOBJS} 

CC 	= gcc

CFLAGS  = -Wall -g -I .

LD 	= gcc

LDFLAGS  = -Wall -g -no-pie

PROGS	= snakes nums hungry

SNAKEOBJS  = randomsnakes.o 

HUNGRYOBJS = hungrysnakes.o 

NUMOBJS    = numbersmain.o

OBJS	= $(SNAKEOBJS) $(HUNGRYOBJS) $(NUMOBJS) 

SRCS	= randomsnakes.c numbersmain.c hungrysnakes.c

HDRS	= lwp.h rr.h fp.h snakes.h util.h schedulers.h

EXTRACLEAN = core $(PROGS)

.PHONY: all allclean clean submission

all: 	$(PROGS)

allclean: clean
	@rm -f $(EXTRACLEAN)

clean:	
	rm -f $(OBJS) util.o lwp.o rr.o magic64.o liblwp.a *~ TAGS

snakes: randomsnakes.o liblwp.a libsnakes.so
	$(LD) $(LDFLAGS) -o snakes randomsnakes.o -L. -lsnakes -llwp -lncurses -lrt

hungry: hungrysnakes.o liblwp.a libsnakes.so
	$(LD) $(LDFLAGS) -o hungry hungrysnakes.o -L. -lsnakes -llwp -lncurses -lrt

nums: numbersmain.o liblwp.a 
	$(LD) $(LDFLAGS) -o nums numbersmain.o -L. -llwp

hungrysnakes.o: hungrysnakes.c $(HDRS)

randomsnakes.o: randomsnakes.c $(HDRS)

numbersmain.o: numbersmain.c $(HDRS)

liblwp.a: lwp.c rr.c util.c magic64.S $(HDRS)
	$(CC) $(CFLAGS) -c rr.c util.c lwp.c magic64.S 
	ar rcs liblwp.a util.o lwp.o rr.o magic64.o

submission: lwp.c rr.c util.c magic64.S $(HDRS) $(SRCS) libsnakes.so Makefile README
	tar -czf project2_submission.tar.gz lwp.c rr.c util.c magic64.S $(HDRS) $(SRCS) libsnakes.so Makefile README

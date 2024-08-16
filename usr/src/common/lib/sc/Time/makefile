#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sc:Time/makefile	3.16"

include $(LIBRULES)

PRODUCT = ../libTime.a

OBJECTS = Duration.o Place.o Time.o Tprint.o Time_prev.o mystrdup.o \
	time_comm.o tmdata.o tmdate.o tmfix.o tmform.o tmgoff.o tmleap.o \
	tmlex.o tmmake.o tmpoff.o tmtime.o tmtype.o tmword.o tmzone.o

all : $(PRODUCT)

install : all

$(PRODUCT) : $(OBJECTS)
	$(AR) $(ARFLAGS) $(@) `$(LORDER) $(OBJECTS) | $(TSORT)`

time_comm.o : ctime/time_comm.c
	$(C++CMD) -c ctime/time_comm.c

mystrdup.o : tm/mystrdup.c
	$(C++CMD) -c tm/mystrdup.c

tmdata.o : tm/tmdata.c
	$(C++CMD) -c tm/tmdata.c

tmdate.o : tm/tmdate.c
	$(C++CMD) -c tm/tmdate.c

tmfix.o : tm/tmfix.c
	$(C++CMD) -c tm/tmfix.c

tmform.o : tm/tmform.c
	$(C++CMD) -c tm/tmform.c

tmgoff.o : tm/tmgoff.c
	$(C++CMD) -c tm/tmgoff.c

tmleap.o : tm/tmleap.c
	$(C++CMD) -c tm/tmleap.c

tmlex.o : tm/tmlex.c
	$(C++CMD) -c tm/tmlex.c

tmmake.o : tm/tmmake.c
	$(C++CMD) -c tm/tmmake.c

tmpoff.o : tm/tmpoff.c
	$(C++CMD) -c tm/tmpoff.c

tmtime.o : tm/tmtime.c
	$(C++CMD) -c tm/tmtime.c

tmtype.o : tm/tmtype.c
	$(C++CMD) -c tm/tmtype.c

tmword.o : tm/tmword.c
	$(C++CMD) -c tm/tmword.c

tmzone.o : tm/tmzone.c
	$(C++CMD) -c tm/tmzone.c

.c.o : 
	$(C++CMD) -c $(<)

demos:
	cd demos; $(MAKE)

tests:

clean:
	rm -f *.ii *.o *.a core a.out
	cd demos; $(MAKE) clean SCLIB=../$(SCLIB)

clobber: clean
	rm -f $(PRODUCT)

#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sc:Path/ksh/makefile	3.3"

include $(LIBRULES)

# Purposely left the ksh stuff in good-old C
# to keep it as close as possible to the actual ksh 
# source.  This way, any future updates to ksh will
# be easy to apply.
#

PRODUCT = ksh.a t group

all : $(PRODUCT)

install : all

OBJECTS = test.o expand.o 

t:	t.o ksh.a
	$(CC_CMD) -o t t.o ../strmatch.o ksh.a -l++

t.o:	t.c
	$(CC_CMD) -c t.c

group:	group.o
	$(CC_CMD) -o group group.o
	chmod 2777 group

ksh.a:	$(OBJECTS)
	$(RM) -f $@
	$(AR) rc $@ $(OBJECTS)

clean:
	$(RM) -f $(OBJECTS)


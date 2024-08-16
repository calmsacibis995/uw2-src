#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ihvkit:pdi/dditools/ddicheck.mk	1.1"

include $(CMDRULES)

#DDIVER=SVR42
#DDIVER=SVR42MP
DDIVER=UW20

all:	llib flint


llib:	 llib-lddi.ln 

llib-lddi.ln:	ddilint.c
	$(LINT) -y $(INCLIST) $(DEFLIST) -DDDI_$(DDIVER) -n -oddi ddilint.c 
	

flint:	flint.c
	$(CC) $(CFLAGS) $(INCLIST) flint.c -o flint

install:	all
	# Change LIBPATH to be $USRLIB
	mv ddicheck ddicheck.tmp
	echo $(USRLIB) | sed 's/\//\\\//g' > ddicheck.path
	sed "s/LIBPATH/`cat ddicheck.path`/g" ddicheck.tmp > ddicheck
	$(INS) -f $(USRBIN) -m $(BINMODE) -u $(OWN) -g $(GRP) ddicheck
	$(INS) -f $(USRLIB) -m $(BINMODE) -u $(OWN) -g $(GRP) flint
	$(INS) -f $(USRLIB) -m 755        -u $(OWN) -g $(GRP) llib-lddi.ln
	$(INS) -f $(USRLIB) -m 755        -u $(OWN) -g $(GRP) ddilint.data
	
clean:
	
clobber: clean
	-rm -f flint llib-lddi.ln
	

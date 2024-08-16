#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ktool:common/ktool/ddicheck/ddicheck.mk	1.5"

include $(CMDRULES)

#DDI_VERSION=SVR42
#DDI_VERSION=SVR42MP
DDI_VERSION=UW20

all:	llib $(PFX)flint

llib:	 llib-lddi.ln 

llib-lddi.ln:	ddilint.c
	$(LINT) -y $(INCLIST) $(DEFLIST) -DDDI_$(DDI_VERSION) -n -oddi ddilint.c 

$(PFX)flint:	flint.c
	$(CC) $(CFLAGS) $(INCLIST) flint.c -o $(PFX)flint

install:	all
	$(INS) -f $(USRBIN) -m $(BINMODE) -u $(OWN) -g $(GRP) ddicheck
	$(INS) -f $(USRLIB) -m $(BINMODE) -u $(OWN) -g $(GRP) $(PFX)flint
	$(INS) -f $(USRLIB) -m 755        -u $(OWN) -g $(GRP) llib-lddi.ln
	$(INS) -f $(USRLIB) -m 755        -u $(OWN) -g $(GRP) ddilint.data
	
clean:
	
clobber: clean
	-rm -f $(PFX)flint llib-lddi.ln
	

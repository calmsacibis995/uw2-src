#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libNwLocale:libnwLocale.mk	1.7"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/libnwLocale.mk,v 1.6 1994/09/26 17:20:44 rebekah Exp $"
include $(LIBRULES)

.SUFFIXES: .P

DOTSO = libNwLoc.so
DOTA = libNwLoc.a

LOCALINC = -I../../head/nw -I../../head/inc -I../../head

SHLIBLDFLAGS = -G -ztext
LFLAGS = -G -dy -ztext
LOCALDEF = -DUSL -DSVR4 -DI18N -DFUNCPROTO=15 -DNARROWPROTO -DN_PLAT_UNIX -D_INTELC32_ -DUNIX -DNATIVE -DN_USE_CRT -D_REENTRANT 

HFLAGS = -h /usr/lib/libNwLoc.so

# getupper.c should not be in makefile
# initunin.c should not be in makefile
# locmain.c should not be in makefile cause it's windows stuff (need to verify)
# unialloc.c should not be in makefile
# unicode.c should not be in makefile

SRCS1 = \
	_llocale.c \
	atoi.c \
	chartype.c \
	charupr.c \
	charval.c \
	fprintf.c \
	getcntry.c \
	getcoll.c \
	getvect.c \
	inc.c \
	inituni.c \
	isalnum.c \
	isalpha.c \
	isdigit.c \
	itoa.c \
	libnwlocale_mt.c \
	linschar.c \
	llocalec.c \
	llocaled.c \
	lmblen.c \
	loadrule.c \
	loc_rite.c \
	locuni.c \
	lsetloc.c \
	lstrchr.c \
	lstrcoll.c \
	lstrcspn.c \
	lstrftim.c \
	lstrpbrk.c \
	lstrrchr.c \
	lstrrev.c \
	lstrspn.c \
	lstrstr.c \
	lstrupr.c \
	lstrxfrm.c \
	ltoa.c \
	ltruncat.c \
	mantissa.c \
	nextchar.c \
	pathfind.c \
	prevchar.c \
	printf.c \
	reorder.c \
	sprintf.c \
	strbcpy.c \
	strimon.c \
	strlen.c \
	strmon.c \
	strncoll.c \
	strncpy.c \
	strnum.c \
	unicat.c \
	unichr.c \
	unicomp.c \
	unicpy.c \
	unicspn.c \
	uniicmp.c \
	unilen.c \
	uniloc.c \
	unincat.c \
	unincpy.c \
	uninset.c \
	uniopen.c \
	unipbrk.c \
	unipcpy.c \
	unirchr.c \
	uniread.c \
	unirev.c \
	uniset.c \
	unispn.c \
	unistr.c \
	unitok.c \
	vfprintf.c \
	vprintf.c \
	vsprintf.c \
	wreorder.c \
	wsprintf.c \
	xlate.c

OBJS_O_1 = \
	_llocale.o \
	atoi.o \
	chartype.o \
	charupr.o \
	charval.o \
	fprintf.o \
	getcntry.o \
	getcoll.o \
	getvect.o \
	inc.o \
	inituni.o \
	isalnum.o \
	isalpha.o \
	isdigit.o \
	itoa.o \
	libnwlocale_mt.o \
	linschar.o \
	llocalec.o \
	llocaled.o \
	lmblen.o \
	loadrule.o \
	loc_rite.o \
	locuni.o \
	lsetloc.o \
	lstrchr.o \
	lstrcoll.o \
	lstrcspn.o \
	lstrftim.o \
	lstrpbrk.o \
	lstrrchr.o \
	lstrrev.o \
	lstrspn.o \
	lstrstr.o \
	lstrupr.o \
	lstrxfrm.o \
	ltoa.o \
	ltruncat.o \
	mantissa.o \
	nextchar.o \
	pathfind.o \
	prevchar.o \
	printf.o \
	reorder.o \
	sprintf.o \
	strbcpy.o \
	strimon.o \
	strlen.o \
	strmon.o \
	strncoll.o \
	strncpy.o \
	strnum.o \
	unicat.o \
	unichr.o \
	unicomp.o \
	unicpy.o \
	unicspn.o \
	uniicmp.o \
	unilen.o \
	uniloc.o \
	unincat.o \
	unincpy.o \
	uninset.o \
	uniopen.o \
	unipbrk.o \
	unipcpy.o \
	unirchr.o \
	uniread.o \
	unirev.o \
	uniset.o \
	unispn.o \
	unistr.o \
	unitok.o \
	vfprintf.o \
	vprintf.o \
	vsprintf.o \
	wreorder.o \
	wsprintf.o \
	xlate.o
 
OBJS_P_1=$(OBJS_O_1:.o=.P)

.c.P:
	$(CC) -Wa,-o,$*.P $(CFLAGS) $(DEFLIST) $(PICFLAG) -c $*.c

all:	dota dotso

dota: $(OBJS_O_1) 
	$(AR) $(ARFLAGS) $(DOTA) $(OBJS_O_1) 

dotso: $(OBJS_P_1) 
	$(LD) -r -o $(DOTSO).r1 $(OBJS_P_1)
	$(LD) $(LFLAGS) $(HFLAGS) -o $(DOTSO) $(DOTSO).r1

clean:
	rm -f $(OBJS_P_1)
	rm -f $(OBJS_O_1)

clobber: clean
	rm -f $(DOTA) $(DOTSO) $(DOTSO).r1

install: all
	$(INS) -f $(USRLIB) -m 644 $(DOTA)
	$(INS) -f $(USRLIB) -m 755 $(DOTSO)

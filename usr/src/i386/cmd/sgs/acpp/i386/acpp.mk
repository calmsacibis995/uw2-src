#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)acpp:i386/acpp.mk	1.18"
#	acpp Makefile
#
#	External Directories
#
include $(CMDRULES)
MDPINC	= $(CPUINC)
INSDIR  = $(CCSLIB)
#
#	Internal Directories
#
CPPCOM	= $(SGSBASE)/acpp/common
PROFDIR=        ./profdir
PROFOUT=        ./profdir
#
#	Compilation Parameters
#
NODBG	= 
DEBUG	= 
CXREF	=
CPLUSPLUS =
DEFLIST	= $(CXREF) $(DEBUG) $(NODBG) $(CPLUSPLUS) -DTRANSITION -DFILTER -DMERGED_CPP -DC_SIGNED_RS -DC_CHSIGN -DRTOLBYTES
INCLIST = -I$(CPPCOM) -I$(MDPINC)
#
# SGSINC not used
# ENVPARMS = -DSGSINC=\"$(INCDIR)\" -DINC=\"$(INC)\"
#
ENVPARMS = -DDFLTINC=\"$(DFLTINC)\"
PREASPREDS= -DPREASPREDS="\"machine($(CPU))\",\"system(unix)\",\"cpu($(CPU))\""
PREDMACROS= -DPREDMACROS="\"$(CPU)\",\"unix\",\"__USLC__\""
ENV	=
LIBES	=
CC_CMD	= $(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) $(ENVPARMS) $(ENV) -c 
#
#	Lint Parameters
#
LINTFLAGS = -p
O	= o
#
#	Other Commands
#
INS     = $(SGSBASE)/sgs.install
#
PROT	= 775
#
FRC	=
#
#	Files making up acpp
#
OBJECTS	= buf.$O chartbl.$O expand.$O file.$O group.$O lex.$O \
	  main.$O  predicate.$O syms.$O token.$O xpr.$O

SOURCES = $(CPPCOM)/buf.c $(CPPCOM)/chartbl.c $(CPPCOM)/expand.c $(CPPCOM)/file.c \
        $(CPPCOM)/group.c $(CPPCOM)/lex.c $(CPPCOM)/main.c $(CPPCOM)/predicate.c \
        $(CPPCOM)/syms.c $(CPPCOM)/token.c $(CPPCOM)/xpr.c

HFILES  = $(CPPCOM)/interface.h $(CPPCOM)/syms.h $(CPPCOM)/cpp.h $(CPPCOM)/group.h \
        $(CPPCOM)/predicate.h

all:	acpp.o

acpp.o:	$(OBJECTS) $(HFILES) $(FRC)
	$(LD) -r -o acpp.o $(OBJECTS)


buf.$O:	$(CPPCOM)/buf.c $(CPPCOM)/buf.h $(CPPCOM)/file.h $(CPPCOM)/group.h \
	$(CPPCOM)/syms.h $(CPPCOM)/predicate.h $(CPPCOM)/cpp.h \
	$(CPPCOM)/interface.h $(FRC)
	$(CC_CMD) $(CPPCOM)/buf.c

chartbl.$O: $(CPPCOM)/chartbl.c $(CPPCOM)/cpp.h
	$(CC_CMD) $(CPPCOM)/chartbl.c

expand.$O: $(CPPCOM)/expand.c $(CPPCOM)/buf.h $(CPPCOM)/syms.h \
	$(CPPCOM)/predicate.h $(CPPCOM)/cpp.h $(CPPCOM)/interface.h $(FRC)
	$(CC_CMD) $(CPPCOM)/expand.c

file.$O: $(CPPCOM)/file.c $(CPPCOM)/buf.h $(CPPCOM)/file.h \
	$(CPPCOM)/cpp.h $(CPPCOM)/interface.h $(FRC)
	$(CC_CMD) $(CPPCOM)/file.c

group.$O: $(CPPCOM)/group.c $(CPPCOM)/file.h $(CPPCOM)/group.h\
	$(CPPCOM)/syms.h $(CPPCOM)/cpp.h $(CPPCOM)/interface.h $(FRC)
	$(CC_CMD) $(CPPCOM)/group.c

lex.$O: $(CPPCOM)/lex.c $(CPPCOM)/buf.h $(CPPCOM)/syms.h \
	$(CPPCOM)/cpp.h $(CPPCOM)/interface.h $(FRC)
	$(CC_CMD) $(CPPCOM)/lex.c

main.$O: $(CPPCOM)/main.c $(CPPCOM)/buf.h $(CPPCOM)/file.h $(CPPCOM)/group.h\
	$(CPPCOM)/syms.h $(CPPCOM)/predicate.h $(CPPCOM)/cpp.h \
	$(CPPCOM)/interface.h $(FRC)
	$(CC_CMD) $(CPPCOM)/main.c

predicate.$O: $(CPPCOM)/predicate.c $(CPPCOM)/predicate.h $(CPPCOM)/cpp.h \
	$(CPPCOM)/interface.h $(FRC)
	$(CC_CMD) $(PREASPREDS) $(CPPCOM)/predicate.c

syms.$O: $(CPPCOM)/syms.c $(CPPCOM)/buf.h $(CPPCOM)/file.h $(CPPCOM)/syms.h \
	$(CPPCOM)/cpp.h $(CPPCOM)/interface.h $(FRC)
	$(CC_CMD) $(PREDMACROS) $(CPPCOM)/syms.c

token.$O: $(CPPCOM)/token.c $(CPPCOM)/buf.h $(CPPCOM)/cpp.h $(CPPCOM)/interface.h \
	$(FRC)
	$(CC_CMD) $(CPPCOM)/token.c

xpr.$O: $(CPPCOM)/xpr.c $(CPPCOM)/syms.h $(CPPCOM)/cpp.h $(CPPCOM)/interface.h $(FRC)
	$(CC_CMD) $(CPPCOM)/xpr.c
shrink:	clobber
#
clean :
	$(RM) -f $(OBJECTS)
	$(RM) -f $(OBJECTS:$O=ln)
#
clobber: clean
	$(RM) -f acpp.o
#
install : $(INSDIR)/$(SGS)acpp
#
$(INSDIR)/$(SGS)acpp:	acpp
	$(CP) acpp acpp.bak
	$(STRIP) acpp
	sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)acpp acpp
	mv acpp.bak acpp
#
save:
	$(RM) -f $(INSDIR)/$(SGS)acpp.back
	$(CP) $(INSDIR)/$(SGS)acpp $(INSDIR)/$(SGS)acpp.back
#
uninstall:	$(INSDIR)/$(SGS)acpp.back
	$(RM) -f $(INSDIR)/$(SGS)acpp
	$(CP) $(INSDIR)/$(SGS)acpp.back $(INSDIR)/$(SGS)acpp
#
lint:	$(SOURCES)
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(SOURCES)
#
acpp.ln:	$(SOURCES)
	$(RM) -f $(OBJECTS:$O=ln)
	$(LINT) -c $(LINTFLAGS) $(DEFLIST) $(SOURCES)
	cat *.ln >acpp.ln
#
listing:	$(SOURCES) 
	$(PRINT) $(PRFLAGS) $(SOURCES) | $(LP)
#
FRC:

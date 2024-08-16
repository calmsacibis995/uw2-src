#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:psm/ast/ast.mk	1.4"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE = ast.mk
DIR = psm/ast
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/ast.ln

LOCALINC = -I.

AST = ast.cf/Driver.o

MODULES = $(AST)

sysHeaders=ebi.h

FILES = \
	ast.o \
	ast_phys.o \
	astpic.o \
	clockintr.o \
	intr_p.o \
	nmi.o \
	spl.o

CFILES = \
	ast.c \
	astpic.c \
	clockintr.c \
	nmi.c

SFILES = \
	ast_phys.s \
	intr_p.s \
	spl.s

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	ast.ln \
	astpic.ln \
	clockintr.ln \
	nmi.ln

all:	$(MODULES)

install: all
	cd ast.cf; $(IDINSTALL) -R$(CONF) -M ast 

$(AST): $(FILES)
	$(LD) -r -o $(AST) $(FILES)

astsym.h: symbols_ast.c
	$(CC) -UDEBUG $(CFLAGS) $(DEFLIST) $(INCLIST) -S symbols_ast.c
	awk -f symbols_ast.awk symbols_ast.s | \
	sed -e '1,$$s;__SYMBOL__;;' > astsym_c.h
	sed -e \
's/^#define	\(.*\)	\[\(.*\)\]/	define(`\1'\'',`ifelse($$#,0,`\2'\'',`\2'\''($$@))'\'')/' \
		astsym_c.h > astsym.h
	rm -f symbols_ast.s

ast_phys.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

intr_p.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h astsym.h

spl.o:		$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h astsym.h

clean:
	-rm -f *.o $(LFILES) *.L $(AST) astsym.h symbols_ast.s

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e ast 

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

$(LINTDIR):
	-mkdir -p $@

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep

#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cplusfe:i386/Makefile	1.16"

#
#	C++ front end makefile
#
include $(CMDRULES)

LINTFLAGS=
LINK_MODE=

LDLIBS=

INS=$(SGSBASE)/sgs.install

FE_OBJECTS = fe/c_gen_be.o fe/cfe.o fe/class_decl.o fe/cmd_line.o \
	fe/const_ints.o fe/debug.o fe/decl_inits.o fe/decls.o fe/def_arg.o \
	fe/error.o fe/expr.o fe/exprutil.o fe/fe_init.o fe/fe_wrapup.o \
	fe/float_pt.o fe/folding.o fe/host_envir.o fe/il.o fe/il_display.o \
	fe/il_read.o fe/il_walk.o fe/il_write.o fe/layout.o fe/lexical.o \
	fe/literals.o fe/lower_eh.o fe/lower_il.o fe/lower_init.o \
	fe/lower_name.o fe/macro.o fe/mem_manage.o \
	fe/mem_tables.o fe/overload.o fe/preproc.o fe/statements.o \
	fe/symbol_tbl.o fe/sys_predef.o \
	fe/target.o fe/templates.o fe/types.o

DISP_OBJECTS = disp/const_ints.o disp/debug.o disp/error.o \
	disp/float_pt.o disp/host_envir.o disp/il.o disp/il_display.o \
	disp/il_read.o disp/il_walk.o disp/mem_manage.o disp/mem_tables.o \
	disp/target.o

INCLUDES = basics.h c_gen_be.h class_decl.h \
	cmd_line.h const_ints.h \
	debug.h decl_inits.h decls.h \
	defines.h def_arg.h error.h expr.h exprutil.h \
	fe_init.h fe_wrapup.h float_pt.h \
	folding.h getopt.h host_envir.h il.h il_def.h \
	il_display.h il_file.h il_read.h \
	il_walk.h il_write.h \
	lang_feat.h layout.h lexical.h \
	literals.h lower_eh.h \
	lower_il.h lower_init.h \
	lower_name.h macro.h mem_manage.h \
	mem_tables.h overload.h preproc.h \
	statements.h symbol_tbl.h sys_predef.h \
	targ_def.h target.h templates.h \
	trans_lims.h types.h version.h \
	walk_entry.h

PROBEFILE = $(COMDIR)/c_gen_be.c

PRODUCTS = c++fe c++disp
BINARIES = fe.o disp.o

DEBUG=0
FE_DEFLIST = -DDEBUG=$(DEBUG) -DCHECKING -D__SYSV__ -DDEM_MAIN -DSHOBJ_SUPPORT
DISP_DEFLIST = -DSTANDALONE_IL_DISPLAY -D__SYSV__

INCLIST= -I. -I$(COMDIR) -I$(CPUINC)

FE_CC_CMD=$(CC) $(CFLAGS) $(FE_DEFLIST) $(INCLIST)
DISP_CC_CMD=$(CC) $(CFLAGS) $(DISP_DEFLIST) $(INCLIST)

all:
	if [ -f $(PROBEFILE) ]; \
	then \
		find $(BINARIES) \( ! -type f -o -links +1 \) \
		    -exec rm -f {} \; 2>/dev/null ; \
		$(MAKE) binaries $(MAKEARGS) DEBUG=$(DEBUG) ; \
	else \
		for file in $(BINARIES); \
		do \
			if [ ! -r $$file ]; \
			then \
				echo "ERROR: $$file is missing" 1>&2; \
				false; \
				break; \
			fi \
		done \
	fi

.MUTEX: fe disp c++fe c++disp

binaries : fe disp c++fe c++disp

c++fe : $(FE_OBJECTS) usr_include.o $(LDLIBS)
	$(LD) -r $(FE_OBJECTS) -o fe.o
	$(CC) $(FE_OBJECTS) usr_include.o $(LDLIBS) $(LINK_MODE) $(LIBSGS) -o $(@)

c++disp : $(DISP_OBJECTS) usr_include.o $(LDLIBS)
	$(LD) -r $(DISP_OBJECTS) -o disp.o
	$(CC) $(DISP_OBJECTS) usr_include.o $(LDLIBS) $(LINK_MODE) $(LIBSGS) -o $(@)

# Too bad if there are plain files named fe or disp here...

fe:
	mkdir $(@)

disp:
	mkdir $(@)

.MUTEX: all disp.o fe.o usr_include.o

install : all disp.o fe.o usr_include.o
	$(CC) fe.o usr_include.o $(LDLIBS) $(LINK_MODE) $(LIBSGS) -o c++fe
	$(MV) c++fe c++fe.bak
	$(CP) c++fe.bak c++fe
	$(STRIP) c++fe
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(CCSLIB)/$(SGS)c++fe c++fe
	$(MV) c++fe.bak c++fe
	$(CC) disp.o usr_include.o $(LDLIBS) $(LINK_MODE) $(LIBSGS) -o c++disp
	$(MV) c++disp c++disp.bak
	$(CP) c++disp.bak c++disp
	$(STRIP) c++disp
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(CCSBIN)/$(SGS)c++disp c++disp
	$(MV) c++disp.bak c++disp

lintit : $(HFILES) $(SOURCES)
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $(SOURCES)

clean:
	-rm -f $(FE_OBJECTS) $(DISP_OBJECTS) usr_include.o

shrink clobber : clean
	-if [ -f $(PROBEFILE) ]; \
	then \
		rm -f $(PRODUCTS); \
	fi

fe/c_gen_be.o : $(COMDIR)/c_gen_be.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/cfe.o : $(COMDIR)/cfe.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/class_decl.o : $(COMDIR)/class_decl.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/cmd_line.o : $(COMDIR)/cmd_line.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/const_ints.o : $(COMDIR)/const_ints.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/const_ints.o : $(COMDIR)/const_ints.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/debug.o : $(COMDIR)/debug.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/debug.o : $(COMDIR)/debug.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/decl_inits.o : $(COMDIR)/decl_inits.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/decls.o : $(COMDIR)/decls.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/def_arg.o : $(COMDIR)/def_arg.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/error.o : $(COMDIR)/error.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/error.o : $(COMDIR)/error.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/expr.o : $(COMDIR)/expr.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/exprutil.o : $(COMDIR)/exprutil.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/fe_init.o : $(COMDIR)/fe_init.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/fe_wrapup.o : $(COMDIR)/fe_wrapup.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/float_pt.o : $(COMDIR)/float_pt.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/float_pt.o : $(COMDIR)/float_pt.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/folding.o : $(COMDIR)/folding.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/host_envir.o : $(COMDIR)/host_envir.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/host_envir.o : $(COMDIR)/host_envir.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/il.o : $(COMDIR)/il.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/il.o : $(COMDIR)/il.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/il_display.o : $(COMDIR)/il_display.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/il_display.o : $(COMDIR)/il_display.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/il_read.o : $(COMDIR)/il_read.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/il_read.o : $(COMDIR)/il_read.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/il_walk.o : $(COMDIR)/il_walk.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/il_walk.o : $(COMDIR)/il_walk.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/target.o : $(COMDIR)/target.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/il_write.o : $(COMDIR)/il_write.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/layout.o : $(COMDIR)/layout.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/lexical.o : $(COMDIR)/lexical.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/literals.o : $(COMDIR)/literals.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/lower_eh.o : $(COMDIR)/lower_eh.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/lower_il.o : $(COMDIR)/lower_il.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/lower_init.o : $(COMDIR)/lower_init.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/lower_name.o : $(COMDIR)/lower_name.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/macro.o : $(COMDIR)/macro.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/mem_manage.o : $(COMDIR)/mem_manage.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/mem_manage.o : $(COMDIR)/mem_manage.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/mem_tables.o : $(COMDIR)/mem_tables.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/mem_tables.o : $(COMDIR)/mem_tables.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/overload.o : $(COMDIR)/overload.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/preproc.o : $(COMDIR)/preproc.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/statements.o : $(COMDIR)/statements.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/symbol_tbl.o : $(COMDIR)/symbol_tbl.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/sys_predef.o : sys_predef.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/target.o : $(COMDIR)/target.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/templates.o : $(COMDIR)/templates.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/types.o : $(COMDIR)/types.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

usr_include.o : $(COMDIR)/usr_include.c
	$(CC) $(CFLAGS) $(INCLIST) -c $?

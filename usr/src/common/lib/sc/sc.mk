#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sc:sc.mk	1.37"

# SC top-level makefile

# Making Standard Components consists of building headers (including 
# template bodies), libraries, and tools (which may consist of scripts 
# and/or binary executables).
#
# The idea with this makefile is that a normal make will build
# the headers, libraries, and tools into a mini CCS-like structure
# that is local, for development, testing, etc.  This structure is
# rooted at $(LOCALSC).  Thus, with $(LOCALSC) defaulting to "." the local
# structure would consist of
# 	./usr/include/CC
#	./usr/ccs/bin
#	./usr/ccs/lib
# These directories are named by macros LSCINC, LSCBIN, and LSCLIB 
# respectively.  (These should not be confused with macros SCBIN and SCLIB, 
# which refer to the location of tools used in the SC make process itself.)
#
# Thus, to make SC, it suffices to 
#	get out the SC component
#	cd <top-level-SC-directory>
#	get out the standard LIBRULES file
#	make -f sc.mk LIBRULES=<that-LIBRULES-file>
# You may also want to define C++C to the CC command you want to use;
# otherwise it will be searched for in your PATH.
#
# In making each individual component (whether library or tool), the top-level
# make goes into the component's directory and invokes a makefile there.
#
# All components that contain library routines go into a single library, 
# lib++a., except for fs and G2++, which are part library, part tool.  
# The component parts of these go into separate libraries libfs.a and libg2++.a.
# (Note that libfs.a has to be separate, since it contains variant new and
# delete implementations; not sure why libg2++.a is separate other than build
# convenience.)
#
# As artifacts of the build process, additional copies of libraries or 
# executables may be created in the various component directories, or in the 
# current directory.  In addition, libraries for each component are created 
# in the current directory.  These may be ignored; it is best just to reference 
# the local CCS-like structure.  (They cannot be eliminated in the current
# scheme because they are part of the dependency chain for lib++a.)
#
# A "make install" will first do a regular "make", then copy the
# headers, libraries, and tools from the local CCS-like structure to
# the real target CCS.  The macros naming the real directories have the 
# conventional rules files names, e.g. CCSBIN.
#
# A "make demo" will build, execute, and verify some demo programs that use
# Standard Components.  It may be used as a simple test suite, before the
# full Standard Components test suite (which is held in a different component)
# is run.

include $(LIBRULES)

SGSBASE = ../../cmd/sgs
CCINC = $(INC)/CC
LSCINC = $(LOCALSC)/usr/include/CC
LSCBIN = $(LOCALSC)/usr/ccs/bin
LSCLIB = $(LOCALSC)/usr/ccs/lib
DEFLIST=-DNDEBUG_ATTLC -DSYSV
INCLIST=-I./incl -I. -I../$(LSCINC)
LINKDIRS= -L.. -L.
ARCHIVES= -l++
SCBIN= local/bin
SCLIB= local/lib
SYS=SYSV
LOCALSC=.

C++FLAGS = $(CFLAGS) -Tauto

CC_CMD = $(CC) $(CFLAGS) $(DEFLIST)
C++CMD = $(C++C) $(C++FLAGS) $(DEFLIST) $(INCLIST)

# Installable headers and template source code

HEADERS = $(LSCINC)/Args.h \
	$(LSCINC)/Array_alg.c $(LSCINC)/Array_alg.h \
	$(LSCINC)/Bits.h \
	$(LSCINC)/Block.c $(LSCINC)/Block.h $(LSCINC)/Blockio.c $(LSCINC)/Blockio.h \
	$(LSCINC)/Fsm.h \
	$(LSCINC)/G2text.h $(LSCINC)/Vblock.c $(LSCINC)/Vblock.h $(LSCINC)/g2++.h \
	$(LSCINC)/g2comp.h $(LSCINC)/g2ctype.h $(LSCINC)/g2debug.h \
	$(LSCINC)/g2desc.h $(LSCINC)/g2inline.h $(LSCINC)/g2io.h \
	$(LSCINC)/g2manip.h $(LSCINC)/g2tree.h \
	$(LSCINC)/Graph.h $(LSCINC)/Ticket.h \
	$(LSCINC)/Graph_alg.h \
	$(LSCINC)/List.c $(LSCINC)/List.h $(LSCINC)/Listio.c $(LSCINC)/Listio.h \
	$(LSCINC)/Map.c $(LSCINC)/Map.h $(LSCINC)/Mapio.c $(LSCINC)/Mapio.h \
	$(LSCINC)/Objection.h \
	$(LSCINC)/Path.h $(LSCINC)/Search_path.h $(LSCINC)/Tmppath.h $(LSCINC)/ksh_test.h \
	$(LSCINC)/Pool.h \
	$(LSCINC)/Regex.h \
	$(LSCINC)/Set.h \
	$(LSCINC)/bag.c $(LSCINC)/bag.h $(LSCINC)/bagio.c $(LSCINC)/bagio.h \
	$(LSCINC)/set.c $(LSCINC)/set.h $(LSCINC)/setio.c $(LSCINC)/setio.h \
	$(LSCINC)/set_of_p.c $(LSCINC)/set_of_p.h $(LSCINC)/set_of_pio.c $(LSCINC)/set_of_pio.h \
	$(LSCINC)/Stopwatch.h \
	$(LSCINC)/String.h \
	$(LSCINC)/Strstream.h \
	$(LSCINC)/Symbol.h \
	$(LSCINC)/Time.h \
	$(LSCINC)/ipcmonitor.h $(LSCINC)/ipcstream.h \
	$(LSCINC)/fs.h

# Individual libraries (but in the end, all of these go into lib++a.)
LIBS = libPool.a libString.a libipc.a libArgs.a libBits.a \
	libFsm.a libGraph.a libGA.a libList.a \
	libMap.a libPath.a libObject.a libRegex.a libSet.a \
	libSwatch.a libStrstr.a libSymbol.a libTime.a

# Component directories that go into lib++.a:
LIBDIRS = Pool String ipc Args Array_alg Bits Block Fsm Graph Graph_alg \
	List Map Path Objection Regex Set Stopwatch Strstream Symbol \
	Time

# Tool components libraries and executables:
TOOLZ = $(LSCLIB)/fsipp $(LSCLIB)/libfs.a \
	$(LSCINC)/g2mach.h $(LSCINC)/g2values.h $(LSCBIN)/g2++comp $(LSCLIB)/libg2++.a \
	$(LSCBIN)/hier $(LSCLIB)/hier2 \
	$(LSCBIN)/incl $(LSCLIB)/incl2 \
	$(LSCBIN)/publik $(LSCLIB)/publik2
 
# Tool components directories:
TOOLDIRS = fs G2++ incl publik hier

ALLDIRS = $(LIBDIRS) $(TOOLDIRS)

# Components with demos:
DEMODIRS = Pool String ipc Args Block Fsm Graph Graph_alg \
	List Map Path Regex Set Stopwatch Symbol Time G2++ fs

# Directories making up the local CCS-like SC structure
LOCAL_DIRS = $(LOCALSC)/usr/include/CC $(LOCALSC)/usr/ccs/bin $(LOCALSC)/usr/ccs/lib
 
.MUTEX: local_dirs headers components lib++.a tools

all: local_dirs headers components lib++.a tools

.MUTEX: all install_SC

install: all install_SC

install_SC: install_headers install_bin install_lib

install_headers:
	for i in $(LSCINC)/*.[hc] ; \
	do \
		$(RM) -f $(CCINC)/`basename $$i` ; \
		$(CP) $$i $(CCINC) ; \
	done
	# deficiency: headers don't have individual change granularity

install_bin: $(CCSBIN)/g2++comp $(CCSBIN)/hier $(CCSBIN)/incl \
	$(CCSBIN)/publik

$(CCSBIN)/g2++comp : $(LSCBIN)/g2++comp
	sh $(SGSBASE)/sgs.install 755 $(OWN) $(GRP) $(@) $(LSCBIN)/g2++comp
	$(STRIP) $(@)

$(CCSBIN)/hier : $(LSCBIN)/hier
	sh $(SGSBASE)/sgs.install 755 $(OWN) $(GRP) $(@) $(LSCBIN)/hier

$(CCSBIN)/incl : $(LSCBIN)/incl
	sh $(SGSBASE)/sgs.install 755 $(OWN) $(GRP) $(@) $(LSCBIN)/incl

$(CCSBIN)/publik : $(LSCBIN)/publik
	sh $(SGSBASE)/sgs.install 755 $(OWN) $(GRP) $(@) $(LSCBIN)/publik

install_lib: $(CCSLIB)/fsipp $(CCSLIB)/hier2 $(CCSLIB)/incl2 \
	$(CCSLIB)/lib++.a $(CCSLIB)/libfs.a $(CCSLIB)/libg2++.a \
	$(CCSLIB)/publik2

$(CCSLIB)/fsipp : $(LSCLIB)/fsipp
	sh $(SGSBASE)/sgs.install 755 $(OWN) $(GRP) $(@) $(LSCLIB)/fsipp
	$(STRIP) $(@)

$(CCSLIB)/hier2 : $(LSCLIB)/hier2
	sh $(SGSBASE)/sgs.install 755 $(OWN) $(GRP) $(@) $(LSCLIB)/hier2
	$(STRIP) $(@)

$(CCSLIB)/incl2 : $(LSCLIB)/incl2
	sh $(SGSBASE)/sgs.install 755 $(OWN) $(GRP) $(@) $(LSCLIB)/incl2
	$(STRIP) $(@)

$(CCSLIB)/lib++.a : $(LSCLIB)/lib++.a
	sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(@) $(LSCLIB)/lib++.a

$(CCSLIB)/libfs.a : $(LSCLIB)/libfs.a
	sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(@) $(LSCLIB)/libfs.a

$(CCSLIB)/libg2++.a : $(LSCLIB)/libg2++.a
	sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(@) $(LSCLIB)/libg2++.a
 
$(CCSLIB)/publik2 : $(LSCLIB)/publik2
	sh $(SGSBASE)/sgs.install 755 $(OWN) $(GRP) $(@) $(LSCLIB)/publik2
	$(STRIP) $(@)


local_dirs: $(LOCAL_DIRS)

$(LOCALSC)/usr/include/CC:
	-mkdir -p $@

$(LOCALSC)/usr/ccs/bin:
	-mkdir -p $@

$(LOCALSC)/usr/ccs/lib:
	-mkdir -p $@

.MUTEX: $(HEADERS)

headers: $(HEADERS)

components :
	set -x; \
	for dir in $(LIBDIRS); \
	do \
		cd $$dir; $(MAKE) \
			DEFLIST="$(DEFLIST)" \
			INCLIST="$(INCLIST)" \
			SYS="$(SYS)" \
			CFLAGS="$(CFLAGS)" \
			C++FLAGS="$(C++FLAGS)" \
			CC_CMD="$(CC) $(CFLAGS) $(DEFLIST)" \
			C++CMD="$(C++C) $(C++FLAGS) $(DEFLIST) $(INCLIST)"; \
		cd ..; \
	done

# The trick here is that we know all our .o's are up to date if and only if 
# all the components are, and we make all the components before we
# even attempt to build lib++.a.  Then we use our directory list to gather
# up the complete list of .o's to put in the big library lib++.a.

lib++.a : $(LIBS)
	rm -f $(@); \
	$(AR) $(ARFLAGS) $(@) `find $(LIBDIRS) -name "*.o" -print | xargs $(LORDER) | $(TSORT)`; \
	$(RM) -f $(LSCLIB)/$(@); \
	$(CP) $(@) $(LSCLIB)/$(@)

tooldirs : lib++.a
	set -x; \
	for dir in $(TOOLDIRS); \
	do \
		cd $$dir; $(MAKE) \
			DEFLIST="$(DEFLIST)" \
			INCLIST="$(INCLIST)" \
			LINKDIRS="$(LINKDIRS)" \
			CFLAGS="$(CFLAGS)" \
			C++FLAGS="$(C++FLAGS)" \
			CC_CMD="$(CC) $(CFLAGS) $(DEFLIST)" \
			C++CMD="$(C++C) $(C++FLAGS) $(DEFLIST) $(INCLIST) $(LINKDIRS)" \
			ARCHIVES="$(ARCHIVES)"; \
		cd ..; \
	done

.MUTEX:	tooldirs $(TOOLZ) 

tools : tooldirs $(TOOLZ)

# Tool dependencies

$(LSCLIB)/fsipp : fs/fsippsrc/fsipp
	$(RM) -f $(@)
	$(CP) $(?) $(@)
 
$(LSCLIB)/libfs.a : fs/libsrc/libfs.a
	$(RM) -f $(@)
	$(CP) $(?) $(@)
 
$(LSCINC)/g2mach.h : G2++/incl/g2mach.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)
 
$(LSCINC)/g2values.h : G2++/incl/g2values.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)
 
$(LSCBIN)/g2++comp : G2++/compsrc/g2++comp
	$(RM) -f $(@)
	$(CP) $(?) $(@)
 
$(LSCLIB)/libg2++.a : G2++/g2++lib/libg2++.a
	$(RM) -f $(@)
	$(CP) $(?) $(@)
 
$(LSCBIN)/hier : hier/bin/hier
	$(RM) -f $(@)
	$(CP) $(?) $(@)
 
$(LSCLIB)/hier2 : hier/hier2
	$(RM) -f $(@)
	$(CP) $(?) $(@)
 
$(LSCBIN)/incl : incl/bin/incl
	$(RM) -f $(@)
	$(CP) $(?) $(@)
 
$(LSCLIB)/incl2 : incl/incl2
	$(RM) -f $(@)
	$(CP) $(?) $(@)
 
$(LSCBIN)/publik : publik/bin/publik
	$(RM) -f $(@)
	$(CP) $(?) $(@)
 
$(LSCLIB)/publik2 : publik/publik2
	$(RM) -f $(@)
	$(CP) $(?) $(@)
 
# Header dependencies

$(LSCINC)/Args.h : Args/incl/Args.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Array_alg.c : Array_alg/incl/Array_alg.c
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Array_alg.h : Array_alg/incl/Array_alg.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Bits.h : Bits/incl/Bits.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Block.c : Block/incl/Block.c
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Block.h : Block/incl/Block.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Blockio.c : Block/incl/Blockio.c
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Blockio.h : Block/incl/Blockio.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Fsm.h : Fsm/incl/Fsm.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/G2text.h : G2++/incl/G2text.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Vblock.c : G2++/incl/Vblock.c
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Vblock.h : G2++/incl/Vblock.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/g2++.h : G2++/incl/g2++.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/g2comp.h : G2++/incl/g2comp.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/g2ctype.h : G2++/incl/g2ctype.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/g2debug.h : G2++/incl/g2debug.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/g2desc.h : G2++/incl/g2desc.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/g2inline.h : G2++/incl/g2inline.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/g2io.h : G2++/incl/g2io.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/g2manip.h : G2++/incl/g2manip.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/g2tree.h : G2++/incl/g2tree.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Graph.h : Graph/incl/Graph.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Ticket.h : Graph/incl/Ticket.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Graph_alg.h : Graph_alg/incl/Graph_alg.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/List.c : List/incl/List.c
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/List.h : List/incl/List.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Listio.c : List/incl/Listio.c
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Listio.h : List/incl/Listio.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Map.c : Map/incl/Map.c
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Map.h : Map/incl/Map.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Mapio.c : Map/incl/Mapio.c
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Mapio.h : Map/incl/Mapio.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Objection.h : Objection/incl/Objection.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Path.h : Path/incl/Path.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Search_path.h : Path/incl/Search_path.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Tmppath.h : Path/incl/Tmppath.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/ksh_test.h : Path/incl/ksh_test.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Pool.h : Pool/incl/Pool.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Regex.h : Regex/incl/Regex.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Set.h : Set/incl/Set.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/bag.c : Set/incl/bag.c
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/bag.h : Set/incl/bag.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/bagio.c : Set/incl/bagio.c
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/bagio.h : Set/incl/bagio.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/set.c : Set/incl/set.c
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/set.h : Set/incl/set.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/setio.c : Set/incl/setio.c
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/setio.h : Set/incl/setio.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/set_of_p.c : Set/incl/set_of_p.c
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/set_of_p.h : Set/incl/set_of_p.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/set_of_pio.c : Set/incl/set_of_pio.c
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/set_of_pio.h : Set/incl/set_of_pio.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Stopwatch.h : Stopwatch/incl/Stopwatch.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/String.h : String/incl/String.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Strstream.h : Strstream/incl/Strstream.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Symbol.h : Symbol/incl/Symbol.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/Time.h : Time/incl/Time.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/ipcmonitor.h : ipc/incl/ipcmonitor.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/ipcstream.h : ipc/incl/ipcstream.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

$(LSCINC)/fs.h : fs/incl/fs.h
	$(RM) -f $(@)
	$(CP) $(?) $(@)

demos :
	set -x; \
	for dir in $(DEMODIRS); \
	do \
		echo Doing demos for $$dir ; \
		cd $$dir/demos; $(MAKE) -i \
			LSCINC="$(LSCINC)" \
			LSCBIN="$(LSCBIN)" \
			LSCLIB="$(LSCLIB)" \
			SCBIN="../../$(SCBIN)" \
			SCLIB="../../$(SCLIB)" \
			DEFLIST="$(DEFLIST)" \
			CFLAGS="$(CFLAGS)" \
			XSCC="$(C++C)" \
			C++FLAGS="$(C++FLAGS)" \
			CC_CMD="$(CC) $(CFLAGS) $(DEFLIST)" \
			C++CMD="$(C++C) $(C++FLAGS) $(DEFLIST) $(INCLIST) $(LINKDIRS)"; \
		cd ../..; \
	done

clean :
	set -x; \
	$(RM) -f $(LIBS) lib++.a; \
	for dir in $(ALLDIRS); \
	do \
		cd $$dir; $(MAKE) clean SCLIB="../$(SCLIB)" ; \
		cd ..; \
	done

clobber : clean
	set -x; \
	$(RM) -rf $(LOCALSC)/usr; \
	for dir in $(ALLDIRS); \
	do \
		cd $$dir; $(MAKE) clobber SCLIB="../$(SCLIB)" ; \
		cd ..; \
	done

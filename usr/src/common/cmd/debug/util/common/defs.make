#ident	"@(#)debugger:util/common/defs.make	1.8"

CPLUS	= $(C++C)
CCFLAGS	= $(CFLAGS)
LDLIBS	= 
INSDIR	= $(CCSBIN)

UIBASE = $(ROOT)/$(MACH)/usr/X
SGSBASE	= $(PRODDIR)/../sgs
COMINC	= $(SGSBASE)/inc/common
CPUINC = $(SGSBASE)/inc/$(CPU)
FPECOM = $(SGSBASE)/fpemu/common
FPECPU = $(SGSBASE)/fpemu/$(CPU)

PRODDIR = ../..

PRODLIB	= $(PRODDIR)/lib
PRODINC	= $(PRODDIR)/inc
INCCOM	= $(PRODDIR)/inc/common
INCCPU	= $(PRODDIR)/inc/$(CPU)
COMMON	= ../common

# MULTIBYTE support
MBDEF	=  #-DMULTIBYTE
MBLIB	=  #-lw

INCLIST1= -I. -I$(COMMON) -I$(INCCPU) -I$(INCCOM)
INCLIST2= -I$(CPUINC) -I$(COMINC) -I$(FPECOM) -I$(FPECPU)

DFLAGS	=

CC_CMD_FLAGS = $(CFLAGS) $(INCLIST1) $(INCLIST2) $(DEFLIST) $(DFLAGS) $(MBDEF) -I$(INC)
CC_CMD	= $(CC) $(CC_CMD_FLAGS)

CPLUS_CMD_FLAGS = $(CCFLAGS) $(INCLIST1) $(INCLIST2) $(DEFLIST) $(DFLAGS)
CPLUS_CMD = $(CPLUS) $(CPLUS_CMD_FLAGS)

ARFLAGS	= qc
YFLAGS	= -ld

# The default target is used if no target is given; it must be first.

default: all

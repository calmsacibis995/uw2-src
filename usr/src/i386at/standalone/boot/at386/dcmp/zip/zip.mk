#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)stand:i386at/standalone/boot/at386/dcmp/zip/zip.mk	1.4"

# zip.mk -- make a decompression object file
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
# 
# You may contact UNIX System Laboratories by writing to
# UNIX System Laboratories, 190 River Road, Summit, NJ 07901, USA

include $(CMDRULES)

LOCALINC = -I ../../.. -I $(ROOT)/i386at/usr/include

FILES = dcmp.o

CFILES =  \
	gzip.c \
	munzip.c \
	inflate.c \
	util.c \
	rmalloc.c 

OBJFS = $(CFILES:.c=.o)

all:	$(FILES)
	cp dcmp.o ../zip.o

dcmp.o: $(OBJFS)
	${LD} -dn -r -o dcmp.o $(OBJFS)

clean:
	-/bin/rm -f $(OBJFS) 

clobber: clean
	-/bin/rm -f $(FILES) ../sip_pconf.h

FRC:

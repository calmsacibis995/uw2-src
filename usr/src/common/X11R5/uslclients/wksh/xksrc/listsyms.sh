#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)wksh:xksrc/listsyms.sh	1.1"

#	Copyright (c) 1990, 1991 AT&T and UNIX System Laboratories, Inc. 
#	All Rights Reserved     

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T    
#	and UNIX System Laboratories, Inc.			
#	The copyright notice above does not evidence any       
#	actual or intended publication of such source code.    
#
#
${NM:-nm} -ph $1 | grep " [A-TV-Z] " | cut -f3 -d" " 

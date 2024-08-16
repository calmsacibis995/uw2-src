/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef RegAccess_h
#define RegAccess_h
#ident	"@(#)debugger:inc/common/RegAccess.h	1.5"

#include "Itype.h"
#include "Reg.h"
#include "sys/regset.h"

class Core;
class Frame;
class ProcObj;

class RegAccess {
	ProcObj		*pobj;
	unsigned char	core;
	unsigned char	fpcurrent;
	unsigned char	gcurrent;
	gregset_t	*gpreg;
	fpregset_t	*fpreg;
	int		readlive( RegRef, long * );
	int		readcore( RegRef, long * );
	int		writelive( RegRef, long * );
public:
			RegAccess();
			~RegAccess() {};
	int		setup_live( ProcObj *);
	int		setup_core( ProcObj *);
	int		update();
	Iaddr		getreg( RegRef );
	int		readreg( RegRef, Stype, Itype & );
	int		writereg( RegRef, Stype, Itype & );
	int		display_regs(Frame *);
	int		set_pc(Iaddr);
};

#endif

// end of RegAccess.h

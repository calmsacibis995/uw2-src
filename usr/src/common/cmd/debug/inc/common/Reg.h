/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef REG_UNK
#define	REG_UNK		-1
#ident	"@(#)debugger:inc/common/Reg.h	1.3"


//	Typedef and struct for register names and attributes
//	(machine independent).
//
//	Each machine has its own set of register names.  This header
//	is the common interface.
//

#include "Fund_type.h"	// for enum Fund_type
#include "Itype.h"	// for enum Stype

typedef int RegRef;			// a register reference (index)

struct RegAttrs {
	RegRef	 ref;
	char	*name;
	char	 size;	// in bytes
	char	 flags;	// see below
	Stype	 stype;
	int	 offset;
};

// bits in "flags"
#define	FPREG	0x01

extern RegAttrs regs[];	// array, last entry contains reg = REG_UNK

extern RegRef regref(const char *name);	// lookup by name
extern Fund_type  regtype(RegRef);

#ifndef __cplusplus
overload regattrs;
#endif
extern RegAttrs  *regattrs(RegRef);
extern RegAttrs  *regattrs(const char *name);

#include "Reg1.h"	/* machine specific */

#endif /* REG_UNK */

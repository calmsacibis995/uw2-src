/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:lib/printers/freeprinter.c	1.9.1.4"
#ident	"$Header: $"

#include "sys/types.h"
#include "stdlib.h"

#include "lp.h"
#include "printers.h"

/**
 **  freeprinter() - FREE MEMORY ALLOCATED FOR PRINTER STRUCTURE
 **/

void			freeprinter (pp)
	PRINTER			*pp;
{
	if (!pp)
		return;
	if (pp->name)
		Free (pp->name);
	if (pp->char_sets)
		freelist (pp->char_sets);
	if (pp->input_types)
		freelist (pp->input_types);
	if (pp->device)
		Free (pp->device);
	if (pp->dial_info)
		Free (pp->dial_info);
	if (pp->fault_rec)
		Free (pp->fault_rec);
	if (pp->interface)
		Free (pp->interface);
	if (pp->printer_type)
		Free (pp->printer_type);
	if (pp->remote)
		Free (pp->remote);
	if (pp->speed)
		Free (pp->speed);
	if (pp->stty)
		Free (pp->stty);
	if (pp->description)
		Free (pp->description);
	if (pp->fault_alert.shcmd)
		Free (pp->fault_alert.shcmd);
#if	defined(CAN_DO_MODULES)
	if (pp->modules)
		freelist (pp->modules);
#endif
	if (pp->printer_types)
		freelist (pp->printer_types);
	if (pp->user)
		Free (pp->user);
	return;
}

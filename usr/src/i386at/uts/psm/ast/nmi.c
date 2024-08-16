/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/ast/nmi.c	1.3"
#ident	"$Header: $"

#include <util/types.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/kdb/xdebug.h>

#include <ebi.h>
#include <ast.h>

extern EBI_II  ast_calltab;
extern event_t ast_event_sv;
extern void dbprintf();
extern void db_stacktrace();

/*
 * int
 * ast_nmi(void)
 *	Architecture specific NMI handler for AST Manhattan SMP.
 *  
 * Calling/Exit States:
 *	Called by: 	base kernel
 *	Parameters:	none
 *	Locking: 	none
 *	Globals read:	ast_calltab, MMIOTable	
 *	Globs. written: none
 *	Returns:	none
 *
 * Description:
 *	Get source of NMI from the EBI.  If source is a fault, panic.
 *	If source is software, broadcast an event.
 *	
 */
/*ARGSUSED*/
int
ast_nmi(void)
{
	memoryErrorInfo mei;
	unsigned int source;
	char *s;
	
	if ((ast_calltab.GetNMISource)(MMIOTable, &source) == ERR_UNKNOWN_INT) 
	{
		/*
		 *+
		 */
		cmn_err(CE_PANIC, "ERROR: Unknown NMI source.");
		/*NOTREACHED*/
	}

	(ast_calltab.NonMaskableIntEOI)(MMIOTable);

	switch (source) 
	{
		case INT_IO_ERROR:
			s = "system I/O error";
			break;
		case INT_SOFT_NMI:
			/*
			 *+
			 */
			cmn_err(CE_NOTE, "AST NMI: software generated NMI");
			/*
			 * Does *not* panic.
			 */
			return ( 0 );
			/*NOTREACHED*/
			break;
		case INT_MEMORY_ERROR:
			s = "uncorrectable ECC error";
			if ((ast_calltab.GetMemErrorInfo)(MMIOTable, &mei) == 
				MEMORY_ERROR_FOUND)
			{
				/*
				 *+
				 */
				cmn_err(CE_CONT, "       AST NMI: memory error at 0x%x\n", 
						mei.location.low);
				cmn_err(CE_CONT, "                slot number     0x%x\n", 
						mei.slotNumber);
				cmn_err(CE_CONT, "                module number   0x%x\n", 
						mei.moduleNumber);
			}
			s = "AST NMI: uncorrectable ECC error";
			break;
		case INT_CPU_ERROR:
			s = "CPU error";
			break;
		case INT_POWER_FAIL:
			ast_event_code = EBI_EVENT_PWR_FAIL;
			EVENT_BROADCAST(&ast_event_sv, 0);
			cmn_err(CE_WARN, "AST NMI: !!!Power Failure Detected!!!");
			/*
			 * Does *not* panic.
			 */
			return ( 0 );
			/*NOTREACHED*/
			break;
		case INT_BUS_ERR:
			s = "system bus address/parity error";
			break;
		case INT_BUS_TIMEOUT:
			s = "system bus timeout";
			break;
		case INT_SHUTDOWN:
			ast_event_code = EBI_EVENT_SHUTDOWN;
			EVENT_BROADCAST(&ast_event_sv, 0);
			cmn_err(CE_NOTE, "AST NMI: front panel shutdown switch pressed");
			/*
			 * Does *not* panic.
			 */
			return ( 0 );
			/*NOTREACHED*/
			break;
		case INT_ATTENTION:
			ast_event_code = EBI_EVENT_ATTENTION;
			EVENT_BROADCAST(&ast_event_sv, 0);
			cmn_err(CE_NOTE, "AST NMI: front panel attention switch pressed");
			return ( 0 );
			/*NOTREACHED*/
			break;
		default:
			s = "unknown NMI source";
			break;
	}
	/*
	 * For special debugging:
	 *
	 *	db_stacktrace((void (*)())dbprintf, (char *)0, 1);
 	 */

	/*
	 * Nothing to do now but panic.
	 */
	cmn_err(CE_PANIC, "AST NMI: source is %d: %s.", source, s);

	return ( 0 );	/* just for lint */
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/syscall.c	1.6"
#ident "$Header: $"

/* syscall.c  
 *
 * System call metrics.  Processes SAR_SYSCALL_P records.
 *
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "../sa.h"
#include "sar.h"


static struct metp_syscall   *syscall = NULL;
static struct metp_syscall   *old_syscall = NULL;
static struct metp_syscall   *first_syscall = NULL;
static struct syscall_info   temp;

static flag    first_sample = TRUE;


flag
sar_syscall_init(void)
{
	int      i;
	
	if (syscall == NULL) {
		syscall = malloc(machinfo.num_engines * sizeof(*syscall));
		old_syscall = malloc(machinfo.num_engines * sizeof(*old_syscall));
		first_syscall = malloc(machinfo.num_engines * sizeof(*first_syscall));
		
		if (syscall == NULL || old_syscall == NULL || first_syscall == NULL) {
			return(FALSE);
		}
	}
	
	for (i = 0; i < machinfo.num_engines; i++) {
		syscall[i].mps_syscall = 0;
		syscall[i].mps_fork = 0;
		syscall[i].mps_lwpcreate = 0;
		syscall[i].mps_exec = 0;
		syscall[i].mps_read = 0;
		syscall[i].mps_write = 0;
		syscall[i].mps_readch = 0;
		syscall[i].mps_writech = 0;
		
		old_syscall[i] = syscall[i];
	}
	
	first_sample = TRUE;
	return(TRUE);
}


/*ARGSUSED*/

int
sar_syscall_p(FILE *infile, sar_header sarh, flag32 of)
{
	int      i;
	int      p;
	
	memcpy(old_syscall, syscall, 
	       machinfo.num_engines * sizeof(*old_syscall));
	
	for (i = 0; i < sarh.item_count; i++) {
		get_item(&temp, sizeof(temp), sarh.item_size, infile);
		p = temp.id;
		syscall[p] = temp.data;
	}
	
	if (first_sample == TRUE) {
		memcpy(first_syscall, syscall,
		       machinfo.num_engines * sizeof(*first_syscall));
		first_sample = FALSE;
	}
	collected[sarh.item_code] = TRUE;

	return(TRUE);
}


sarout_t
sar_syscall_out(int column, int mode, int devnum)
{
	struct metp_syscall	*start;
	struct metp_syscall	*end;
	time_t			*td;
	sarout_t		answer;

	SET_INTERVAL(mode, syscall, old_syscall, first_syscall);

	switch (column) {
	      case SYSCALL_SCALL:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mps_syscall);
		return(answer);
		break;

	      case SYSCALL_SREAD:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mps_read);
		return(answer);
		break;

	      case SYSCALL_SWRIT:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mps_write);
		return(answer);
		break;

	      case SYSCALL_FORK:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mps_fork);
		return(answer);
		break;

	      case SYSCALL_LWPCREATE:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mps_lwpcreate);
		return(answer);
		break;

	      case SYSCALL_EXEC:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mps_exec);
		return(answer);
		break;

	      case SYSCALL_RCHAR:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mps_readch);
		return(answer);
		break;

	      case SYSCALL_WCHAR:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mps_writech);
		return(answer);
		break;

	      default:
		sarerrno = SARERR_OUTFIELD;
		return(-1);
		break;
	}
}


void
sar_syscall_cleanup(void)
{
	free(syscall);
	free(old_syscall);
	free(first_syscall);

	syscall = NULL;
	old_syscall = NULL;
	first_syscall = NULL;
}


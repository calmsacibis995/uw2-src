/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nwsema.c	1.19"
#ident 	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nwsema.c,v 2.54.2.4 1995/02/13 20:59:22 doshi Exp $"

/*
 *  Netware Unix Client
 *
 *	MODULE: nwsema.c
 *	ABSTRACT: Define interface to a semaphoring operation accessable
 *	in the kernel for sync'ing access to shared resources.
 *
 */ 

#ifdef _KERNEL_HEADERS
#include <mem/kmem.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <util/ksynch.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/nuc_hier.h>
#include <net/nw/ntr.h>
#include <net/nuc/nuc_prototypes.h>
#else /* ndef _KERNEL_HEADERS */
#include <kdrivers.h>
#include <sys/nwctypes.h>
#include <sys/nucerror.h>
#include <sys/nuctool.h>
#include <sys/nuc_hier.h>
#include <sys/nuc_prototypes.h>
#endif /* ndef _KERNEL_HEADERS */

#define NTR_ModMask	NTRM_tool


LKINFO_DECL(semaSVInfo, "NETNUC:NUC:semaSV", 0);
LKINFO_DECL(psuedoSemaLockInfo, "NETNUC:NUC:psuedoSemaLockInfo", 0);
extern lock_t *criticalSectLock;
extern	int nucSemaphoreCount;
extern  nucSema_t semaTable[];	/* defined in Space.c */
static int semaCtr;

/*
 * BEGIN_MANUAL_ENTRY(NWtlInitSemaphore.3k)
 * NAME
 *		NWtlInitSemaphore - Initialize NW Client semaphores
 *
 * SYNOPSIS
 *		module: nwsema/nwsema.c
 *			ccode_t
 *			NWtlInitSemaphore()
 * INPUT
 *			NONE
 *
 * OUTPUT
 *			NONE
 *
 * RETURN VALUES
 *
 *			NONE
 *
 * DESCRIPTION
 *			
 *		Initializes the semaphore structures for general use
 *
 * NOTES
 *
 *		Manifest constant nucSemaphoreCount 
 *		Is specified in nucttune.h parameter NUC_NUM_SEMAPHORES
 *
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 *
 * END_MANUAL_ENTRY
 */

ccode_t
NWtlInitSemaphore(void)
{
	register int i;

	NTR_ENTER(0, 0, 0, 0, 0, 0);

	semaCtr = 0;
	for (i = 0; i < nucSemaphoreCount; i++)
	{
		semaTable[i].number = -1;
		semaTable[i].value = 1;
		semaTable[i].initialValue = 0;
		if ((semaTable[i].semaSV = SV_ALLOC (KM_NOSLEEP)) == NULL) {
				cmn_err(CE_PANIC, "NWtlInitSemaphore: semaSV alloc failed");
				return (NTR_LEAVE (FAILURE));
		}
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM: NWtlInitSemaphore: alloc sv_t * at 0x%x, size = 0x%x",
                semaTable[i].semaSV, sizeof(sv_t), 0 );
#endif
	}

	return (NTR_LEAVE( 0 ));
}

void_t
NWtlDestroySemaSV (void)
{
	register int i;

	NTR_ENTER(0, 0, 0, 0, 0, 0);

	for (i = 0; i < nucSemaphoreCount; i++) {
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: NWtlDestroySemaSV: free sv_t * at 0x%x, size = 0x%x",
                semaTable[i].semaSV, sizeof(sv_t), 0 );
#endif
		SV_DEALLOC (semaTable[i].semaSV);
	}

	NTR_LEAVE( 0 );
}


/*
 * BEGIN_MANUAL_ENTRY(NWtlCreateAndSetSemaphore.3k)
 * NAME
 *		NWtlCreateAndSetSemaphore - 	Create a semaphore and initialize
 *									it for use.
 *
 * SYNOPSIS
 *		module: nwsema/nwsema.c
 *
 *		ccode_t
 *		NWtlCreateAndSetSemaphore( semaPtr, startValue )
 *		int	*semaPtr;
 *		int startValue;
 *
 * INPUT
 *		semaPtr 	- Pointer to an integer variable
 *		startValue	- Beginning value of the semaphore
 *
 * OUTPUT
 *		semaPtr - If successful, will contain new semaphore number
 *
 * RETURN VALUES
 *
 *		-1 - FAILURE
 *		0  - SUCCESS
 *
 * DESCRIPTION
 *
 *		Allocates a semaphore from the pool of free semaphores if
 *		any exist and initializes it for use by setting the semaphore
 *		value to the starting value.  This effectively determines
 *		how many contexts can grab the semaphore before they begin
 *		to sleep.
 *
 * SEE ALSO
 *		NWtlDestroySemaphore(3k), NWtlEnterCriticalSection(3k),
 *		NWtlExitCriticalSection(3k), FindSemaphore_l(3k),
 *
 * NOTES
 *		This most likely will change in SMP envrionments due
 *		to the kernel-dependent operations needed to lock CPU's
 *		from one-another (typically by using a CPSem in a spin lock).
 *
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWtlCreateAndSetSemaphore (int *semaPtr, int startValue )
{
	register int index;
	pl_t		s;

	NTR_ENTER(2, semaPtr, startValue, 0, 0, 0);

	s = LOCK (criticalSectLock, NUCPLHI);

	if ( (index = FindSemaphore_l(-1)) == -1) {
		UNLOCK (criticalSectLock, s );
#ifdef NUC_DEBUG
		cmn_err (CE_WARN, "No free Semaphores.\n");		/* Fix 001 */
#endif
		return( NTR_LEAVE( -1 ) );
	}

	semaTable[index].number = *semaPtr = index;
	semaTable[index].initialValue = startValue;
	semaTable[index].value = startValue;
	semaCtr++;
	UNLOCK (criticalSectLock, s );

	return( NTR_LEAVE( 0 ) );
}

/*
 * BEGIN_MANUAL_ENTRY(NWtlDestroySemaphore.3k)
 * NAME
 *		NWtlDestroySemaphore - Return semaphore back to the pool.
 *
 * SYNOPSIS
 *		module: nwsema/nwsema.c
 *			ccode_t
 *			NWtlDestroySemaphore( semaphore )
 *			int semaphore;
 *
 * INPUT
 *			semaphore - semaphore number to be destroyed
 *
 * OUTPUT
 *			nothing
 *
 * RETURN VALUES
 *
 *			-1 - FAILURE
 *			0  - SUCCESS
 *
 * DESCRIPTION
 *
 *		Returns a previously created semaphore to the pool of
 *		free semaphores
 *
 * SEE ALSO
 *
 *		NWtlCreateAndSetSemaphore(3k), NWtlEnterCriticalSection(3k),
 *		NWtlExitCriticalSection(3k), FindSemaphore_l(3k)
 *
 * NOTES
 *
 *		Semaphore freeing will fail if the semaphore is currently
 *		in use.
 *
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWtlDestroySemaphore (int semaphore )
{
	register int index;
	pl_t		pl;

	NTR_ENTER(1, semaphore, 0, 0, 0, 0);


	/*
	 *	Raise the priority to mask off interrupts
	 */
	pl = LOCK (criticalSectLock, NUCPLHI);

	if ( (index = FindSemaphore_l( semaphore )) == -1 )
	{
		UNLOCK (criticalSectLock, pl );
#ifdef NUC_DEBUG
		cmn_err (CE_PANIC, "Bad Semaphore Number passed! \n");
#endif
		return( NTR_LEAVE( FAILURE ) );
	}
	
	if ( semaTable[index].value != semaTable[index].initialValue )
	{
		/*
		 *	It is currently in use, can't destroy it...
		 */
		UNLOCK (criticalSectLock, pl );
#ifdef NUC_DEBUG
		cmn_err (CE_WARN, "NWtlDestroySema: FAILED due to semaphore in use!\n");
#endif
		return( NTR_LEAVE( FAILURE ) );
	}
	semaTable[index].number = -1;	/* Reset condition */
	semaTable[index].value = 1;
	semaTable[index].initialValue = 0;
	semaCtr--;
	/*
	 *	Restore the current system interrupt level
	 */
	UNLOCK (criticalSectLock, pl );
	return( NTR_LEAVE( 0 ) );
}

/*
 * BEGIN_MANUAL_ENTRY(NWtlPSemaphore.3k)
 * NAME
 *
 *		NWtlPSemaphore - Perform a P operation on a semaphore
 *
 * SYNOPSIS
 *		ccode_t
 *		NWtlPSemaphore( semaphore )
 *		int	semaphore;
 *
 * INPUT
 *		semaphore	-	Semaphore to decrement
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		0	-	SUCCESS
 *		-1	-	FAILURE
 *
 * DESCRIPTION
 *		Decrements the value of the semaphore and causes context to
 *		sleep if value falls below desired threshold.
 *
 * SEE ALSO
 *		NWtlVSemaphore(3k)
 *
 * NOTES
 *		This routine should NEVER be called from an interrupt handler
 *		as the results may be undesirable.
 *
 *		When the semaphore is incremented, all contexts sleeping on
 *		the semaphore will be awoken and allowed to race to perform
 *		the next decrement operation.
 *
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWtlPSemaphore (int semaphore )
{
	register int index;
	pl_t s;

	NTR_ENTER(1, semaphore, 0, 0, 0, 0);


	s = LOCK (criticalSectLock, NUCPLHI);

	if ( (index = FindSemaphore_l( semaphore )) == -1 ) {
		UNLOCK (criticalSectLock, s);
#ifdef NUC_DEBUG
		cmn_err (CE_PANIC, "Bad Semaphore Number passed! to PSemaphore \n");
#endif
		return( NTR_LEAVE( FAILURE ) );
	}

	for (;;) {
		if (semaTable[index].value > 0) {
			semaTable[index].value--;
			break;
		}
		else {
			/*
			 *	didn't get it, try again...
			 *	Sleep this thread until the guy who grabbed
			 *	it is done with it.
			 */
			NTR_PRINTF("PSema sleeping( 0x%x, %d), value = %d\n",  
				&(semaTable[index]), primed,
				semaTable[index].value);
			SV_WAIT (semaTable[index].semaSV,
						primed, criticalSectLock);
			NTR_PRINTF("PSema awake( 0x%x, %d), value = %d\n",  
				&(semaTable[index]), primed,
				semaTable[index].value);
			/*
			 *	Thread is awake now, mask interrupts before
			 *	messing with the semaphore value
			 */
			LOCK (criticalSectLock, NUCPLHI);
		}
	}

	/*
	 *	We have it now...
	 */
	UNLOCK (criticalSectLock, s);

	return( NTR_LEAVE( 0 ) );
}

/*
 * BEGIN_MANUAL_ENTRY(NWtlCPSemaphore.3k)
 * NAME
 *		NWtlCPSemaphore - Test and allocate control of the semaphore
 *						if it is available.
 *
 * SYNOPSIS
 *		module: nwsema/nwsema.c
 *
 *		ccode_t
 *		NWtlCPSemaphore( semaphore )
 *		int semaphore;
 *
 * INPUT
 *		semaphore	- Number of the semaphore to be tested
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		TRUE (-1)	- Successful control of the semaphore
 *		FALSE(0)	- Semaphore decrement dropped below 0
 *
 * DESCRIPTION
 *		Performs a non-blocking decrement operation on a semaphore.
 *		If the semaphore value drops below 0 after the decrement,
 *		the operation fails, else is successful.
 *
 * SEE ALSO
 *		NWtlPSemaphore(3k)
 *
 * NOTES
 *		This routine can, and should, be used by interrupt handlers
 *		to gain control of a semaphore.  
 *
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 *
 * END_MANUAL_ENTRY
 */

#ifdef NEVER_USED
ccode_t
NWtlCPSemaphore (int semaphore )
{
	register int index;
	int			ccode;
	pl_t 		s;

	NTR_ENTER(1, semaphore, 0, 0, 0, 0);


	s = LOCK (criticalSectLock, NUCPLHI);

	if ( (index = FindSemaphore_l( semaphore )) == -1 )
	{
		UNLOCK (criticalSectLock, s);
#ifdef NUC_DEBUG
		cmn_err (CE_PANIC, "Bad Semaphore Number passed! to CPSemaphore \n");
#endif
		return( NTR_LEAVE( FAILURE ) );
	}

	if (semaTable[index].value >  0) 
	{
		--semaTable[index].value;
		ccode = TRUE;
	}
	else
		ccode = FALSE;

	UNLOCK (criticalSectLock, s);
	return( NTR_LEAVE( ccode ) );
}
#endif NEVER_USED

/*
 * BEGIN_MANUAL_ENTRY(NWtlVSemaphore.3k)
 * NAME
 *			NWtlVSemaphore - Perform a V (increment) operation on
 *						   the specified semaphore.
 *
 * SYNOPSIS
 *		module: nwsema/nwsema.c
 *
 *		ccode_t
 *		NWtlVSemaphore( semaphore )
 *		int	semaphore;
 *
 * INPUT
 *		semaphore	- Number of the semaphore to increment
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		0	-	SUCCESS
 *		-1	-	FAILURE
 *
 * DESCRIPTION
 *		Increment the value of this semaphore effectively reliquishing
 *		control of it.
 *
 * SEE ALSO
 *		NWtlPSemaphore(3k)
 *
 * NOTES
 *		Will not increment the value of the semaphore higher than
 *		the original value. 
 *
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		taskRWLock
 *	LOCKS HELD WHEN RETURNED:
 *		taskRWLock
 *
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWtlVSemaphore_l (int semaphore )
{
	register int index;
	pl_t		s;

	NTR_ENTER(1, semaphore, 0, 0, 0, 0);


	s = LOCK (criticalSectLock, NUCPLHI);

	if ( (index = FindSemaphore_l( semaphore )) == -1 )
	{
		UNLOCK (criticalSectLock, s);
#ifdef NUC_DEBUG
		cmn_err (CE_PANIC, "Bad Semaphore Number passed! to VSemaphore \n");
#endif
		return( NTR_LEAVE( FAILURE ) );
	}

	/*
	 *	Increment value, don't attempt to provide boundary checks as
	 *	an offending V would release someone elses resources anyway,
	 *	and in the case of event sync sema's it is guaranteed to 
	 *	exceed the initial value, because the gate starts closed.
	 */
	++(semaTable[index].value);

	UNLOCK (criticalSectLock, s);

	/*
	 *	If after the increment, the value is 0, 
	 *	someone is currently sleeping on it, so we'll wake them up.
	 */

	SV_BROADCAST (semaTable[index].semaSV, 0);

	NTR_PRINTF("VSema waking up( 0x%x), value = %d\n", &(semaTable[index]),
		semaTable[index].value,0);

	return( NTR_LEAVE( 0 ) );
}

/*
 * BEGIN_MANUAL_ENTRY(FindSemaphore_l.3k)
 * NAME
 *			FindSemaphore_l - Private routine to peruse the semaphore
 *							table for semaphore re-use
 *
 * SYNOPSIS
 *			static int
 *
 * INPUT
 *
 * OUTPUT
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *
 * SEE ALSO
 *
 * NOTES
 *
 *
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		criticalSectLock
 *	LOCKS HELD WHEN RETURNED:
 *		criticalSectLock
 *
 * END_MANUAL_ENTRY
 */
int
FindSemaphore_l (int semaphoreNumber )
{
	register int i;

	for (i = 0; i < nucSemaphoreCount; i++)
	{
		if (semaTable[i].number == semaphoreNumber) {
			return( i  );
		}
	}
	return( -1 );
}

/*
 * BEGIN_MANUAL_ENTRY(NWtlResetSemaphore.3k)
 * NAME
 *
 *		NWtlResetSemaphore - Resets a semaphore to its initial value.
 *
 * SYNOPSIS
 *		ccode_t
 *		NWtlResetSemaphore( semaphore )
 *		int	semaphore;
 *
 * INPUT
 *		semaphore	-	Semaphore to reset
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		0	-	SUCCESS
 *		-1	-	FAILURE
 *
 * DESCRIPTION
 *		Sets the specified semaphore to the its value when initially
 *		created.
 *
 * SEE ALSO
 *
 * NOTES
 *		The caller must be certain that the semaphore is not being
 *		waited on by any tasks when calling this function.
 *
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 *
 *		This function is used primarily by tasks using semaphores to
 *		prevent sleep/wakeup sequences from being lost and the 
 *		semaphore needs resetting.
 *
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWtlResetSemaphore (int semaphore )
{
	register int index;
	pl_t s;

	NTR_ENTER(1, semaphore, 0, 0, 0, 0);


	s = LOCK (criticalSectLock, NUCPLHI);

	if ( (index = FindSemaphore_l( semaphore )) == -1 ) {
		UNLOCK (criticalSectLock, s);
#ifdef NUC_DEBUG
		cmn_err (CE_PANIC, "Bad Semaphore Number passed to ResetSemaphore \n");
#endif
		return( NTR_LEAVE( FAILURE ) );
	}

	semaTable[index].value = semaTable[index].initialValue;

	UNLOCK (criticalSectLock, s);

	if (SV_BLKD (semaTable[index].semaSV)) {
		SV_BROADCAST (semaTable[index].semaSV, 0);
	}

	return( NTR_LEAVE( 0 ) );
}

ccode_t
NWtlCreateAndSetPsuedoSema (psuedoSema_t **sema, int startValue )
{
	pl_t	s;

	NTR_ENTER( 1, sema, 0, 0, 0, 0 );

	*sema = kmem_alloc (sizeof (psuedoSema_t), KM_NOSLEEP);

	if (*sema == NULL) {
		return (NTR_LEAVE (FAILURE));
	}

	if (((*sema)->psuedoSemaLock = LOCK_ALLOC (CRITSECTLOCK_HIER, NUCPLHI,
			&psuedoSemaLockInfo, KM_NOSLEEP)) == NULL) {
		kmem_free (*sema, sizeof (psuedoSema_t));
		cmn_err(CE_WARN, "NWtlCreateAndSetPsuedoSema: "
				 "psuedoSemaLock alloc failed");
		return (NTR_LEAVE (FAILURE));
	}

	if (((*sema)->psuedoSemaSV = SV_ALLOC (KM_NOSLEEP)) == NULL) {
			cmn_err(CE_WARN, "NWtlCreateAndSetPsuedoSema: "
					 "psuedoSemaSV alloc failed");
			LOCK_DEALLOC((*sema)->psuedoSemaLock);
			kmem_free (*sema, sizeof (psuedoSema_t));
			return (NTR_LEAVE (FAILURE));
	}

	s = LOCK((*sema)->psuedoSemaLock, NUCPLHI );
	(*sema)->psuedoSemaValue = startValue;
	(*sema)->psuedoSemaWaitCount = 0;
	(*sema)->psuedoSemaDraining = 0;
	UNLOCK ((*sema)->psuedoSemaLock, s);

	return (NTR_LEAVE (SUCCESS));
}

ccode_t
NWtlDestroyPsuedoSema (psuedoSema_t **sema )
{
	pl_t	s;

	NTR_ENTER( 1, sema, 0, 0, 0, 0 );

	s = LOCK((*sema)->psuedoSemaLock, NUCPLHI );
	SV_DEALLOC ((*sema)->psuedoSemaSV);
	UNLOCK ((*sema)->psuedoSemaLock, s);

	LOCK_DEALLOC ((*sema)->psuedoSemaLock);
	kmem_free (*sema, sizeof (psuedoSema_t));
	*sema = NULL;

	return (NTR_LEAVE (0));
}

/*
 * The semantics of the P, V, and Wakeup operations on pseudo-semaphores
 * are taken to be as follows:
 *	P -- consumer of the semaphore
 *	V -- producer of the semaphore
 *	Wakeup --
 *		A general reset of the semaphore, which has the effect
 *		of undoing all current P/V states. In particular, a
 *		wakeup must:
 *			- set all blocked contexts running,
 *			- consume any produced value on the semaphore.
 *
 * V and Wakeup operations may be driven from interrupt state, so it
 * is not possible for  each to wait for the state created by the other
 * to clear. That means, the P operation must adjust according to whether
 * V or Wakeup operations have occurred, so that V and Wakeup operations
 * interfere with each other to the minimum degree possible.
 *
 * To this end, the following is the implementation of these operations.
 * For simplicity, this high level description does not address spin
 * locking, which is contained in the real code. 
 *
 *	There are 3 state variables: 
 *		(1) value, 
 *		(2) waitcnt, 
 *	and 	(3) draining
 *
 * with the semaphore. value indicates the consumable count for the semaphore,
 * waitcnt indicates the number of pending P operations (which can be less
 * than the number of waiters on the synch variable for the semaphore, as
 * processes may wait until a previously issued "reset" completes.
 * 
 * V: 				/ the producer 
 *	++value;
 *	SV_BROADCAST
 *
 * Wakeup:			
 *	value = 0;		/ reset any previous V's 
 *	
 *	if (waitcnt) {
 *		draining = 1;
 *		SV_BROADCAST	
 *	}			/ reset all waiters 
 *	
 * P:
 *	/ 
 *	/ first, wait for any existing waiters to clear if a Wakeup
 *	/ is yet to complete.
 *	/
 *	while (draining && value == 0 )
 *		SV_WAIT
 *
 *	/
 *	/ loop to wait for producers or new resetters
 *	/
 *	while (val == 0) {
 *		++waitcnt;
 *		SV_WAIT
 *		--waitcnt;
 *		/
 *		/ did we get a reset?
 *		/
 *		if (draining) {
 *			/
 *			/ clear draining state if we are the last one to leave
 *			/
 *			if (waitcnt == 0)
 *				draining = 0;
 *			goto done;
 *		}
 *	]
 *	-- val;		/ normal exit. consume 
 * done:
 *		;
 */

void_t
NWtlPPsuedoSema( psuedoSema_t* sema )
{
	pl_t s;

	NTR_ENTER( 1, sema, 0, 0, 0, 0 );

	s = LOCK( sema->psuedoSemaLock, NUCPLHI );

	while (sema->psuedoSemaDraining && 
			(sema->psuedoSemaValue == 0)) {
		NTR_PRINTF( "NWtlPPsuedoSema: Waiting.\n", 0, 0, 0 );
		SV_WAIT( sema->psuedoSemaSV, primed, sema->psuedoSemaLock );
		(void) LOCK( sema->psuedoSemaLock, NUCPLHI );
	}

	while (sema->psuedoSemaValue == 0) {

		++sema->psuedoSemaWaitCount;

		NTR_PRINTF( "NWtlPPsuedoSema: Waiting.\n", 0, 0, 0 );
		SV_WAIT( sema->psuedoSemaSV, primed, sema->psuedoSemaLock );
		(void) LOCK( sema->psuedoSemaLock, NUCPLHI );

		NTR_ASSERT(sema->psuedoSemaWaitCount > 0);

		--sema->psuedoSemaWaitCount;

		if ( sema->psuedoSemaDraining ) {
			if (sema->psuedoSemaWaitCount == 0) {
				sema->psuedoSemaDraining = 0;
				SV_BROADCAST(sema->psuedoSemaSV, 0);
			}
			goto done;
		}
	}
	--sema->psuedoSemaValue;
done:
	UNLOCK(sema->psuedoSemaLock, s);
	NTR_LEAVE( 0 );
	return;
}

void_t
NWtlVPsuedoSema( psuedoSema_t* sema )
{
	pl_t s;

	NTR_ENTER( 1, sema, 0, 0, 0, 0 );

	s = LOCK( sema->psuedoSemaLock, NUCPLHI );
	++sema->psuedoSemaValue;
	SV_BROADCAST( sema->psuedoSemaSV, 0);
	UNLOCK( sema->psuedoSemaLock, s );

	NTR_LEAVE( 0 );
	return;
}

void_t
NWtlWakeupPsuedoSema( psuedoSema_t* sema )
{
	pl_t s;

	NTR_ENTER( 1, sema, 0, 0, 0, 0 );

	s = LOCK( sema->psuedoSemaLock, NUCPLHI );

	sema->psuedoSemaValue = 0;
	
	if ( sema->psuedoSemaWaitCount != 0) {
		NTR_ASSERT (sema->psuedoSemaWaitCount > 0);
		sema->psuedoSemaDraining = 1; 
		SV_BROADCAST ( sema->psuedoSemaSV, 0 );
	}
	UNLOCK( sema->psuedoSemaLock, s);

	NTR_LEAVE( 0 );
	return;
}

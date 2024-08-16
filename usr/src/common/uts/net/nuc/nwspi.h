/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nwspi.h	1.12"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nwspi.h,v 2.51.2.3 1995/01/16 22:53:16 hashem Exp $"

#ifndef _NET_NUC_SPIL_NWSPI_H
#define _NET_NUC_SPIL_NWSPI_H

/*
 *  Netware Unix Client
 *
 *	  MODULE: nwspi.h
 *	ABSTRACT: Header file of structures needed in the SPI layer
 */

#define NUM_SPROTO (int)3


typedef struct {
	int			mode;
	SPI_OPS_T	*spi_ops;
} SPI_SWITCH_T;

/*
 *	Manifest constants for defining name space capabilities
 */
#define NFS_MODE		0x02

/*
 *	spilInternalServiceRegistrationSemaphore is used to serialize access to the 
 *	SPIL auto-registration of services.  It is initialized by function
 *	nwmpInternalCreateService.
 */
extern	int				spilInternalServiceRegistrationSemaphore;

/*
 *	spilInternalServiceRegistrationServerSemaphore is used to prevent lost
 *	wakeups from the task desiring service registration to the process
 *	performing them.  It is initialized by function
 *	nwmpInternalCreateService.
 */
extern	int				spilInternalServiceRegistrationServerSemaphore;

/*
 *	spilInternalServiceRegistrationName is used to contain the name of 
 *	the service to be internally created.  It is populated by any task 
 *	after gaining access to spilServiceRegistrationSemaphore.  This buffer
 *	is then slept upon by the task waiting for the service to be registered and
 *	nwmpInternalCreateService will wake it up after returning from user space.
 */
extern	char			spilInternalServiceRegistrationName[];

/*
 *	spilInternalTaskAuthenticationSemaphore is used to serialize access to the 
 *	SPIL auto-authentication of services.  It is initialized by function
 *	nwmpInternalCreateTask.
 */
extern	int				spilInternalTaskAuthenticationSemaphore;

/*
 *	spilInternalTaskAuthenticationAddress is contains the network address of 
 *	the service to be authenticated.  It is populated by any task 
 *	after gaining access to spilTaskAuthenticationSemaphore.  This buffer
 *	is then slept upon by the task waiting for the service to be authenticated
 *	and nwmpInternalCreateTask will wake it up after returning from user space.
 */
extern	struct	netbuf	spilInternalTaskAuthenticationAddress;
extern	char			spilInternalTaskAuthenticationBuffer[];

/*
 *	spilInternalTaskAuthentication{Uid,Gid} contain the credentials of the
 *	task to be created for the above-named service.  They are also set by 
 *	the task wanting authentication. Also the PID.
 */
extern	uint32			spilInternalTaskAuthenticationUid;
extern	uint32			spilInternalTaskAuthenticationGid;
extern	uint32			spilInternalTaskAuthenticationPid;
extern	int16			spilInternalTaskAuthenticationXautoFlags;

/*
 *	spilInternalTaskAuthenticationQueue is slept upon by function 
 *	nwmpInternalCreateTask until awaken by any task needing task
 *	authentication.  spilInternalTaskAuthenticationName should contain 
 *	the name of the service to be registered and
 *	spilInternalTaskAuthentication{Uid,Gid} should contain the 
 *	credentials before issuing the wakeup.
 */
extern	int				spilInternalTaskAuthenticationQueue;
extern	sv_t			*spilInternalTaskAuthenticationQueueSV;

#endif /* _NET_NUC_SPIL_NWSPI_H */

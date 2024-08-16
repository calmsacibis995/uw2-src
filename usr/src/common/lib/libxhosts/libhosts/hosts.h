/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libhosts:libhosts/hosts.h	1.1"
#if	!defined(HOSTS_H)
#define	HOSTS_H

#if	!defined(__cplusplus)
void
    *hostInit(int debugLevel),
    *hostNew(char *name, char *address, void *data, void (*dataFree)()),
    hostFree(void *host_p);

char
    *hostName(void *handle_p);
#else
extern "C" void *hostInit(int debugLevel);
extern "C" void *hostNew(char *name, char *address, void *data, void (*dataFree)());
extern "C" void hostFree(void *host_p);
extern "C" char *hostName(void *handle_p);
#endif
#endif

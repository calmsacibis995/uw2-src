/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwClnt:headers/nucrequester.h	1.3"
#ifndef _NUCREQUESTER_H_
#define _NUCREQUESTER_H_

typedef struct {
    uint32     ce_connHandle;
    int     ce_fd;
    struct netbuf   ce_serverAddress;
    char        ce_buffer[MAX_ADDRESS_SIZE];
    char        *ce_serverName;
} ce_t;

#define MAX_CONNECTIONS_PER_PROCESS		32
#define AUTH_KEY_LENGTH 				24
#define NCP_HEADER_SIZE                 512 
#define SUCCESS                         0  
#define SESSION_KEY_LENGTH              8 


#endif /* _NUCREQUESTER_H_ */

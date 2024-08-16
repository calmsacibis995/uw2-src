/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwClnt:headers/nucinit.h	1.4"
#ifndef _NUCINIT_H_
#define _NUCINIT_H_

typedef struct {
    nuint32      securityPreference;
    nuint32      checksumPreference;
    nuint8       *preferredServer;
} INIT_REQ_T;

typedef struct {
    nuint8       *preferredTree;
    nuint8       *defaultContext;
    nuint8       *clientNLSPath;
} DS_INIT_REQ_T;


#define WILL_NOT_SIGN           0x00000000
#define WILL_SIGN_IF_REQUIRED   0x00000001
#define PREFERS_TO_SIGN         0x00000002
#define SIGNING_REQUIRED        0x00000003

#define WILL_NOT_CHECKSUM			0x00000000
#define WILL_CHECKSUM_IF_REQUIRED	0x00000001
#define PREFERS_TO_CHECKSUM			0x00000002
#define CHECKSUMS_REQUIRED			0x00000003

#endif /* _NUCINIT_H_ */

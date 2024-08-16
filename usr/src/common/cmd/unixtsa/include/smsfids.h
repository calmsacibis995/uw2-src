/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/smsfids.h	1.1"

#if !defined(_SMSFIDS_H_INCLUDED)
#define _SMSFIDS_H_INCLUDED
    
/*    Miscellaneous defines */
#if defined(DEBUG_CODE)
#define DEBUG_HEADER_STRING_FIELD         NWSM_HEADER_DEBUG_STRING, UINT64_ZERO, HEADER_STRING
#define DEBUG_HEADER_FIELD                HEADER_STRING_LEN, NULL, HEADER_STRING_LEN
#define DEBUG_HEADER_STRING_FIELD_ALT     NWSM_HEADER_DEBUG_STRING, UINT64_ZERO, HEADER_STRING_ALT
#define DEBUG_HEADER_FIELD_ALT            HEADER_STRING_LEN_ALT, NULL, HEADER_STRING_LEN_ALT
#endif

#define NWSM_REV_DATA                    "REV 1.4"
#define NWSM_SYNC_DATA                    0x5AA5
#define NWSM_VARIABLE_SIZE                0x80

/*  Media Mark Defines */
#define NWSM_MARK_TYPE_HARDWARE           1
#define NWSM_MARK_TYPE_SOFTWARE           2
#define NWSM_SMM_FILE_MARK                1
#define NWSM_SMM_SET_MARK                 2

#include <smsfids.inc>

#endif


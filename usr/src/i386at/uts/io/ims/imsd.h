/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/ims/imsd.h	1.1"

/*
** ident @(#) @(#) imsd.h 1.1 1 12/6/93 09:36:49 
**
** sccs_id[] = {"@(#) 1.1 imsd.h "}
*/

/*
***************************************************************************
**
**      INCLUDE FILE NAME:  imsd.h
**
**      PURPOSE:  Intelligent Management Subsystem Daemon
**                Interface Declarations.
**
**      DEPENDENCIES:
**          o NONE
**
**
**      REVISION HISTORY:
**      FPR/CRN    Date    Author     Description
**              05/26/92   K. Conner  Initial Release.
**
**
**      COPYRIGHT:
**
**          (c) Tricord systems, Inc. 1992
**               All rights reserved.
**
**
***************************************************************************
*/

/*
***************************************************************************
**                                Defines
***************************************************************************
*/
/*
**  Type values returned by the IMS driver to the IMS daemon.
*/
#define TYPE_LOG        1        /* Returned data is log message.      */
#define TYPE_READ       2        /* Completed read request from IMS    */

/*
***************************************************************************
**                               Macros
***************************************************************************
*/

/*
***************************************************************************
**                               Typedefs
***************************************************************************
*/
typedef struct read_req
{
    int type;
    char buffer[IMS_MAX_LOG_MSG];
} READ_REQ;


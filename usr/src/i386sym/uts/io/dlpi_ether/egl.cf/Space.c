/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:io/dlpi_ether/egl.cf/Space.c	1.2"
#ident	"$Header: $"


#include <sys/types.h>
#include <config.h>
#include <conf/dlpi_ether/conf_attr.h>

#define EGL_LVL	5

struct egl_conf egl0 = { 0, 0, 0x5800, 0x9c0000, EGL_LVL, EGL__CMAJOR_0 } ;
struct egl_conf egl1 = { 1, 0, 0x5000, 0x940000, EGL_LVL, EGL__CMAJOR_0 } ;

struct egl_conf *egl_conf[] = {
	&egl0, &egl1 } ;

int egl_nconf = 2 ;

unchar egl_bin = 5 ;

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/config_proto.h	1.1"
/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 *
 *    @(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/config_proto.h,v 1.1 1994/02/11 18:23:56 nick Exp $
 */
#ifndef __CONFIG_PROTO_H__
#define __CONFIG_PROTO_H__

#define CONFIG_CONSOLE_DEVICE ConsoleDevice
#define CONFIG_CONSOLE_DEVICE_DEFN extern char ConsoleDevice[]

#define CONFIG_PRT_PATH PRTConfigPath
#define CONFIG_PRT_PATH_DEFN extern char PRTConfigPath[]

#define CONFIG_CONTROL_PATH ControlFilePath
#define CONFIG_CONTROL_PATH_DEFN extern char ControlFilePath[]

#define CONFIG_PATH ConfigFilePath
#define CONFIG_PATH_DEFN extern char ConfigFilePath[]

void InitConfig(void);


#endif


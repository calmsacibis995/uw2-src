/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:netmgt/objects.h	1.5"
#ident	"$Header: /SRCS/esmp/usr/src/nw/head/netmgt/objects.h,v 1.5 1994/09/14 17:47:13 cyang Exp $"
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991, 1992,    *
 *                 1993, 1994  Novell, Inc. All Rights Reserved.            *
 *                                                                          *
 ****************************************************************************
 *      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	    *
 *      The copyright notice above does not evidence any   	            *
 *      actual or intended publication of such source code.                 *
 ****************************************************************************/

/*      @(#)objects.h	1.1 STREAMWare TCP/IP SVR4.2  source        */
/*      @(#)objects.h	6.5 INTERACTIVE SNMP  source        */
/*      SCCS IDENTIFICATION        */
/* objects.h - MIB objects */

/*
 *
 * Contributed by NYSERNet Inc. This work was partially supported by
 * the U.S. Defense Advanced Research Projects Agency and the Rome
 * Air Development Center of the U.S. Air Force Systems Command under
 * contract number F30602-88-C-0016.
 *
 */

/*
 * All contributors disclaim all warranties with regard to this
 * software, including all implied warranties of mechantibility
 * and fitness. In no event shall any contributor be liable for
 * any special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in action of contract, negligence or other tortuous action,
 * arising out of or in connection with, the use or performance
 * of this software.
 */

#ifndef _SNMP_OBJECTS_H
#define _SNMP_OBJECTS_H

/*
 * As used above, "contributor" includes, but not limited to:
 * NYSERNet, Inc.
 * Marshall T. Rose
 */

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct object_syntax {
	char *os_name;		       /* syntax name */
	int os_type;		       /* syntax type */
	IFP os_decode;		       /* PE -> data  */
	IFP os_free;		       /* free data   */
} *OS;
#define NULLOS	((OS) 0)

int readsyntax(), add_syntax();
OS test2syn();

typedef struct object_type {
	char *ot_text;		       /* OBJECT DESCRIPTOR */
	char *ot_id;		       /* OBJECT IDENTIFIER */
	OID ot_name;		       /*   .. */
	OS ot_syntax;		       /* SYNTAX */
	int ot_access;		       /* ACCESS */
#define	OT_NONE		0x00
#define	OT_RDONLY	0x01
#define	OT_WRONLY	0x02
#define	OT_RDWRITE	(OT_RDONLY | OT_WRONLY)

	unsigned long ot_views;	       /* for views */
	int ot_status;		       /* STATUS */
#define	OT_OBSOLETE	0x00
#define	OT_MANDATORY	0x01
#define	OT_OPTIONAL	0x02
#define	OT_DEPRECATED	0x03

	caddr_t ot_info;	       /* object information */
	IFP ot_getfnx;		       /* get/get-next method */
	IFP ot_setfnx;		       /* set method */
#define	type_SNMP_PDUs_commit	(-1)
#define	type_SNMP_PDUs_rollback	(-2)

	caddr_t ot_save;	       /* for set method */
	caddr_t ot_smux;	       /* for SMUX */
	struct object_type *ot_chain;  /* hash-bucket for text2obj */
	struct object_type *ot_sibling;/* linked-list for name2obj */
	struct object_type *ot_children;	/*   .. */

	struct object_type *ot_next;   /* linked-list for get-next */
} object_type, *OT;
#define	NULLOT	((OT) 0)

typedef struct object_instance {
	OID oi_name;		       /* instance OID */
	OT oi_type;		       /* prototype */
} object_instance, *OI;
#define	NULLOI	((OI) 0)

#ifdef __STDC__

char *oid2ode(OID oid);
int readobjects(char *file);
int add_objects(register OT ot);
int add_objects_aux(void);

OT name2obj(OID oid);
OT text2obj(char *text);
OID text2oid(char *text);

OI name2inst(OID oid);
OI next2inst(OID oid);
OI text2inst(char *text);

int o_number(OI oi, struct type_SNMP_VarBind *v, caddr_t number);
int o_longword(OI oi, struct type_SNMP_VarBind *v, int number);
int o_string(OI oi, struct type_SNMP_VarBind *v, char *base, int len);
int o_specific(OI oi, struct type_SNMP_VarBind *v, caddr_t value);
int o_ipaddr(OI oi, struct type_SNMP_VarBind *v, caddr_t value);

#else /*__STDC__*/
char *oid2ode();
int readobjects();
intadd_objects();
int add_objects_aux();
OT name2obj();
OT text2obj();
OID text2oid();
OI name2inst();
OI next2inst();
OI text2inst();

int o_number();
int o_longword();
int o_string();
int o_specific();
int o_ipaddr();
#endif /*__STDC__*/

#define	o_integer(oi,v,value)	o_longword ((oi), (v), (int) (value))

extern int smux_debug;

char *strdup();

#ifdef __cplusplus
}
#endif

#endif /* _SNMP_OBJECTS_H */

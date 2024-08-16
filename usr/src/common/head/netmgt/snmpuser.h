/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:netmgt/snmpuser.h	1.4"
#ident	"$Header: /SRCS/esmp/usr/src/nw/head/netmgt/snmpuser.h,v 1.4 1994/06/28 21:49:26 cyang Exp $"
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

/*      @(#)snmpuser.h	1.1 STREAMWare TCP/IP SVR4.2  source        */
/*      SCCS IDENTIFICATION        */
/*      @(#)snmpuser.h	6.5 INTERACTIVE SNMP  source        */
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1989, 1990 INTERACTIVE Systems Corporation
 * All rights reserved.
 */

#ifndef _SNMP_SNMPUSER_H
#define _SNMP_SNMPUSER_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

/* oid_lib */
#define NAME_OID_HASH_SIZE 2000

typedef struct _OID_TREE_ELE 
  {
    unsigned long sid_value;
    char *oid_name;
    char *oid_number_str;
    struct _OID_TREE_ELE *next;
    struct _OID_TREE_ELE *first_descendent;
  } OID_TREE_ELE;

typedef struct _NAME_OID_HASH_ELE 
  {
    char *oid_name;
    char *oid_number_str;
    unsigned long hash_value;   /* all hash values in a list should be equal */
    struct _NAME_OID_HASH_ELE *next; /* pointer to next element in hash list */
  } NAME_OID_HASH_ELE;



struct MIB_OID 
  {
    char *name;
    char *number;
  };

/*
 * The following externs are needed to let users load/modify the mib
 */

#ifdef __STDC__
OID  make_obj_id_from_dot(unsigned char *text_str);
long parse_sub_id_hex(unsigned char **temp_ptr);
long parse_sub_id_octal(unsigned char **temp_ptr);
long parse_sub_id_decimal(unsigned char **temp_ptr);
short make_dot_from_obj_id(OID  oid_ptr, char *buffer);
short get_str_from_sub_ids(char *string_buffer, unsigned int *sid,
			   short sid_counter);
void init_new_oid_routines(struct MIB_OID mib_oid_table[]);
void init_oid_tree(struct MIB_OID mib_oid_table[]);
OID_TREE_ELE *add_ele_to_tree(
	    OID_TREE_ELE * oid_tree_ele_ptr, int index, char *oid_number_str);
OID_TREE_ELE *add_ele_to_tree_level(OID_TREE_ELE * oid_tree_ele_ptr,
				    int index, long sid, char *oid_number_str);
void init_name_oid_hash_array(struct MIB_OID mib_oid_table[]);
NAME_OID_HASH_ELE *add_name_oid_to_hash(NAME_OID_HASH_ELE * hash_ptr,
					int hash, int index);
OID_TREE_ELE *oid_tree_dive(OID_TREE_ELE * oid_tree_ptr, unsigned int *sid,
			    short sid_counter);
int new_mib_from_file(char *filename);
int parse_mib_line(char *in_buffer, char *name_buf, char *num_buf);
int merge_mib_from_file(char *filename);
#else
OID_TREE_ELE *add_ele_to_tree();
OID_TREE_ELE *add_ele_to_tree_level();
NAME_OID_HASH_ELE *add_name_oid_to_hash();
OID_TREE_ELE *oid_tree_dive();
OID  make_obj_id_from_dot();
short parse_sub_id_alpha();
long parse_sub_id_hex();
long parse_sub_id_octal();
long parse_sub_id_decimal();
short make_dot_from_obj_id();
short get_str_from_sub_ids();
#endif

/* print_lib */
#ifdef __STDC__
void print_packet_out(unsigned char *ptr, long len);
void print_octet_string_out(OctetString * oct_ptr, short wrap);
short print_varbind_list(VarBindList * vbl_ptr);
short print_ascii(OctetString * os_ptr);
#else
void print_packet_out();
void print_octet_string_out();
short print_varbind_list();
short print_ascii();
#endif

#ifdef __cplusplus
}
#endif

#endif /* _SNMP_SNMPUSER_H */

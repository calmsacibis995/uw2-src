/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsnmp:oid_lib.c	1.4"
#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/libsnmp/oid_lib.c,v 1.4 1994/08/02 23:36:47 cyang Exp $"
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

#ifndef lint
static char TCPID[] = "@(#)oid_lib.c	1.1 STREAMWare TCP/IP SVR4.2 source";
#endif /* lint */
/*      SCCS IDENTIFICATION        */
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1989, 1990 INTERACTIVE Systems Corporation
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ifndef lint
static char SNMPID[] = "@(#)oid_lib.c	6.2 INTERACTIVE SNMP source";
#endif /* lint */

/*
 *
 * Copyright (C) 1991 by SNMP Research, Incorporated.
 *
 * This software is furnished under a license and may be used and copied
 * only in accordance with the terms of such license and with the
 * inclusion of the above copyright notice. This software or any other
 * copies thereof may not be provided or otherwise made available to any
 * other person. No title to and ownership of the software is hereby
 * transferred.
 *
 * The information in this software is subject to change without notice
 * and should not be construed as a commitment by SNMP Research, Incorporated.
 *
 * Restricted Rights Legend:
 *  Use, duplication, or diclosure by the Government is subject to restrictions
 *  as set forth in subparagraph (c)(1)(ii) of the Rights in Technical Data
 *  and Computer Software clause at DFARS 52.227-7013 and in similar clauses
 *  in the FAR and NASA FAR Supplement.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "snmp.h" 
#include "snmpuser.h"
#include "snmp-mib.h"


OID_TREE_ELE *oid_tree_ele_root;
static char *unassigned_text = "UNASSIGNED";

NAME_OID_HASH_ELE *name_oid_hash_array[NAME_OID_HASH_SIZE];

struct MIB_OID *mib_oid_table = orig_mib_oid_table;

/*
 * Routines to initialize the new data structures that will speed name
 * to OID and OID to name translations at the expense of memory and
 * initialization time.
 */
int global_init_new_oid_routines = 0;

void
init_new_oid_routines(struct MIB_OID * mib_oid_table)
{
	init_oid_tree(mib_oid_table);
	init_name_oid_hash_array(mib_oid_table);

	global_init_new_oid_routines = 1;
}

void init_oid_tree(struct MIB_OID * mib_oid_table)
{
  int i;
  /*
   * should probably confirm that oid_tree_ele_root is still null
   * or do memory reclamation
   */
  oid_tree_ele_root = NULL;
  
  for (i = 0; mib_oid_table[i].name != NULL; i++) 
    {
      oid_tree_ele_root = add_ele_to_tree(oid_tree_ele_root, i, 
					  mib_oid_table[i].number);
    }
}

OID_TREE_ELE *
add_ele_to_tree(OID_TREE_ELE *oid_tree_ele_ptr,
		int index,
		char *oid_number_str)
{
  long sid;
  OID_TREE_ELE *temp_ele_ptr;

  /* let's check that we haven't gone too far */
  if (oid_number_str == '\0') 
    {
      LIB_ERROR2("add_ele_to_tree:  bad entry in mib_oid_table.  Severe Error!\nName: %s \t Number: %s\n", 
		 mib_oid_table[index].name, 
		 mib_oid_table[index].number);
      return (NULL);
    }
  /* Otherwise, let's find out the SID we're dealing with */
  if (*oid_number_str == '0') 
    {
      if ((oid_number_str[1] == 'x') || (oid_number_str[1] == 'X')) 
	{
	  if ((sid = parse_sub_id_hex((unsigned char **)&oid_number_str)) < 0)
	    {
	      LIB_ERROR("add_ele_to_tree, hex:\n");
	      return (NULL);
	    }
	} 
      else 
	{
	  if ((sid=parse_sub_id_octal((unsigned char **)&oid_number_str)) < 0)
	    {
	      LIB_ERROR("add_ele_to_tree, octal:\n");
	      return (NULL);
	    }
	}
  } 
  else 
    if (isdigit(*oid_number_str)) 
      {
	if ((sid=parse_sub_id_decimal((unsigned char **)&oid_number_str)) < 0)
	  {
	    LIB_ERROR("add_ele_to_tree, decimal:\n");
	    return (NULL);
	  }
      } 
    else 
      {
	LIB_ERROR2("add_ele_to_tree, bad character: %d, %s\n",
		   *oid_number_str, oid_number_str);
	return (NULL);
      }

  /*
   * OK, so now we know the SID for the level we're on and we've
   * advanced oid_number_str past this SID in the string.
   */

  /*
   * If we're the first at this level, create the first entry for the
   * level, even if this isn't the definition for the level
   */
  if (oid_tree_ele_ptr == NULL) 
    {/* create an entry */
      if ((oid_tree_ele_ptr = (OID_TREE_ELE *)
	   malloc(sizeof (OID_TREE_ELE))) == NULL) 
	{
	  LIB_ERROR("add_ele_to_tree:  Memory allocation error\n");
	  return (NULL);
	}
      /* initialize values */
      oid_tree_ele_ptr->sid_value = sid;
      oid_tree_ele_ptr->next = NULL;
      oid_tree_ele_ptr->first_descendent = NULL;

      /*
       * Check to see if we're done descending.  If not, don't give this
       * element this name since it's the name of an element farther in tree.
       */
      if (*oid_number_str != '\0') 
	{
	  oid_number_str++;	/* advanced past the "." there */

	  /* give it a blank text string */
	  oid_tree_ele_ptr->oid_name = unassigned_text;	
	  
	  /* now go down to next level */
	  oid_tree_ele_ptr->oid_number_str = NULL;
	  oid_tree_ele_ptr->first_descendent =
	    add_ele_to_tree(oid_tree_ele_ptr->first_descendent, index, 
			    oid_number_str);
	} 
      else 
	{
	  oid_tree_ele_ptr->oid_name = mib_oid_table[index].name;
	  oid_tree_ele_ptr->oid_number_str = mib_oid_table[index].number;
	}

      return (oid_tree_ele_ptr);
    }			       /* end of is first entry on this level */
	
  /*
   * OK, this isn't the first entry at this level.  We've got to find where in
   * this level we go.
   */
  
  /*
   * need to have a "root" pointer for this level's linked list.  This is
   * because the first element may change if the added element is < current
   * first element
   */

  /* could pass directly - here for readability */
  temp_ele_ptr = oid_tree_ele_ptr; 

  temp_ele_ptr = 
    add_ele_to_tree_level(temp_ele_ptr, index, sid, oid_number_str);

  return (temp_ele_ptr);
}				       /* end of add_ele_to_tree() */

OID_TREE_ELE *
add_ele_to_tree_level(OID_TREE_ELE *oid_tree_ele_ptr,
		      int index,
		      long sid,
		      char *oid_number_str)
{
  OID_TREE_ELE *temp_oid_tree_ele_ptr;
  
  /* at end of string?? */
  if (oid_tree_ele_ptr == NULL) 
    {
      if ((temp_oid_tree_ele_ptr = (OID_TREE_ELE *)
	   malloc(sizeof (OID_TREE_ELE))) == NULL) 
	{
	  LIB_ERROR("add_ele_to_tree_level:  Memory allocation error");
	  return (NULL);
	}
      /* initialize values */
      temp_oid_tree_ele_ptr->sid_value = sid;
      temp_oid_tree_ele_ptr->next = NULL;
      temp_oid_tree_ele_ptr->first_descendent = NULL;
      
      /*
       * Check to see if we're done descending.  If not, don't give this
       * element this name since it's the name of an element farther in tree.
       */
      if (*oid_number_str != '\0') 
	{
	  oid_number_str++;	/* advanced path the "." there */

	  /* give it a blank text string */
	  temp_oid_tree_ele_ptr->oid_name = unassigned_text;	
	  temp_oid_tree_ele_ptr->oid_number_str = NULL;
	  /* now go down to next level */
	  temp_oid_tree_ele_ptr->first_descendent =
	    add_ele_to_tree(temp_oid_tree_ele_ptr->first_descendent, index, 
			    oid_number_str);
	} 
      else 
	{
	  temp_oid_tree_ele_ptr->oid_name = mib_oid_table[index].name;
	  temp_oid_tree_ele_ptr->oid_number_str = mib_oid_table[index].number;
	}

      return (temp_oid_tree_ele_ptr);
    }
  /*
   * Have we found where to insert our SID in the list?
   */
  if (oid_tree_ele_ptr->sid_value > sid) 
    {
      /* create new entry */
      if ((temp_oid_tree_ele_ptr = (OID_TREE_ELE *)
	   malloc(sizeof (OID_TREE_ELE))) == NULL) 
	{
	  LIB_ERROR("add_ele_to_tree_level:  Memory allocation error");
	  return (NULL);
	}
      /* initialize values */
      temp_oid_tree_ele_ptr->sid_value = sid;
      temp_oid_tree_ele_ptr->next = oid_tree_ele_ptr;
      temp_oid_tree_ele_ptr->first_descendent = NULL;

      /*
       * Check to see if we're done descending.  If not, don't give this
       * element this name since it's the name of an element farther in tree.
       */
      if (*oid_number_str != '\0') 
	{
	  oid_number_str++;	/* advance past the "." there */

	  /* give it a blank text string */
	  temp_oid_tree_ele_ptr->oid_name = unassigned_text;
	  temp_oid_tree_ele_ptr->oid_number_str = NULL;
	  /* now go down to next level */
	  temp_oid_tree_ele_ptr->first_descendent =
	    add_ele_to_tree(temp_oid_tree_ele_ptr->first_descendent, index, oid_number_str);
	}
      else 
	{
	  temp_oid_tree_ele_ptr->oid_name = mib_oid_table[index].name;
	  temp_oid_tree_ele_ptr->oid_number_str = mib_oid_table[index].number;
	}
      return (temp_oid_tree_ele_ptr);
    }
  /*
   * is this an existing entry that needs entry updated?
   */
  if (oid_tree_ele_ptr->sid_value == sid) 
    {
      if (*oid_number_str != '\0') 
	{
	  oid_number_str++;	/* advance past the "." there */
	  /* DON'T change name to unassigned, just leave it ALONE!!! */
	  /* now go down to next level */
	  oid_tree_ele_ptr->first_descendent =
	    add_ele_to_tree(oid_tree_ele_ptr->first_descendent, index, 
			    oid_number_str);
	} 
      else
	oid_tree_ele_ptr->oid_name = mib_oid_table[index].name;

      return (oid_tree_ele_ptr);
    }
  /*
   * otherwise, it must be a less than entry, so just hop to next element
   */
  oid_tree_ele_ptr->next =
    add_ele_to_tree_level(oid_tree_ele_ptr->next, index, sid, oid_number_str);

  return (oid_tree_ele_ptr);
}

void
init_name_oid_hash_array(struct MIB_OID * mib_oid_table)
{
  int i, j, hash;
  
  for (i = 0; i < NAME_OID_HASH_SIZE; i++)
    name_oid_hash_array[i] = NULL;

  /* for each line in the mib translation table */
  for (i = 0; mib_oid_table[i].name != NULL; i++) 
    {
      
      /* calculate hash */
      hash = 0;
      for (j = (strlen(mib_oid_table[i].name) - 1); j >= 0; j--)
	hash = (hash + mib_oid_table[i].name[j]) % NAME_OID_HASH_SIZE;

      /* now add entry to hash array */
      name_oid_hash_array[hash] =
	add_name_oid_to_hash(name_oid_hash_array[hash], hash, i);
    }
}

NAME_OID_HASH_ELE *
add_name_oid_to_hash(NAME_OID_HASH_ELE *hash_ptr,
		     int hash,
		     int index)
{
  NAME_OID_HASH_ELE *temp_hash_ptr;

  /*
   * at end of list? Then allocate new element
   */
  if (hash_ptr == NULL) 
    {
      if ((temp_hash_ptr = 
	   (NAME_OID_HASH_ELE *) malloc(sizeof (NAME_OID_HASH_ELE))) == NULL) 
	{
	  LIB_ERROR("add_name_oid_to_hash: Memory allocation error\n");
	  return (NULL);
	}
      /* initialize the values */
      temp_hash_ptr->oid_name = mib_oid_table[index].name;
      temp_hash_ptr->oid_number_str = mib_oid_table[index].number;
      temp_hash_ptr->hash_value = hash;
      temp_hash_ptr->next = NULL;
      
      return (temp_hash_ptr);
    }

  /*
   * if this hash name is greater than test name, then insert just before
   * this entry.
   */
  if (strcmp(hash_ptr->oid_name, mib_oid_table[index].name) > 0) 
    {
      if ((temp_hash_ptr = (NAME_OID_HASH_ELE *)
	   malloc(sizeof (NAME_OID_HASH_ELE))) == NULL) 
	{
	  LIB_ERROR("add_name_oid_to_hash: Memory allocation error\n");
	  return (NULL);
	}
      /* initialize the values */
      temp_hash_ptr->oid_name = mib_oid_table[index].name;
      temp_hash_ptr->oid_number_str = mib_oid_table[index].number;
      temp_hash_ptr->hash_value = hash;
      temp_hash_ptr->next = hash_ptr;
      
      return (temp_hash_ptr);
    }

  /*
   * must be equal to or less than, so go to next element on list.
   */
  hash_ptr->next = add_name_oid_to_hash(hash_ptr->next, hash, index);

  return (hash_ptr);
}				       /* end of add_name_oid_to_hash() */


/*
 * Routines to create an object identifer in an OID from
 * dot notation input.
 */

/*
make_obj_id_from_dot

     This routine is called to create a library form object iden-
     tifier  from  an character string.  The string input is usu-
     ally  in  the  format  "integer.integer.integer...."  (i.e.,
     "1.3.6.1.2.1.1.1.0"),  but  can  be  shortened  by using the
     names as they appear in RFC 1067 (i.e.,  "sysDescr.0").   It
     returns  a  pointer to a malloc'ed data structure containing
     the internal library representation for an  object  identif-
     ier.    This  identifier  can  then  be  used  in  calls  to
     make_varbind() and make_pdu() (in the case of traps).   This
     malloc'ed  data  structure  will  be  free'ed  by  calls  to
     free_pdu() after the pointer has been used.
*/

OID
make_obj_id_from_dot(unsigned char *text_str)
{
  OID oid_ptr;
  unsigned char *temp_ptr, *dot_ptr;
  short i, cc;
  unsigned char temp_buffer[256];
  short dot_count;
  NAME_OID_HASH_ELE *hash_ptr;
  int hash;

  /* see if there is an alpha descriptor at begining */
  if (isalpha(*text_str)) 
    {      /* if so, do a substitution */
      dot_ptr = (unsigned char *)strchr((char *)text_str, '.');

      /* if no dot, point to end of string */
      if (dot_ptr == NULL)
	dot_ptr = text_str + strlen((char *)text_str);
      temp_ptr = text_str;

/*      for (i = 0; ((temp_ptr < dot_ptr) && (i < 256)); i++)
	temp_buffer[i] = *temp_ptr++;
      temp_buffer[i] = '\0';

      for (i = 0; mib_oid_table[i].name != NULL; i++) 
	{
	  if ((strcmp(mib_oid_table[i].name, (char *)temp_buffer) == 0) &&
	      (strlen((char *)temp_buffer) == strlen(mib_oid_table[i].name))) 
	    {
	      strcpy((char *)temp_buffer, mib_oid_table[i].number);
	      break;
	    }
	}
      if (mib_oid_table[i].name == NULL) 
	{
	  LIB_ERROR1("Make_obj_id_from_dot, table lookup failed: %s\n",
		     temp_buffer);
	  return (NULL);
		}*/
      hash = 0;
      for (i = 0; ((temp_ptr < dot_ptr) && (i < 256)); i++) 
	{
	  temp_buffer[i] = *temp_ptr++;
	  hash = (hash + (int) temp_buffer[i]) % NAME_OID_HASH_SIZE;
	}
      temp_buffer[i] = '\0';

      /*
       * temp_buffer now as the ascii name to look up.  Check out the
       * hash table for a matching entry
       */

      if (global_init_new_oid_routines == 0)
	init_new_oid_routines(mib_oid_table);

      for (hash_ptr = name_oid_hash_array[hash];
	   hash_ptr != NULL; hash_ptr = hash_ptr->next) 
	{
	  cc = strcmp(hash_ptr->oid_name, (char *)temp_buffer);

	  /* if > 0, then it's not in this list */
	  if (cc > 0) 
	    {
	      LIB_ERROR1("Make_obj_id_from_dot, hash table lookup failed: %s\n",
			 temp_buffer);
	      return (NULL);
	    }
	  /* if it's ==, then this is it */
	  if (cc == 0)
	    break;
	  /* otherwise, <, so loop for next test */
	}		       /* end of for loop */

      if (hash_ptr == NULL) 
	{
	  LIB_ERROR1("Make_obj_id_from_dot: name not found: %s\n", 
		     temp_buffer);
	  return (NULL);
	}
      
      /*
       * Blow away old contents of temp_buffer (we're done with them
       * anyways) and replace it with the dotted-decimal equivs.
       */
      strcpy((char *)temp_buffer, hash_ptr->oid_number_str);

      /* now concatenate the non-alpha part to the begining */
      strcat((char *)temp_buffer, (char *)dot_ptr);
    } 
  else 
    {		       /* is not alpha, so just copy into temp_buffer */
      strcpy((char *)temp_buffer, (char *)text_str);
    }

  /* Now we've got something with numbers instead of an alpha header */

  /* count the dots.  num +1 is the number of SID's */
  dot_count = 0;
  for (i = 0; temp_buffer[i] != '\0'; i++) 
    {
      if (temp_buffer[i] == '.')
	dot_count++;
    }
  if ((oid_ptr = (OID) malloc(sizeof (OIDentifier))) == NULL) 
    {
      LIB_PERROR("make_obj_id_from_dot, oid_ptr");
      return (NULL);
    }
  oid_ptr->oid_ptr = NULL;

  if ((oid_ptr->oid_ptr = 
       (unsigned int *)malloc((dot_count + 1) * sizeof (unsigned int))) 
      == NULL) 
    {
      LIB_PERROR("make_obj_id_from_dot, oid_ptr->oid_ptr");
      free_oid(oid_ptr);
      NULLIT(oid_ptr);
      return (NULL);
    }
  oid_ptr->length = dot_count + 1;

  /* now we convert number.number.... strings */
  temp_ptr = temp_buffer;
  for (i = 0; i < dot_count + 1; i++) 
    {
      if (*temp_ptr == '0') 
	{
	  if ((temp_ptr[1] == 'x') || (temp_ptr[1] == 'X')) 
	    {
	      if ((oid_ptr->oid_ptr[i] = parse_sub_id_hex(&temp_ptr)) < 0) 
		{
		  LIB_ERROR("make_obj_id_from_dot, hex:\n");
		  free_oid(oid_ptr);
		  NULLIT(oid_ptr);
		  return (NULL);
		}
	    } 
	  else 
	    {
	      if ((oid_ptr->oid_ptr[i] = parse_sub_id_octal(&temp_ptr)) < 0) 
		{
		  LIB_ERROR("make_obj_id_from_dot, octal:\n");
		  free_oid(oid_ptr);
		  NULLIT(oid_ptr);
		  return (NULL);
		}
	    }

	} 
      else 
	if (isdigit(*temp_ptr)) 
	  {
	    if ((oid_ptr->oid_ptr[i] = parse_sub_id_decimal(&temp_ptr)) < 0) 
	      {
		LIB_ERROR("make_obj_id_from_dot, decimal:\n");
		free_oid(oid_ptr);
		NULLIT(oid_ptr);
		return (NULL);
	      }
	  }
	else 
	  {
	    LIB_ERROR2("make_obj_id_from_dot, bad character: %d, %s\n",
		       *temp_ptr, temp_ptr);
	    free_oid(oid_ptr);
	    NULLIT(oid_ptr);
	    return (NULL);
	  }
      if (*temp_ptr == '.')
	temp_ptr++;    /* to skip over dot */
      else 
	if (*temp_ptr != '\0') 
	  {
	    LIB_ERROR2("make_obj_id_from_dot, expected dot: %d %s",
		       *temp_ptr, temp_ptr);
	    free_oid(oid_ptr);
	    NULLIT(oid_ptr);
	    return (NULL);
	  }
    }			       /* end of for loop */

  if (oid_ptr->oid_ptr[0] >= 4) {/* we have a bogus OID */
    LIB_ERROR1("make_obj_id_from_dot, illegal OID value %d\n",
	       oid_ptr->oid_ptr[0]);
    free_oid(oid_ptr);
    NULLIT(oid_ptr);
    return (NULL);
  }
  return (oid_ptr);
}

long
parse_sub_id_hex(unsigned char **temp_ptr)
{
  short i, cc;
  unsigned long value;
  unsigned long temp_value;

  (*temp_ptr)++;		       /* skip the '0' */
  (*temp_ptr)++;		       /* skip the 'x' or 'X' */
  
  value = 0;

  for (i = 0; ((i < 8) && (**temp_ptr != '.') && (**temp_ptr != '\0')); i++) 
    {
      if (!isxdigit(**temp_ptr)) 
	{
	  LIB_ERROR1("parse_sub_id_hex, bad digit: %s\n", *temp_ptr);
	  return (long)(-1);
	}
      if ((cc = sscanf((char *)(*temp_ptr), "%1x", &temp_value)) != 1) 
	{
	  LIB_ERROR("parse_sub_id_hex, serious error!\n");
	  return (long)(-1);
	}
      /* gotta mask cause bleeding MSC screws up longs occasionally */
      value = (value << 4) + (0x0f & (unsigned long)temp_value);
      (*temp_ptr)++;
    }			       /* end of for loop */

  return (value);
}

long
parse_sub_id_octal(unsigned char **temp_ptr)
{
  short i, cc;
  unsigned long value;
  unsigned long temp_value;

  value = 0;

  for (i = 0; ((i < 11) && (**temp_ptr != '.') && (**temp_ptr != '\0')); i++) 
    {
      if ((!isdigit(**temp_ptr)) || (**temp_ptr == '8') ||
	  (**temp_ptr == '9')) 
	{
	  LIB_ERROR1("parse_sub_id_octal, bad digit: %s\n", *temp_ptr);
	  return (long)(-1);
	}
      if ((cc = sscanf((char *)(*temp_ptr), "%1o", &temp_value)) != 1) 
	{
	  LIB_ERROR("parse_sub_id_octal, serious error!\n");
	  return (long)(-1);
	}
      /* gotta mask cause bleeding MSC screws up longs occasionally */
      value = (value << 3) + (0x07 & (unsigned long)temp_value);
      (*temp_ptr)++;
    }			       /* end of for loop */

  return (value);
}


long
parse_sub_id_decimal(unsigned char **temp_ptr)
{
  short i, cc;
  unsigned long value;
  int temp_value;

  value = 0;
  for (i = 0; ((i < 11) && (**temp_ptr != '.') && (**temp_ptr != '\0')); i++) 
    {
      if (!isdigit(**temp_ptr)) 
	{
	  LIB_ERROR1("parse_sub_id_decimal, bad digit: %s\n", *temp_ptr);
	  return (long)(-1);
	}
      if ((cc = sscanf((char *)(*temp_ptr), "%1d", &temp_value)) != 1) 
	{
	  LIB_ERROR("parse_sub_id_decimal, serious error!\n");
	  return (long)(-1);
	}
      /* gotta mask cause bleeding MSC screws up longs occasionally */
      value = (value * (unsigned long)10) + (0x0f & (unsigned long)temp_value);
      (*temp_ptr)++;
    }			       /* end of for loop */
  
  return (value);
}

/*
 * Routines to build up a "dot notation" character string from an
 * Object Identifier
 */

/*
make_dot_from_obj_id

     This routine is  called  to  convert  an  Object  Identifier
     library construct into a dot notation character string, usu-
     ally for us in a human interface.  The  dot-notation  output
     is the usual form (1.2.3.4.1.2.1.1) with the a MIB name sub-
     situted for the most possible sub-identifiers starting  from
     the  left  (1.3.6.1.2.1.1.1.0  becomes sysDescr.0).  The MIB
     names included in the library are found in the mib_oid_table
     in snmp-mib.h.  This include file is used in the compilation
     of oid_lib.c, part of the libsnmpuser.a library and need not
     be included in applications.
*/

short
make_dot_from_obj_id(OID oid_ptr,
		     char *buffer)
{
  short cc;

  buffer[0] = '\0';
  if ((oid_ptr == NULL) || (oid_ptr->length == 0) ||
      (oid_ptr->oid_ptr == NULL)) 
    {
      LIB_ERROR("make_dot_from_obj_id, bad (NULL) OID\n");
      return (-1);
    }
  if (global_init_new_oid_routines == 0)
    init_new_oid_routines(mib_oid_table);


  if ((cc = get_str_from_sub_ids(buffer, oid_ptr->oid_ptr,
				 (short)oid_ptr->length)) == -1) 
    {
      LIB_ERROR("make_dot_from_obj_id, get_str_from_sub_ids\n");
      return (-1);
    }
  return (0);
}

short
get_str_from_sub_ids(char *string_buffer, unsigned int *sid, short sid_counter)
{
  short cc, i;
  short best_i, best_len, test_len;
  char temp_buffer[20];
  int dot_count;
  OID_TREE_ELE *oid_tree_ele_ptr;

  /* build test string ... put dots between each entry but not after last */
  for (i = 0; i < sid_counter; i++) 
    {
      sprintf(temp_buffer, "%d", sid[i]);
      if ((i + 1) != sid_counter) 
	{
	  strcat((char *)string_buffer, (char *)temp_buffer);
	  strcat((char *)string_buffer, ".");
      } 
      else
	strcat((char *)string_buffer, (char *)temp_buffer);
    }

  /*
   * Now, let's try to convert the dotted decimal header of the
   * string in temp_buffer into a more reasonable ASCII string.
   */

  oid_tree_ele_ptr = oid_tree_dive(oid_tree_ele_root, sid, sid_counter);

  /*
   * Did we get something?  If so, then replace dot-digit string
   * with the name.
   */
  if ((oid_tree_ele_ptr != NULL) && 
      (strcmp(oid_tree_ele_ptr->oid_name, (char *)unassigned_text) != 0)) 
    {
      
      /* blow away our old string */
      strcpy((char *)string_buffer, oid_tree_ele_ptr->oid_name);

      /* count the dots.  num +1 is the number of SID's */
      dot_count = 0;
      for (i = 0; oid_tree_ele_ptr->oid_number_str[i] != '\0'; i++) 
	{
	  if (oid_tree_ele_ptr->oid_number_str[i] == '.')
	    dot_count++;
	}

      /* now flesh out from where the name leaves off */
      
      for (i = dot_count + 1; i < sid_counter; i++) 
	{
	  sprintf(temp_buffer, ".%d", sid[i]);
	  strcat((char *)string_buffer, (char *)temp_buffer);
	}
    }
  /* else, just return the dot string we already filled in */
  return (1);
}

OID_TREE_ELE *
oid_tree_dive(OID_TREE_ELE *oid_tree_ptr, unsigned int *sid, short sid_counter)
{
	OID_TREE_ELE *temp_ptr, *temp2_ptr;

	/* check for search termination cases */
	if (oid_tree_ptr == NULL)
		return (NULL);

	if (sid_counter == 0)
		return (NULL);

	if (sid_counter < 0) {
		LIB_ERROR("oid_tree_dive: sid_counter too low\n");
		return (NULL);
	}
	/*
   * OK, the quick ones are out of the way.  Now, check to see if
   * we've hit a level with no value corrisponding to the sid[0] value
   */

	for (temp_ptr = oid_tree_ptr; temp_ptr != NULL; temp_ptr = temp_ptr->next)
		if (temp_ptr->sid_value == sid[0])
			break;

	/*
   * if none on this level.  we've hit the end of the road at previous level
   */
	if (temp_ptr == NULL)
		return (NULL);

	/*
   * OK, we've got a match at this level, let's try down the next level
   *
   * If the next level down returns null, it found Nada, so return our
   * level's tree element (temp_ptr).  If something is returned, then it's
   * a more qualified answer than this one and we return it up the stack
   */
	if ((temp2_ptr = oid_tree_dive(temp_ptr->first_descendent, &sid[1], --sid_counter)) == NULL)
		return (temp_ptr);

	return (temp2_ptr);
}				       /* end of oid_tree_dive */

new_mib_from_file(filename)
	char *filename;
{
	int mib_entry_count, new_mib_entry_count;
	char in_buffer[1024];
	char name_buf[255], num_buf[255];
	FILE *fp;
	int cc;
	struct MIB_OID *new_mib_table;
	int i;

	/* sanity check file contents first */
	if ((fp = fopen(filename, "r")) == NULL) {
		LIB_ERROR1("new_mib_from_file: cannot open file: %s\n", filename);
		return (-1);
	}
	mib_entry_count = 0;
	while (fgets(in_buffer, sizeof (in_buffer), fp) != NULL) {
		if (in_buffer[0] != '"')
			continue;

		if ((cc = parse_mib_line(in_buffer, name_buf, num_buf)) < 0) {
			LIB_ERROR1("new_mib_from_file: Illegal line: %s\n", in_buffer);
			fclose(fp);
			return (-1);
		}
		/* sanity check name characters */
		for (i = strlen((char *)name_buf) - 1; i >= 0; --i)
			if ((!isprint(name_buf[i])) || (name_buf[i] == '.')) {
				LIB_ERROR1("new_mib_from_file: Bad mib name: %s\n", name_buf);
				fclose(fp);
				return (-1);
			}
		/* sanity check number characters */
		for (i = strlen((char *)num_buf) - 1; i >= 0; --i)
			if (!(isxdigit(num_buf[i]) || (num_buf[i] == '.') ||
			      (((num_buf[i] == 'x') || (num_buf[i] == 'X')) &&
			       (num_buf[i - 1] == '0')))) {
				LIB_ERROR1("new_mib_from_file: Bad mib number: %s\n", num_buf);
				fclose(fp);
				return (-1);
			}
		/* not a comment, so this will be a new mib entry */
		mib_entry_count++;

	}			       /* end of read wile loop */

	fclose(fp);		       /* close the file and reopen */

	/* allocate a new mib table */
	new_mib_table = (struct MIB_OID *)malloc(sizeof (struct MIB_OID) * (mib_entry_count + 1));

	/* now initialize the new table */
	if ((fp = fopen(filename, "r")) == NULL) {
		LIB_ERROR1("new_mib_from_file: cannot open file: %s\n", filename);
		return (-1);
	}
	new_mib_entry_count = 0;
	while (fgets(in_buffer, sizeof (in_buffer), fp) != NULL) {
		if (in_buffer[0] != '"')
			continue;

		if ((cc = parse_mib_line(in_buffer, name_buf, num_buf)) < 0) {
			LIB_ERROR1("new_mib_from_file: Illegal line: %s\n", in_buffer);
			fclose(fp);
			return (-1);
		}
		/* now allocate memory for these strings and add them to the table */
		new_mib_table[new_mib_entry_count].name = (char *)malloc(strlen((char *)name_buf) + 1);
		strcpy(new_mib_table[new_mib_entry_count].name, (char *)name_buf);

		new_mib_table[new_mib_entry_count].number = (char *)malloc(strlen((char *)num_buf) + 1);
		strcpy(new_mib_table[new_mib_entry_count].number, (char *)num_buf);

		new_mib_entry_count++;
	}

	/* close mib file */
	fclose(fp);

	/* now terminate table */
	new_mib_table[new_mib_entry_count].name = NULL;
	new_mib_table[new_mib_entry_count].number = NULL;

	/* now blow away the old value for mib_oid_table */
	mib_oid_table = new_mib_table;

	/* and re-initialize the data structures */
	init_new_oid_routines(mib_oid_table);

	return (0);
}

parse_mib_line(in_buffer, name_buf, num_buf)
	char *in_buffer;
	char *name_buf;
	char *num_buf;
{
	char *t1, *t2;

	/* add some error checking later */

	t1 = strchr(in_buffer, '"');
	t2 = strchr(&t1[1], '"');
	*t2 = '\0';
	strcpy(name_buf, &t1[1]);

	t1 = strchr(&t2[1], '"');
	t2 = strchr(&t1[1], '"');
	*t2 = '\0';
	strcpy(num_buf, &t1[1]);

	return (0);
}

merge_mib_from_file(filename)
	char *filename;
{
	int orig_mib_entry_count, new_mib_entry_count, mib_entry_count;
	char in_buffer[1024];
	char name_buf[255], num_buf[255];
	FILE *fp;
	int cc;
	struct MIB_OID *new_mib_table;
	int i;

	/* sanity check file contents first */
	if ((fp = fopen(filename, "r")) == NULL) {
		LIB_ERROR1("merge_mib_from_file: cannot open file: %s\n", filename);
		return (-1);
	}
	new_mib_entry_count = 0;
	while (fgets(in_buffer, sizeof (in_buffer), fp) != NULL) {
		if (in_buffer[0] != '"')
			continue;

		if ((cc = parse_mib_line(in_buffer, name_buf, num_buf)) < 0) {
			LIB_ERROR1("merge_mib_from_file: Illegal line: %s\n", in_buffer);
			fclose(fp);
			return (-1);
		}
		/* sanity check name characters */
		for (i = strlen((char *)name_buf) - 1; i >= 0; --i)
			if ((!isprint(name_buf[i])) || (name_buf[i] == '.')) {
				LIB_ERROR1("merge_mib_from_file: Bad mib name: %s\n", name_buf);
				fclose(fp);
				return (-1);
			}
		/* sanity check number characters */
		for (i = strlen((char *)num_buf) - 1; i >= 0; --i)
			if (!(isxdigit(num_buf[i]) || (num_buf[i] == '.') ||
			      (((num_buf[i] == 'x') || (num_buf[i] == 'X')) &&
			       (num_buf[i - 1] == '0')))) {
				LIB_ERROR1("merge_mib_from_file: Bad mib number: %s\n", num_buf);
				fclose(fp);
				return (-1);
			}
		/* not a comment, so this will be a new mib entry */
		new_mib_entry_count++;

	}			       /* end of read wile loop */

	fclose(fp);		       /* close the file and reopen */

	/* Now count how many in current MIB */
	for (orig_mib_entry_count = 0; mib_oid_table[orig_mib_entry_count].name != NULL; orig_mib_entry_count++) ;

	/* allocate a new mib table */
	new_mib_table = (struct MIB_OID *)malloc(sizeof (struct MIB_OID) * (orig_mib_entry_count + new_mib_entry_count + 1));

	/* Load new mib with contents of current MIB */
	new_mib_entry_count = 0;
	while (mib_oid_table[new_mib_entry_count].name != NULL) {

		/* now allocate memory for these strings and add them to the table */
		new_mib_table[new_mib_entry_count].name = (char *)malloc(strlen(mib_oid_table[new_mib_entry_count].name) + 1);
		strcpy(new_mib_table[new_mib_entry_count].name, mib_oid_table[new_mib_entry_count].name);

		new_mib_table[new_mib_entry_count].number = (char *)malloc(strlen(mib_oid_table[new_mib_entry_count].number) + 1);
		strcpy(new_mib_table[new_mib_entry_count].number, mib_oid_table[new_mib_entry_count].number);

		new_mib_entry_count++;
	}			       /* end of loading new mib with current mib */

	/* now add the new table */
	if ((fp = fopen(filename, "r")) == NULL) {
		LIB_ERROR1("merge_mib_from_file: cannot open file: %s\n", filename);
		return (-1);
	}
	while (fgets(in_buffer, sizeof (in_buffer), fp) != NULL) {
		if (in_buffer[0] != '"')
			continue;

		if ((cc = parse_mib_line(in_buffer, name_buf, num_buf)) < 0) {
			LIB_ERROR1("merge_mib_from_file: Illegal line: %s\n", in_buffer);
			fclose(fp);
			return (-1);
		}
		/* now allocate memory for these strings and add them to the table */
		new_mib_table[new_mib_entry_count].name = (char *)malloc(strlen((char *)name_buf) + 1);
		strcpy(new_mib_table[new_mib_entry_count].name, (char *)name_buf);

		new_mib_table[new_mib_entry_count].number = (char *)malloc(strlen((char *)num_buf) + 1);
		strcpy(new_mib_table[new_mib_entry_count].number, (char *)num_buf);

		new_mib_entry_count++;
	}

	/* close mib file */
	fclose(fp);

	/* now terminate table */
	new_mib_table[new_mib_entry_count].name = NULL;
	new_mib_table[new_mib_entry_count].number = NULL;

	/* now blow away the old value for mib_oid_table */
	mib_oid_table = new_mib_table;

	/* and re-initialize the data structures */
	init_new_oid_routines(mib_oid_table);

	return (0);
}

LCOM  "@(#)cmd-nw:nls/English/nwumacro.m4	1.2"
LCOM  "$Id: nwumacro.m4,v 1.5 1994/08/06 21:55:06 vtag Exp $"
LCOM
LCOM Copyright 1991, 1992 Unpublished Work of Novell, Inc.
LCOM All Rights Reserved.
LCOM
LCOM This work is an unpublished work and contains confidential,
LCOM proprietary and trade secret information of Novell, Inc. Access
LCOM to this work is restricted to (I) Novell employees who have a
LCOM need to know to perform tasks within the scope of their
LCOM assignments and (II) entities other than Novell who have
LCOM entered into appropriate agreements.
LCOM 
LCOM No part of this work may be used, practiced, performed,
LCOM copied, distributed, revised, modified, translated, abridged,
LCOM condensed, expanded, collected, compiled, linked, recast,
LCOM transformed or adapted without the prior written consent
LCOM of Novell.  Any use or exploitation of this work without
LCOM authorization could subject the perpetrator to criminal and
LCOM civil liability.
LCOM
LCOM ****************************************************************
LCOM M4 macros for I18 support 
LCOM ****************************************************************
LCOM
LCOM Comment macros:
LCOM  LCOM comment			; Local comment for this file
LCOM  COM(`line')			; Comment line for both .h and .cat files
LCOM  CLINE(`line')			; Line for .cat file
LCOM  HLINE(`line')			; Line for .h file
LCOM	Note: use `' quotes with COM(), CLINE(), and HLINE().
LCOM 
LCOM Message set macro:
LCOM  SET(manifest-constant, setNumber)
LCOM    Where:
LCOM		manifest-constant	is the #define symbol for msgtable.h.
LCOM		setNumber			is the current message set for the
LCOM							.cat file. Sets should start with 2.
LCOM Message string macros:
LCOM  REV_STR( manifest-constant, string)
LCOM		The revision string should be set 1 message 1.
LCOM
LCOM  INFORM_STR( `manifest-constant', value, string) 
LCOM	Where:
LCOM		manifest-constant	is the #define symbol for msgtable.h.
LCOM		value				is the value of the symbol for msgtable.h and
LCOM							the string number.
LCOM		string				is the message string. (Note: all strings
LCOM							should be enclosed within `' quotes.
LCOM
LCOM
LCOM	INFORM_STRs must be in ascending order for gencat.  A 
LCOM	reference to a message string is by set number and message 
LCOM	number via the catgets() function.
LCOM
ifdef(`CAT', `define(MSGQUOTE,`$quote $1')', `define(MSGQUOTE,`dnl')')dnl
ifdef(`CAT', `define(HLINE,`dnl')', `define(HLINE,`$1')')dnl
ifdef(`CAT', `define(CLINE,`$1')', `define(CLINE,`dnl')')dnl 
ifdef(`CAT', `define(COM,$ $1)', `define(COM,/* $1 */)')dnl
ifdef(`CAT', `define(INFORM_STR,`$2 "$3"')',`define(INFORM_STR,`#define $1 $2')')dnl
ifdef(`CAT', `define(INFORM_STR_TAG,`$2 "Module_Name-Module_Version-$2: $3"')',`define(INFORM_STR_TAG,`#define $1 $2')')dnl
ifdef(`CAT', `define(REV_STR,`$ Do NOT touch the following REV string. (MUST match msgtable.h)
1 "$2"')',`define(REV_STR,`#define $1 1 
/* Do NOT touch the following REV string. (MUST match nwumsgs.msg)*/
#define	$1_STR "$2" ')')dnl
ifdef(`CAT', `define(SET,`$set $2  $1')', `define(SET, `#define $1 $2')')dnl
define(`Module_Name', `Undefined')
define(`Module_Version', `0')
LCOM
LCOM Define a quote character so that messages in the .msg file
LCOM can be delimited by quotes.
LCOM
MSGQUOTE(`"')
LCOM

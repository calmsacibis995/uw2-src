/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:xmtravel/xmtravel.h	1.1"
/*
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
 */
/*
 * Motif Release 1.2
 */


#include <stdio.h>
#include <Xm/Xm.h>
#include <Mrm/MrmPublic.h>

#include "menu_cb.h"
#include "client_cb.h"
#include "trip_cb.h"
#include "dialog.h"

/* These definitions correspond to values defined in client.uil, menu.uil */
/* and trip.uil.  The defined values must match those in client.uil, menu.uil */
/* and trip.uil.                                                           */


#define aisle             1
#define client_bboard     2
#define client_main_w     3
#define coach             4
#define kosher            5
#define none_meal         6
#define none_seat         7
#define seafood           8
#define smoking           9
#define vegetarian        10
#define window            11
#define airlines          12
#define cancel_b          13
#define destination       14
#define find_b            15
#define origin            16
#define trip_bboard       17
#define trip_main_w       18
#define trip_name         19
#define first_class       20
#define non_smoking       21
#define business_class    22
#define save_b            23
#define client_name       24

#define menu_c_brief      25
#define menu_c_business   26
#define menu_c_detail     27
#define menu_c_home       28
#define menu_c_save       29
#define menu_c_schedule   30
#define menu_t_find       31

#define address_1         32
#define address_2         33  
#define address_3         34 
#define homephone         35
#define workphone         36

#define L_MAX_WIDGETS  37



typedef struct _travel_info {
  Boolean	changed;
  Boolean	name_entered;
  Boolean      	number_entered;
  Boolean 	date_entered;
  Boolean	air_select; 
  Boolean	ori_select; 
  Boolean	dest_select;
  Widget	c_toplevel;
  Widget	t_toplevel;
  XtAppContext	app_context;
  MrmHierarchy	hierarchy;
} TravelDataRec, *TravelDataPtr;


/* Global data */

extern TravelDataRec	globalData;

extern Widget          l_widget_array [L_MAX_WIDGETS];


/* Hardcoded list of client names */

static char	*names[] = { "Will Brown-Bag", "Ima Hogg", 
                             "U. R. A. Hogg", "Mr. Jes Floggem", 
                             "Lenore", "Dr I. Killem", "Jim Nasium",
                             "Prof.  Flunkem", "Bill Stone",
                             "Dr I. Curem", "Tom Stonecrusher",
                             "John Doe", "Jane Doe" 
} ;



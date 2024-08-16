/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:xmtravel/client_cb.h	1.1"
/*
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
 */
/*
 * Motif Release 1.2
 */

extern void	c_create_widgets();
extern void 	client_select_activate();
extern void 	client_save_activate();
extern void 	schedule_trip_activate();
extern void	first_class_changed();
extern void	business_class_changed();
extern void	coach_changed();
extern void	non_smoking_changed();
extern void	smoking_changed();
extern void	aisle_changed();
extern void	window_changed();
extern void     none_seat_changed();
extern void	data_changed();
extern void	name_changed();
extern void	ok_response();
extern void     cancel_response();
extern void	help_response();
extern void	nomatch_response();
extern void     nomatch_bill_delete();
extern void     cancel_sb_response();
extern void	name_text_popup();
extern void     bill_client();
extern void     delete_client();
extern void	move_left();
extern void	move_right();
extern void	move_down();
extern void	move_up();

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:gui/vga.h	1.5"

#ifndef _VGA_H_
#define _VGA_H_

/*
 * These are appended to either /usr/X or $XWINHOME
 */
#define DO_SAVE	1
#define DO_TEST 2


typedef struct _chipdata {
	char *listInfo; /* formatted line for list widget */
        int item;	/* array index of matching item in vendordata */
} cdata;

struct _current_state {
	Widget toplevel;
	Widget width_widget;
	Widget height_widget;
	Widget spin_text_width;
	Widget spin_text_height;
	Widget width_label;
	Widget height_label;
	Widget inch;
	Widget combobox1;
	Widget combobox2;
	Widget slist2;
	Widget slist1;
	Widget cm;
	Widget monitor_size_inch_option;
	Widget monitor_size_cm_option;
	int num_entries;	/* total number of mode/res entries */
	int num_vendors;	/* total number of vendors */
	
	int curr_vendornum;	/* current vendor array index */
 	int monitor_width;
	int monitor_height; 
	int inch_or_cm_sw;	/* inch =1 or cm = 2 radio button set */
	int monitor_size_button; /* last selected entry for monitor size */
	int resolution_memsize;	/* current memory radio button set */
	int memsize_set;	/* current memory radio button set */
	int chiplist_vendornum;	/* current chiplist array index */
	int chiplist_selected; /* contains the current chiplist item
				selected  array index */
	int full_selected;
	int moderes_selected;	/* current mode/res entry selected */
	int curr_mode;
	int state;
	int chiplist_cnt;	/* current number of entries in chipset list */
	int view_mode;		/* 1= fullist, 2 = chipset list */
	int test_done;
	int exit_sw;
	int changes_made;
	int chipset_state;
	int save_done;
};	

#endif	/* _VGA_H_ */

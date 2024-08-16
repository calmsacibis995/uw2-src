/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/lfb_map.h	1.1"
#if (! defined(__LFB_MAP_INCLUDED__))

#define __LFB_MAP_INCLUDED__



/*
 * System include files
 */

#include <sys/types.h>

/*
 * Path name to the map device
 */

#define	MAP_DEVICE_FILE_PATH\
	"/dev/" "lfb"

/*
 * A structure to specify the extents of a mappable area.
 * Used in the SET_BOUNDS call to the map driver.
 */

struct lfbmap_unit_struct
{

	/* 
	 * Index of the mapped area.
	 */

	int		map_area_index; 

	/* 
	 * Start physical addr of the mapable  area
	 */

	paddr_t	map_start_address_p;

	/* 
	 * Length of the mapable area
	 */

	off_t	map_length;

};

struct  lfbmap_struct
{
	/* 
	 * number of mapable areas 
	 */
	int map_count;

	/*
	 * List of areas to map
	 */
	struct lfbmap_unit_struct *map_struct_list_p;
};
	

/*
 * ioctl commands
 */

#define	MAP	('m' << 16 | 'd' << 8)

#define	MAP_SETBOUNDS		(MAP | 1)
#define	MAP_GETBOUNDS		(MAP | 2)
#define MAP_GETMAPCOUNT		(MAP | 3)
#define	MAP_ENABLEDEBUG		(MAP | 4)
#define	MAP_DISABLEDEBUG	(MAP | 5)
#define	MAP_GETVERSION		(MAP | 6)


#endif

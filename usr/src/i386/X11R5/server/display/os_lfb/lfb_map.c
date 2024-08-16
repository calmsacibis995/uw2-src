/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)os_lfb:os_lfb/lfb_map.c	1.6"

#define PRIVATE
#define PUBLIC
/***
 ***	NAME
 ***
 ***		lfb_map.c :  Pseudo device driver for mapping physical memory
 ***				
 ***	SYNOPSIS
 ***
 ***
 ***	DESCRIPTION
 ***
 ***	This is a pseudo device driver  that will enable privileged
 ***	processes to perform the following operations:
 ***	1. Memory map a particular area
 ***	2. Enable I/O access to ports
 ***
 ***	Memory mapping
 ***  	--------------
 *** 	Before a process can actually memory map an area, the driver
 ***	has to be notified about the list of area that are available
 ***	for mapping. This is done through the ioctl command `MAP_SETBOUNDS'.
 ***	The list is passed in the form of an array of structures of type
 ***	`struct map_struct'. The definition of the structure is as follows:
 ***	
 ***	struct  DRIVER_PREFIX(map_struct)
 ***	{
 ***		int map_count;
 ***		struct map_unit_struct *map_struct_list_p;
 ***	};
 ***
 *** 	The map_unit struture is given below:
 ***
 ***	struct DRIVER_PREFIX(map_unit_struct)
 ***	{
 ***		int		map_area_index; 
 ***		paddr_t	map_start_address_p;		
 ***		off_t	map_length;	
 ***	};
 ***
 ***    Once this is done, any of the areas can be mapped by using the
 ***	mmap system call. The offset specified in the call to mmap should
 ***	fall in atleast one of the areas specified in the ioctl SETBOUNDS
 ***	call.
 ***	Ioctl commands are also provided for
 ***	getting bounds table information (MAP_GETBOUNDS),
 ***	enabling/disabling display of driver debug messages on the
 ***	system console. 
 *** 	
 ***	Enabling I/O access to ports
 ***	----------------------------
 ***	This facility can be availed by a privileged process through
 ***	the ioctl command `MAP_ENABLEIO'. The ioctl command expects
 ***	the following structure:
 ***	
 ***	struct DRIVER_PREFIX(io_port_struct)
 ***	{
 ***		int port_count;
 ***		unsigned short *port_address_list_p;
 ***	};
 ***
 ***	RETURNS
 ***
 ***	MACRO VARIABLES
 ***
 ***		DEBUG: Enable debug messages to appear on system console
 ***
 ***	FILES
 ***
 ***		lfb_map.c: source
 ***		lfb_map.h: Interface file
 ***
 ***	SEE ALSO
 ***
 ***		UNIX System V Release 4.2 Device Driver Reference
 ***
 ***	CAVEATS
 ***
 ***	
 ***	Enabling I/O access using this driver is currently
 ***	unimplemented.
 ***	
 ***	BUGS
 ***
 ***	AUTHORS
 ***	
 ***	HISTORY
 ***
 ***		11_Nov_1993 : First coding.
 ***		5_Mar_1994  :
 ***	1. The offset passed to mmap entry point is now
 ***	being treated as a physical address rather than offset into one
 ***	of the mappable areas. Moreover any of the areas specified in the 
 ***	MAP_SETBOUNDS call can be mapped now.
 ***	2. A new ioctl (MAP_GETVERSION) command to get the driver's version
 ***	number has been added. The calling process has to pass address  of
 ***	a memory location which is atleast 4 bytes long.On return the
 ***	 higher order short word will contain the major version number and
 ***	the lower short word will contain the minor version number.
 ***/

/*
 * M4 Section Starts
 *
 * PRELUDE
 *
 * DEFINITIONS
 *
 * Define the default driver name.  This is the name seen by the
 * kernel code.
 *
 * Switch on tracing if `M4DEBUG' is defined.
 *
 * M4 Section Ends 
 */

PUBLIC

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

PRIVATE

#define		LFB_MAJOR_VERSION_NUMBER		1
#define		LFB_MINOR_VERSION_NUMBER		1

/*
 * define _KERNEL to get kernel specific defines from the system
 * files to follow.  This is usually done from the Makefile for driver
 * code.
 */

#undef	PUBLIC
#undef	PRIVATE

#include <stddef.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <sys/vmparam.h>
#include <sys/immu.h>
#include <sys/kmem.h>
#include <sys/ioctl.h>
#include <sys/cmn_err.h>
#include <sys/moddefs.h>
#include <sys/cred.h>
#include <sys/debug.h>
#include <sys/systm.h>
#include <sys/debug.h>

#include <sys/ddi.h>

/*
 * Driver state structure
 */

struct map_driver_state
{
	int	 map_table_size;
	struct lfbmap_unit_struct*	map_table_p;
	/*
	 * Lower short word: Minor version number
	 * Higher short word: Major version number
	 */
	unsigned int	 driver_version_number;

#if defined(DEBUG)
	int	map_debug;		/*is the driver module in debug mode */
#endif

};

static struct map_driver_state *driver_state_p;

/*
 * The following flag needs to be defined for purposes of the kernel
 * build.  This is not used anywhere in the code.
 */
int lfbdevflag = 0;
	
/*
 * Driver is demand loadable
 */

static int lfbload();

MOD_DRV_WRAPPER(lfb, lfbload, NULL, NULL, "");


/*
 * lfbload
 */


static int
lfbload(void) 
{

	/*
	 * Allocate space for the driver state structure
	 */

	driver_state_p = 
		kmem_zalloc(sizeof(struct map_driver_state), KM_SLEEP);

	if (driver_state_p  ==  NULL)
	{
		cmn_err(CE_NOTE,
			"lfb"
			"Unable to initialize map driver, kmem_zalloc failed!\n");
			return ENOMEM;
	}
	else
	{
		/*
	  	 * Initialize our version number
		 */
		driver_state_p->driver_version_number =
		 (LFB_MAJOR_VERSION_NUMBER << 16) | (LFB_MINOR_VERSION_NUMBER);
		return 0;
	}

}/*mmap_load*/


/*
 * lfbopen
 */

/*ARGSUSED*/
int
lfbopen(dev_t *device_p, int flags, int type, cred_t *credential_p)
{
	/*
	 * Make sure that the driver state structure is available
	 */

	ASSERT(driver_state_p != NULL);

#if defined(DEBUG)
	if (driver_state_p->map_debug)
	{
		cmn_err(CE_CONT,
				"(lfbopen)\n"
				"{\n");
	}
#endif


	/*
	 * Only a privileged process can open this device
	 */

	if (drv_priv(credential_p) != 0)
	{

#if defined(DEBUG)
		if (driver_state_p->map_debug)
		{
			cmn_err(CE_CONT,
					"\tcalling process not privileged!\n"
					"}\n");
		}
#endif

		return EPERM;

	}

	/*
	 * There is nothing to be done in the open routine
	 */

#if defined(DEBUG)
		if (driver_state_p->map_debug)
		{
			cmn_err(CE_CONT,
					"}\n");
		}
#endif

	return 0;					/* success */

}/*map_open*/


/*
 *lfbclose
 */

/*ARGSUSED*/
int
lfbclose(
	dev_t *device_p, 
	int flags, 
	int type, 
	cred_t *credential_p)
{

	ASSERT(driver_state_p != NULL);

#if defined(DEBUG)
	if (driver_state_p->map_debug)
	{
		cmn_err(CE_CONT,
				"(lfbclose){\n}\n");
	}
#endif

	return 0;

}/*mmapclose*/


/*
 * lfbioctl
 */

/*ARGSUSED*/
int
lfbioctl(
	dev_t dev, 
	int command, 
	void *args, 
	int mode,
	cred_t *credential_p, 
	int *rvalp)
{
	int no_of_bytes;	/*No of bytes of kernel memory to be allocated*/

	/*
	 * Temporary map structures
	 */
	struct  lfbmap_struct tmp_map_struct; 
	struct	lfbmap_unit_struct tmp_map_unit_struct; 


	ASSERT(driver_state_p != NULL);

#if defined(DEBUG)
	if (driver_state_p->map_debug)
	{
		cmn_err(CE_CONT,
				"(lfbioctl)\n"
				"{\n");
	}
#endif

	/*
	 * All the ioctl commands   can be exercised only by
	 * a privileged process
	 */
	
	if (drv_priv(credential_p) != 0)
	{

#if defined(DEBUG)
		if (driver_state_p->map_debug)
		{
			cmn_err( CE_CONT,
					"\tcalling process not privileged!\n"
					"}\n");
		}
#endif

		return EPERM;

	}
	

	/*
	 * Process the requested command.
	 */
	
	switch (command)
	{

	case MAP_SETBOUNDS:

#if defined(DEBUG)
		if (driver_state_p->map_debug)
		{
			cmn_err(CE_CONT,
					"\tcmd = MAP_SETBOUNDS\n");
		}
#endif
		/*
		 * The user process should have passed a pointer to
		 * map_struct. Copy it into the driver area.  This will be
		 * used later by the MMAP call.
		 */

		/*
		 * Copy in the structure specifying the number of maps to read
		 * in.
		 */
		if (copyin((caddr_t) args, 
				   (caddr_t) &tmp_map_struct,
				   sizeof(struct lfbmap_struct) ) < 0)
		{
#if defined(DEBUG)
			if (driver_state_p->map_debug)
			{
				cmn_err(CE_CONT,
						"\tCopyin failed!\n"
						"}\n");
			}
#endif
			return EFAULT;
		}
		else					/*  */
		{
			no_of_bytes = sizeof(struct lfbmap_unit_struct) * 
				tmp_map_struct.map_count;

			/*
			 * Note the size of the map table
			 */

			driver_state_p->map_table_size = 
				tmp_map_struct.map_count;
			
			/*
			 * allocate required kernel memory
			 */

			driver_state_p->map_table_p =  
				kmem_zalloc(no_of_bytes, KM_SLEEP );

			if (driver_state_p->map_table_p == NULL)
			{
#if defined(DEBUG)
				if (driver_state_p->map_debug)
				{
					cmn_err(CE_CONT,
							"\tkmem_zalloc failed!\n"
							"}\n");
				}
#endif

				driver_state_p->map_table_size = 0;

				return ENOMEM;

			}

			/*
			 * copy in the list of map areas
			 */
			if (copyin((caddr_t) tmp_map_struct.map_struct_list_p,
					   (caddr_t) driver_state_p->map_table_p, 
					   no_of_bytes) < 0) 
			{

#if defined(DEBUG)
				if (driver_state_p->map_debug)
				{
					cmn_err(CE_CONT,
							"\tCopyin failed!\n"
							"}\n");
				}
#endif

				return EFAULT;

			}
			else
			{

#if defined(DEBUG)
				if (driver_state_p->map_debug)
				{
					cmn_err(CE_CONT,
							"}\n");
				}
#endif

				return 0;

			}
		}
		/*NOTREACHED*/
		break;

	case MAP_GETBOUNDS:	

#if defined(DEBUG)
		if (driver_state_p->map_debug)
		{
			cmn_err(CE_CONT,
					"\tcmd = MAP_GETBOUNDS\n");
		}
#endif
		/*
		 * Get the number of map structures first
		 */
		if (copyin((caddr_t) args, 
				   (caddr_t) &tmp_map_unit_struct,
				   sizeof(tmp_map_unit_struct)) < 0)
		{
#if defined(DEBUG)
			if (driver_state_p->map_debug)
			{
				cmn_err(CE_CONT,
						"\tCopyin failed!\n"
						"}\n");
			}
#endif

				return EFAULT;
		}

		/*
		 * check validity of map unit number passed by the
		 * user process
		 */
		if((tmp_map_unit_struct.map_area_index >=
			driver_state_p->map_table_size) || 
		   (tmp_map_unit_struct.map_area_index < 0))
		{

#if defined(DEBUG)
			if (driver_state_p->map_debug)
			{
				cmn_err(CE_CONT,
						"\tInvalid map unit number\n"
						"}\n");
			}
#endif

			return EINVAL;

		}

		/*
		 * Everything is fine, copy the required entry to user space
		 */
		if (copyout((caddr_t) &(driver_state_p->
					map_table_p[tmp_map_unit_struct.map_area_index]),
					(caddr_t) args, 
					sizeof(struct lfbmap_unit_struct)) <0)
		{

#if defined(DEBUG)
			if (driver_state_p->map_debug)
			{
				cmn_err(CE_CONT,
						"\tCopyout failed!\n"
							"}\n");
			}
#endif

				return EFAULT;
			
		}
		else
		{

#if defined(DEBUG)
			if (driver_state_p->map_debug)
			{
				cmn_err(CE_CONT,
						"}\n");
			}
#endif
			return 0;
		}
		/*NOTREACHED*/
		break;
		
	case MAP_ENABLEDEBUG:

#if defined(DEBUG)
		if (driver_state_p->map_debug)
		{
			cmn_err(CE_CONT,
					"\tcmd = MAP_ENABLEDEBUG\n"
					"}\n");
		}
#endif

#if defined(DEBUG)

		driver_state_p->map_debug=1;
		
		return 0;

#else

		return EINVAL;

#endif
		/*NOTREACHED*/
		break;
		

	case MAP_DISABLEDEBUG:

#if defined(DEBUG)
		if (driver_state_p->map_debug)
		{
			cmn_err(CE_CONT,
					"\tcmd = MAP_DISABLEDEBUG\n"
					"}\n");
		}
#endif

#if defined(DEBUG)

		driver_state_p->map_debug=0;

		return 0;

#else
		return EINVAL;
#endif
		/*NOTREACHED*/
		break;
		
	case MAP_GETVERSION:
#if defined(DEBUG)
		if (driver_state_p->map_debug)
		{
			cmn_err(CE_CONT,
					"\tcmd = MAP_GETVERSION\n"
					"}\n");
		}
#endif
		/*
		 * Copyout the driver's version number, if it fails
		 * return EFAULT
		 */ 
		if (copyout((caddr_t)(&driver_state_p->driver_version_number),
				(caddr_t)args,
				 sizeof(driver_state_p->driver_version_number)) < 0)
		{
			return EFAULT;
		}
		else
		{
			return 0;
		}
		/*NOTREACHED*/
		break;
	default:

#if defined(DEBUG)
		if (driver_state_p->map_debug)
		{
			cmn_err(CE_CONT,
					"\tcmd = Invalid\n"
					"}\n");
		}
#endif

		break;

	}/*switch*/
	
	/*
	 * If we reached here, return invalid argument error
	 */

	return EINVAL;
}


/*
 * mmap entry point.
 */
/*ARGSUSED*/
int
lfbmmap(
	dev_t device, 
	off_t offset, 
	int protection_flag)
{
	int	entry_number = 0;

	
	ASSERT(driver_state_p != NULL);

#if defined(DEBUG)
	if(driver_state_p->map_debug)
	{
		cmn_err(CE_CONT,
				"(lfbmmap)\n"
				"{\n"
				"\toffset = %x\n",
				(int)offset);
	}
#endif

	/*
	 * Check if the map tables have been initialized
	 */
	if ((driver_state_p->map_table_p == NULL) ||
		( driver_state_p->map_table_size <= 0 ))
	{
#if defined(DEBUG)
		if(driver_state_p->map_debug)
		{
			cmn_err(CE_CONT,
					"\t#map_table not initialized\n"
					"}\n");
		}
#endif 
		return NOPAGE; 
	}
	/*
	 * Our job is pretty simple. The offset that is given down to us is an 
	 * absolute  physical address, just make sure that this address
	 * falls into one of the ``map unit''s, if so return the physical
	 * page number corresponding to this offset, otherwise return NOPAGE 
	 */

	for(entry_number=0;entry_number < driver_state_p->map_table_size;
		 ++entry_number)
	{

		if ((driver_state_p->map_table_p[entry_number].map_start_address_p <=
			offset) && 
			(driver_state_p->map_table_p[entry_number].map_start_address_p +
			driver_state_p->map_table_p[entry_number].map_length > offset))
		{
			/*
			 * Found match
			 */
			 break;
		}
	}
	if (entry_number == driver_state_p->map_table_size)
	{
#if defined(DEBUG)
		if(driver_state_p->map_debug)
		{
			cmn_err(CE_CONT,
					"\t#Map request out of range\n"
					"}\n");
		}
#endif 
		return NOPAGE;
	}
#if defined(DEBUG)
		if(driver_state_p->map_debug)
		{
			cmn_err(CE_CONT,
					"}\n");
		}
#endif 
	return hat_getppfnum(offset,PSPACE_MAINSTORE);
}

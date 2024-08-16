/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pdi.cmds:scsicomm.c	1.8"

/*  "error()" has been internationalized. The string to be output
 *  must at least include the message number and optionally a catalog name.
 *  The string is output using <MM_ERROR>.
 *
 *  "warning()" has been internationalized. The string to be output
 *  must at least include the message number and optionally a catalog name.
 *  The string is output using <MM_WARNING>.
 */


#include	<sys/types.h>
#include	<sys/mkdev.h>
#include	<sys/stat.h>
#include	<sys/sdi_edt.h>
#include	<fcntl.h>
#include	<errno.h>
#include	<sys/sysi86.h>
#include	<sys/vtoc.h>
#include	<sys/sd01_ioctl.h>
#include	<string.h>
#include	<stdio.h>
#include	"mirror.h"
#include	"scsicomm.h"
#include 	<ctype.h>
#include	<pfmt.h>
#include	<locale.h>

#define	SD01_DEV	"/dev/rdsk/c0b0t0d0s0"
extern int	errno;
extern int	Debug;

/*
 *  Look in the mdevice file to get driver major numbers 
 *	Inputs:  drvrname - pointer to driver name.
 *	         count - pointer where to put the number of major numbers
 *	Return:  address of drv_majors structure
 *		 0 otherwise.
 */

struct drv_majors *
GetDriverMajors(drvrname, count)
char	*drvrname;
int	*count;
{
	int	i, firstblk, lastblk, firstchr, lastchr;
	int	block_mode_flag;
	FILE	*mdevicefp;
	char	*ptr, tmpdrvname[NAME_LEN], field[MFIELDS][MAXFIELD];
	struct	drv_majors *drvptr, drv_maj[MAXMAJNUMS];
	char	mdevice[256];
	int	mult_maj;

	for (i=0; i<NAME_LEN; i++)	{
		if ((*(drvrname + i) > 'A') && ((*(drvrname + i) < 'Z')))   {
			tmpdrvname[i] = *(drvrname + i) + 0x20;
		} else	{
			tmpdrvname[i] = *(drvrname + i);
		}
	}

	sprintf(mdevice,"/etc/conf/mdevice.d/%s",drvrname);
	if (Debug) fprintf(stderr, "GetDriverMajors(%s  %s)\n",drvrname, tmpdrvname);

	/*  open the mdevice file  */
	if ((mdevicefp = fopen(mdevice,"r")) == NULL) { 
		errno = 0;
		if (Debug)
			warning("Could not open %s\n",mdevice);
		return((struct drv_majors *) NULL);
	}

	*count = 0;
	mult_maj = 0;
	/*  look for driver in each of the lines in the mdevice file  */
	if (ParseLine1(field,mdevicefp,MFIELDS) == MFIELDS)	{
		block_mode_flag = (int) strchr(field[2], 'b');

		if (block_mode_flag)	{
			firstblk = atoi(field[4]);
			if (ptr = strchr(field[4],'-')) {
				*ptr++ = '\0';
				mult_maj = 1;
				lastblk = atoi(ptr);
			}
		}

		firstchr = atoi(field[5]);
		if (ptr = strchr(field[5],'-')) {
			*ptr++ = '\0';
			mult_maj = 1;
			lastchr = atoi(ptr);
		}
		if (mult_maj) {
			if (block_mode_flag && ((lastblk - firstblk) != (lastchr - firstchr)))	{
				errno = 0;
				warning(":392:number of block major numbers does not equal number of character major numbers\n");
				fclose(mdevicefp);
				return((struct drv_majors *) NULL);
			}
		}
		else 
				lastchr = atoi(field[5]);

		for (i=firstchr; i <= lastchr; i++, (*count)++)     {
			if (block_mode_flag)	
				drv_maj[*count].b_maj = firstblk++;
			else	
				drv_maj[*count].b_maj = NO_MAJOR;
			drv_maj[*count].c_maj = i;
			if (Debug) fprintf(stderr, "Driver %s\t:  cmaj=%d,\tbmaj=%d\n",tmpdrvname, drv_maj[*count].c_maj, drv_maj[*count].b_maj);
		}

	fclose(mdevicefp);
	}
	else {
		fclose(mdevicefp);
		return((struct drv_majors *) NULL);
	}

	if (*count == 0)
		return((struct drv_majors *) NULL);

	/*  Allocate space for drv_majors structure  */
	if ((drvptr = (struct drv_majors *) calloc(1, sizeof(struct drv_majors) * (*count))) == NULL)	{
		errno = 0;
		warning(":386:calloc for major number structure failed\n");
		fclose(mdevicefp);
		return((struct drv_majors *) NULL);
	}
	memcpy(drvptr, drv_maj, sizeof(struct drv_majors) * (*count));
	return(drvptr);
}

/* a routine to parse /etc/conf/mdevice.d/module files.
/* Invoked in GetDriverMajors. Special chars in those files
/* have to be skipped.
*/

int
ParseLine1(array,fp, numfields)
char array[][MAXFIELD];
FILE *fp;
int numfields;
{
	int	i, j;
	char line[BUFSIZ];
	char *linep;
	char a[MAXFIELD];

	/*  inititalize the array to null strings  */
	for (i=0; i<numfields; i++) 
		array[i][0] = '\0';

	/*  skip over comment lines */
	j = 0;
	while (fgets(line, BUFSIZ, fp)) {
		linep = line;
		if ((*linep == '#') || (*linep == '*') || (*linep == '$')) 
			for (++linep; *linep != '\0'; ++linep);
		else {
			while ((*linep != '\n') && (*linep != '\0')) {
				while (isspace(*linep)) linep++;
				for (i=0;i < 20; i++)
					a[i] = '\0';
				for (i=0; !isspace(*linep); i++, linep++)
					a[i] = *linep;
				strcpy (array[j++], a);
			}
		}
	}
	if (j != numfields) {
		errno = 0;
		warning(":367:Number of columns incorrect in file.\n");
	}
	return(j);
}

/*
 * read the next line from a file, skipping over white space
 * and comments and place each field into a character array. 
 * Returns the number of fields read or EOF.
 */

int
ParseLine(array,fp, numfields)
char array[][MAXFIELD];
FILE *fp;
int numfields;
{
	int	i, j;
	char	ch;

	/*  inititalize the array to null strings  */
	for (i=0; i<numfields; i++) 
		array[i][0] = '\0';

	/*  skip over comment lines , reading the remainder of the line  */
	while ((ch = getc(fp)) == '#') {
		fscanf(fp,"%*[^\n]%*[\n]");
	}
	if (ungetc(ch,fp) == EOF)
		return(EOF);

	j = 0;
	for (i=0; i<numfields; i++) {
		if(fscanf(fp,"%s",array[i]) == EOF) {
			j = 0;
			break;
		}
		j++;
	}
	/*  read the remainder of the line  */
	fscanf(fp,"%*[^\n]%*[\n]");
	if (j == 0) {
		return(EOF);
	} else {
		if (j != numfields) {
			errno = 0;
			warning(":367:Number of columns incorrect in file.\n");
		}
		return(j);
	}
}

/*
 *  Check to see if the device contains any mirrored partitions
 *	Inputs:  dev_name - pointer to the name of the block device.
 *	Return:  1 if device contains a mirrored partition
 *		 0 otherwise.
 *		 2 can't tell.
 */

int
ckmirror(devname)
char	*devname;
{
	int		i, j, mir, cnt, num_part;
	struct MTABLE	*mtabptr, *omtabptr;
	struct stat	statbuf, mirstat;

	if (stat(MIRROR_TABLE, &mirstat) < 0)	{
		if (errno == ENOENT)
			return(0);
		warning(":368:stat of '%s' failed\n", MIRROR_TABLE);
		return(2);
	}

	if ((mtabptr = (struct MTABLE *) malloc(mirstat.st_size)) == NULL)    {
		errno = 0;
		warning(":369:malloc for %s failed\n", MIRROR_TABLE);
		return(2);
	}

	if (stat(devname, &statbuf) < 0)	{
		warning(":368:stat of '%s' failed\n", devname);
		return(2);
	}

	if ((mir = open(MIRROR_TABLE, 0)) < 0)	{
		warning(":370:open of '%s' failed\n", MIRROR_TABLE);
		return(2);
	}

	if ((cnt = read(mir, mtabptr, mirstat.st_size)) != mirstat.st_size)   {
		warning(":371:read of '%s' failed\n", MIRROR_TABLE);
		close(mir);
		return(2);
	}

	close(mir);
	if (cnt % sizeof(struct MTABLE))	{
		errno = 0;
		warning(":372:size of '%s' is not correct\n",
			MIRROR_TABLE);
		return(2);
	}

	omtabptr = mtabptr;
	for (i=0; i < cnt/sizeof(struct MTABLE); i++)	{
		for (j=0; j<NDISKPARTS; j++)	{
			if (strlen(mtabptr->mt_dpart[j].dp_name) == 0)
				continue;
			if ((statbuf.st_rdev / num_part) == (mtabptr->mt_dpart[j].dp_dev / num_part))
				return(1);
		}
		mtabptr++;
	}
	free(omtabptr);
	return(0);
}

/*
 *  Routine to read in the pdsector and VTOC.
 *	Inputs:  devname - pointer to the device name
 *	         vtocptr - pointer to where to put the VTOC
 *	Return:  1 if pdsector VTOC was read in
 *		 0 otherwise.
 */

int
rd_vtoc(dpart,vbuf)
char	*dpart;		/* Disk partition name */
struct	vtoc	*vbuf;	/* buf for VTOC data */
{
	int		fd;		/* Disk Partition File Descriptor */
	struct vtoc	*vtoc;
	struct pdinfo	*pdsector;
	unsigned	voffset;
	char		secbuf[512];
	struct phyio	args;		/* IO command buffer */

	/*  Open the Disk Partition.  */
	if ((fd = open(dpart,O_RDONLY)) == -1)
	{
		pfmt(stderr, MM_ERROR,
			":373:Cannot open disk partition '%s'!\n", dpart);
		pfmt(stderr, MM_ERROR,
			":374:system call error is: %s\n", strerror(errno));
		return(0);
	}

	/*
	*  Read the disk Physical Descriptor (PD) block.
	*  (Got the following stuff from prtvtoc)
	*/
	args.sectst = 0;
	args.memaddr = (unsigned long) &secbuf;
	args.datasz = 512;
	if (ioctl(fd,V_PDREAD,&args) == -1)
	{
		pfmt(stderr, MM_ERROR,
			":375:V_PDREAD ioctl to %s failed:\n", dpart);
		pfmt(stderr, MM_ERROR,
			":374:system call error is: %s\n", strerror(errno));
		close(fd);
		return(0);
	}

	close(fd);
	if (args.retval != 0)
	{
		pfmt(stderr, MM_ERROR,
			":376:Bad return value from V_PDREAD ioctl to %s:\n",
				dpart);
		return(0);
	}

	pdsector = (struct pdinfo *) secbuf;
	if (pdsector->sanity != VALID_PD)
	{
		pfmt(stderr, MM_ERROR,
			":377:PD sector on '%s' is insane!\n", dpart);
		return(0);
	}

        voffset = pdsector->vtoc_ptr & 511;
        vtoc = (struct vtoc *) (secbuf + voffset);
	if (vtoc->v_sanity != VTOC_SANE)
	{
		pfmt(stderr, MM_ERROR,
			":378:VTOC on '%s' is insane!\n", dpart);
		return(0);
	}

	*vbuf = *vtoc;
	close(fd);
	return(1);
}

/*
 *  Routine to print out error information.
 *	Inputs:  message - pointer to the string of the error message
 *	         data1...data5 - pointers to additional arguments
 *
 *	NOTE:  This routine does not return.  It exits.
 */

void
error(message, data1, data2, data3, data4, data5)
char	*message;	/* Message to be reported */
long	data1;		/* Pointer to arg	 */
long	data2;		/* Pointer to arg	 */
long	data3;		/* Pointer to arg	 */
long	data4;		/* Pointer to arg	 */
long	data5;		/* Pointer to arg	 */
{
	(void) fflush(stdout);
	if (message[0] == ':')
		(void) pfmt(stderr, MM_ERROR,
			message, data1, data2, data3, data4, data5);
	else 	{
		(void) fprintf(stderr, "ERROR: ");
		(void) fprintf(stderr, message, data1, data2, data3, data4, data5);
	}
	if (errno)
		pfmt(stderr, MM_ERROR,
			":374:system call error is: %s\n", strerror(errno));

	exit(ERREXIT);
}

/*
 *  Routine to print out warning information.
 *	Inputs:  message - pointer to the string of the warning message
 *	         data1...data5 - pointers to additional arguments
 */

void
warning(message, data1, data2, data3, data4, data5)
char	*message;	/* Message to be reported */
long	data1;		/* Pointer to arg	 */
long	data2;		/* Pointer to arg	 */
long	data3;		/* Pointer to arg	 */
long	data4;		/* Pointer to arg	 */
long	data5;		/* Pointer to arg	 */
{
	(void) fflush(stdout);
	if (message[0] == ':')
		(void) pfmt(stderr, MM_WARNING,
			message, data1, data2, data3, data4, data5);
	else 	{
		(void) fprintf(stderr, "WARNING: ");
		(void) fprintf(stderr, message, data1, data2, data3, data4, data5);
	}
	if (errno) {
		pfmt(stderr, MM_ERROR,
			":374:system call error is: %s\n", strerror(errno));
		errno = 0;
	}
}

void
nowarning(message, data1, data2, data3, data4, data5)
char	*message;	/* Message to be reported */
long	data1;		/* Pointer to arg	 */
long	data2;		/* Pointer to arg	 */
long	data3;		/* Pointer to arg	 */
long	data4;		/* Pointer to arg	 */
long	data5;		/* Pointer to arg	 */
{
	return;
}

/*
 *  Return the name of the block device given the name of the
 *  character device.
 *	Inputs:  rpart - pointer to the name of the character partition.
 *	         bpart - address of where to put the block device name
 *	Return:  1 if constructed the block device name
 *		 0 otherwise.
 */

int
get_blockdevice(rpart, bpart)
char	*rpart, *bpart;
{
	char	*ptr;
	struct	stat	statbuf;

	/*  Verify rpart is a character device special file.  */
	if (stat(rpart, &statbuf) == -1)
	{
		return(0);
	}

	if ((statbuf.st_mode & S_IFMT) != S_IFCHR)
	{
		return(0);
	}

	strcpy(bpart, rpart);
	if ((ptr = strchr(rpart,'r')) == NULL)
	{
		return(0);
	}
	strcpy(strchr(bpart, 'r'), ++ptr);
	return(1);
}

/*
 *  Swap bytes in a 16 bit data type.
 *	Inputs:  data - long containing data to be swapped in low 16 bits
 *	Return:  short containing swapped data
 */

short
scl_swap16(data)
unsigned long	data;
{
	unsigned short	i;

	i = ((data & 0x00FF) << 8);
	i |= ((data & 0xFF00) >> 8);

	return (i);
}

/*
 *  Swap bytes in a 24 bit data type.
 *	Inputs:  data - long containing data to be swapped in low 24 bits
 *	Return:  long containing swapped data
 */

long
scl_swap24(data)
unsigned long	data;
{
	unsigned long	i;

	i = ((data & 0x0000FF) << 16);
	i |= (data & 0x00FF00);
	i |= ((data & 0xFF0000) >> 16);

	return (i);
}

/*
 *  Swap bytes in a 32 bit data type.
 *	Inputs:  data - long containing data to be swapped
 *	Return:  long containing swapped data
 */

long
scl_swap32(data)
unsigned long	data;
{
	unsigned long	i;

	i = ((data & 0x000000FF) << 24);
	i |= ((data & 0x0000FF00) << 8);
	i |= ((data & 0x00FF0000) >> 8);
	i |= ((data & 0xFF000000) >> 24);

	return (i);
}

/*
 *  Return the SCSI Equipped Device Table
 *	Inputs:  hacnt - pointer to integer to place the number of HA's.
 *	Return:  address of the EDT
 *	         0 if couldn't read the EDT
 */

struct scsi_edt *
readedt(hacnt)
int	*hacnt;
{
	struct	scsi_edt *xedt;
	int	sdi_fd, ha_count;
	char 	*mktemp();
	char 	sditempnode[20];
	dev_t	sdi_dev;

	setuid (0);

	*hacnt = 0;

	/* get device for sdi */
	if (sysi86(SI86SDIDEV, &sdi_dev) == -1) {
		error (":387:ioctl(SI86SDIDEV) failed.\n");
		return(0);
	}

	strcpy(sditempnode, TEMPNODE);
	mktemp(sditempnode);

	if (mknod(sditempnode, (S_IFCHR | S_IREAD), sdi_dev) < 0) {
		error(":391:mknod failed for sdi temp device\n");
		return(0);
	}

/*
 *	This open will no longer fail because we are using a
 *	special pass_thru major which is only for issuing sdi_ioctls.
 *	This open does not require exclusive use of the pass_thru
 *	to an HBA so there is no problem with it being in use.
 */
	errno = 0;
	if ((sdi_fd = open(sditempnode, O_RDONLY)) < 0) {
		unlink (sditempnode);
		error(":382:Cannot open sdi device: %s\n", sditempnode);
	}

	/*  Get the Number of HA's in the system  */
	ha_count = 0;
	if (ioctl(sdi_fd, B_HA_CNT, &ha_count) < 0)  {
		(void) close(sdi_fd);
		unlink(sditempnode);
		error(":388:ioctl(B_HA_CNT) failed\n"); 
		return(0);
	}

	if (ha_count == 0)	{
		(void) close(sdi_fd);
		unlink(sditempnode);
		errno = 0;
		error(":383:Unable to determine the number of HA boards.\n");
		return(0);
	}

	*hacnt = ha_count;
	/*  Allocate space for SCSI EDT  */
	if ((xedt = (struct scsi_edt *) calloc(1, sizeof(struct scsi_edt) * MAX_TCS * ha_count)) == NULL)	{
		(void) close(sdi_fd);
		unlink(sditempnode);
		errno = 0;
		error(":379:Calloc for EDT structure failed\n");
		return(0);
	}

	/*  Read in the SCSI EDT  */
	if (ioctl(sdi_fd, B_REDT, xedt) < 0)  {
		(void) close(sdi_fd);
		unlink(sditempnode);
		error(":389:ioctl(B_REDT) failed\n"); 
		return(0);
	}

	(void) close(sdi_fd);
	unlink(sditempnode);
	return(xedt);
}

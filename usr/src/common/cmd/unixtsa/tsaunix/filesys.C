#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/filesys.C	1.9"
/*

$Abstract: 
This file contains the functions which access the unix file system table which
is /etc/fstab on BSD systems. For other variants of unix these functions have to
be implemented.

*/

#include <tsad.h>
#include "tsaglobl.h"
#include "filesys.h"
#include   <sys/mnttab.h>
#include <sys/wait.h>
#include	<string.h>

/***
 *
 *  name	IsRemovable() - return 0 if a specified device is
 *			removable, non-zero otherwise
 *
 *		On SVR4 systems, returns 3 if Device is not recognized
 *		in /etc/device.tab and 4 if Device is found but is not
 *		removable.
 *
 ***/

#define	DEVATTR		"devattr"	// command to use
#define	REMOVABLE	"removable"	// device attribute to test for

static int IsRemovable( char *Device )
    {
    int		rc ;
    char	command_buffer[64] ;

    sprintf(command_buffer, "%s %s %s >/dev/null 2>&1",
	DEVATTR, Device, REMOVABLE) ;
    rc = system(command_buffer) ;

    return(WEXITSTATUS(rc)) ;
    }

/***
 *
 *  name	hasmntopt - parse mount option string
 *
 *  synopsis	char *hasmntopt(struct mnttab	*mnt, char *opt) ;
 *
 ***/

char *hasmntopt(struct mnttab *Mnt, char *Opt)

    {
    char	*ptr ;

    ptr = strtok(Mnt->mnt_mntopts, ",") ;
    while ( ptr != NULL )
	{
	if ( strcmp(ptr, Opt) == 0 )
	    return (ptr) ;
	else
	    ptr = strtok((char *)0, ",") ;
	}
    return((char *)0) ;
    }

/*
 * OpenFilesystemTable - Open the file system table
 *
 * synopsis : int OpenFilesystemTable(unsigned int *filep)
 *
 * description :
 *
 * return : 0 - success
 *         -1 - failure
 */
int OpenFilesystemTable(FSTAB_PTR *filep)
{
	*filep = fopen(MNTTAB,"r") ;

	if ( *filep == NULL ) {
		return(-1);
	}
	return(0);
}

/*
 * GetLocalFilesystemEntry - Get the next local filesystem table entry from
 *                           the file system table.
 *
 * synopsis : RESOURCE_LIST *GetLocalFilesystemEntry(FSTAB_PTR filep)
 *
 * Description :
 *
 *
 * Returns : pointer to RESOURCE_LIST structure, or NULL
 * 
 * Warning : This function returns the  pointer to a static area. The calling
 * function must copy the data if necessary.
 */
RESOURCE_LIST *GetLocalFilesystemEntry(FSTAB_PTR filep)
{
	static RESOURCE_LIST	resourceStr ;
	struct mnttab		mnt_buf ;
	struct stat 		stat_buf ;

	for (;;) { /* Loop until a local fs or end */

		/* get next filesystem entry */
		if ( getmntent(filep, &mnt_buf) < 0 ) {
			return(NULL);
			}

		/*
		 * a local filesystem can be stat()'ed and is either a
		 * block or character special file.
		 */
		if ( stat(mnt_buf.mnt_special, &stat_buf) != 0 )
			continue ;
		else if ( S_ISBLK(stat_buf.st_mode) || 
				S_ISCHR(stat_buf.st_mode) ) {

			if ( stat(mnt_buf.mnt_mountp, &stat_buf) == 0 )
			    break ;
		}
	}


	strcpy(resourceStr.resourceName,mnt_buf.mnt_mountp);
	resourceStr.validResource = 1 ;
	resourceStr.resourceFlags = 0 ;
	resourceStr.resourceNameLength = strlen(resourceStr.resourceName);
	resourceStr.resourceDevice = stat_buf.st_dev ;
	if ( hasmntopt(&mnt_buf,MNTOPT_RO) ) {
		resourceStr.resourceFlags |= FS_READONLY ;
	}
	if ( IsRemovable(mnt_buf.mnt_special) == 0 ) {
		resourceStr.resourceFlags |= FS_REMOVABLE ;
	}
	return(&resourceStr);
}

/*
 * CloseFileSystemTable - Close the file system table.
 *
 * synopsis : void CloseFilesystemTable(FSTAB_PTR filep)
 *
 */
void CloseFilesystemTable(FSTAB_PTR filep)
{
	fclose(filep) ;
}

/*
 * GetResourceInfo - Get the information about the filesystem mounted on 
 * 
 * synopsis: int GetResourceInfo(char *mnt_dir,RESOURCE_INFO_STRUCTURE *resInfo)
 *
 * Description :
 *
 * return :
 *
 * remarks : Current implementation is SUN OS specific
 */
int GetResourceInfo(char *mnt_dir,RESOURCE_INFO_STRUCTURE *resInfo)
{
	struct statvfs buf ;
	int rc ;

	rc = statvfs(mnt_dir,&buf);
	if ( rc ) {
		return(rc);
	}
	resInfo->blockSize = (UINT16)(buf.f_frsize);
	resInfo->totalBlocks = (UINT32)(buf.f_blocks);
	resInfo->freeBlocks = (UINT32)(buf.f_bfree);
	resInfo->resourceIsRemovable = FALSE ;
	
	return(rc);
}

int FindFirstDirEntry(char *name, DIRECTORY_STRUCT *Entry, UINT16 scanningMode)
{
	DIR *dirp ;
	struct dirent *dirEntry ;
	char tempBuffer[MAXPATHLEN+1] ;
	int namelen ;

	if ( (dirp = opendir(name)) == NULL ) {
#ifdef DEBUG
		logerror(PRIORITYWARN, "couldn't opendir(%s) (%d)\n", name, errno) ;
		fflush(PRIORITYWARN);
#endif
		return(-1);
	}

	namelen = strlen(name+1);

	/* Get the first directory entry */
	for (;;) {
		if ((dirEntry = readdir(dirp)) == NULL ) {
#ifdef DEBUG
		logerror(PRIORITYWARN, "couldn't readdir (%d)\n", errno) ;
		fflush(PRIORITYWARN);
#endif
			closedir(dirp);
			return(-2);
		}
		if ( namelen ) { // name is not root
			sprintf(tempBuffer,"%s/%s",name,dirEntry->d_name);
		}
		else { // name is root
			sprintf(tempBuffer,"/%s",dirEntry->d_name);
		}
#ifdef DEBUG
		logerror(PRIORITYWARN, "looking at %s\n", tempBuffer) ;
		fflush(PRIORITYWARN);
#endif
		
		if ( lstat(tempBuffer,&Entry->statb) ) {
			closedir(dirp);
			return(-3);
		}
		if (scanningMode == SCANNING_FILES && 
					!S_ISDIR(Entry->statb.st_mode)) {
			break ;
		}
		else if (scanningMode == SCANNING_DIRS && 
					S_ISDIR(Entry->statb.st_mode)) {
			break ;
		}
	}

	Entry->dirHandle = dirp ;
	strcpy(Entry->name,dirEntry->d_name);
	Entry->namelen = strlen(Entry->name) ;
	strcpy(Entry->parentPath,name);

	return(0);
}

int FindNextDirEntry(DIRECTORY_STRUCT *Entry, UINT16 scanningMode)
{
	DIR *dirp = (DIR *)(Entry->dirHandle);
	struct dirent *dirEntry ;
	char tempBuffer[MAXPATHLEN+1] ;
	int parentPathLen ;

	parentPathLen = strlen(Entry->parentPath+1);
	/* Get the next directory entry */
	for (;;) {
		if ((dirEntry = readdir(dirp)) == NULL ) {
			closedir(dirp);
			Entry->dirHandle = NULL ;
			return(-2);
		}
	
		if ( parentPathLen ) { // parent is not root
			sprintf(tempBuffer,"%s/%s",Entry->parentPath,
							dirEntry->d_name);
		}
		else { // parent is root
			sprintf(tempBuffer,"/%s",dirEntry->d_name);
		}
#ifdef DEBUG
		logerror(PRIORITYWARN, "looking at %s\n", tempBuffer) ;
		fflush(PRIORITYWARN);
#endif
		
		if ( lstat(tempBuffer,&Entry->statb) ) {
			closedir(dirp);
			Entry->dirHandle = NULL ;
			return(-3);
		}
		if (scanningMode == SCANNING_FILES && 
					!S_ISDIR(Entry->statb.st_mode)) {
			break ;
		}
		else if (scanningMode == SCANNING_DIRS && 
					S_ISDIR(Entry->statb.st_mode)) {
			break ;
		}
	}

	strcpy(Entry->name,dirEntry->d_name);
	Entry->namelen = strlen(Entry->name) ;

	return(0);
}

int OpenAndLockFile(char *path, SMFILE_HANDLE *smFileHandle, int mode)
{
	int flags = O_RDWR, fd ;

	if ((mode & NWSM_OPEN_MODE_MASK) == NWSM_OPEN_READ_ONLY ) {
		flags = O_RDONLY ;
	}
	if ((fd = open(path,flags)) == -1) {
		if ( errno == EACCES && flags == O_RDWR && mode != 0 ) {
			flags = O_RDONLY ;
			if ((fd = open(path,flags)) == -1) {
				return(HandleUnixError(errno));
			}
		}
		return(HandleUnixError(errno));
	}

	if ((mode & NWSM_OPEN_MODE_MASK) != NWSM_OPEN_READ_ONLY ) {
		// Try lock with deny write access
		flock_t lockStructure ;
		int prevErr ;

		memset((char *)&lockStructure,'\0',sizeof(flock_t));
		lockStructure.l_type = F_WRLCK ;
		lockStructure.l_whence = SEEK_SET ;
		if ( fcntl(fd,F_SETLK,&lockStructure) == -1 ) {
			/* locking operation failed. If errno is ENOSYS, locking
			   is not allowed on this filesystem. So just go ahead and
			   backup and pray that nothing has changed before the file
			   is fully backed up
			*/
			if ( errno != ENOSYS ) {
				if ( mode == 0  || (mode & NWSM_OPEN_MODE_MASK) ==
						NWSM_USE_LOCK_MODE_IF_DW_FAILS) { 
					prevErr = errno ;
					close(fd);
					return(HandleUnixError(prevErr));
				}
			}
		}
	}
	smFileHandle->FileHandle = fd ;
	return(0);
}
 
int UnlockAndClose(int fileHandle) 
{
	close(fileHandle);
	return(0) ;
}

int ReadFile(SMFILE_HANDLE *smFileHandle, 
int _bytesToRead, 
char **buffer,
unsigned int  *_bytesRead)
{
	int bytes ;
	char *bufferPtr = *buffer ;

	*_bytesRead = 0 ;
	lseek(smFileHandle->FileHandle,smFileHandle->position,SEEK_SET);

	while(_bytesToRead > 0) {
		bytes = (int) read(smFileHandle->FileHandle,bufferPtr,(size_t)_bytesToRead);
		if ( bytes > 0 ) {
			smFileHandle->position += bytes ;
		}
		if ( bytes > _bytesToRead || bytes == -1 ) {
			return(NWSMTS_READ_ERROR);
		}
		*_bytesRead += bytes ;
		bufferPtr += bytes ;
		_bytesToRead -= bytes ;
	}
	return(0);
}

int WriteFile(SMFILE_HANDLE *smFileHandle, 
char *bufferPtr,
UINT32 _bytesToWrite, 
UINT32  *_bytesWritten)
{
	int bytesWritten ;

	bytesWritten = (UINT32) write(smFileHandle->FileHandle, bufferPtr, (unsigned int) _bytesToWrite);
	if ( bytesWritten == -1 ) {
		return(NWSMTS_WRITE_ERROR);
	}
	*_bytesWritten = bytesWritten ;
	return(0);
}


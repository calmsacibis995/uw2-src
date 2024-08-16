#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/tsamsgs.C	1.7"

#include	<stdio.h>
#include        <nl_types.h>
#include	<tsaunix_msgs.h>
#include	"tsamsgs.h"

char *tsaMessages[TSA_END_ERROR_CODES+1] ;

char *defaultTsaMessages[] = {
     "Unknown Error Message",
     "Exclude Device Special Files",
     "Include Removable Filesystems",
     "respond: t_snd failed: t_look() returned:%d", /* T_SND_FAILED */
     "NFS",
     "UNIX Workstation",
     "Exclude major TSA resources",
     "Include major TSA resources",
     "Exclude directories(full path)",
     "Include directories(full path)",
     "Exclude files",
     "Include files",
     "Exclude path/files",
     "Include path/files",
     "/",
     "ERROR LOG",
     "SKIPPED DATA SETS",
     "ROOT:",
     ":",
     "Don't traverse subdirectories",
     "respond: t_snd failed: error %d", /* T_SND_ERROR */
     "<unknown>",
     "respond: t_rcv failed: error %d", /* T_RCV_ERROR */
     "\r\n ", 		/* CR_LF_INDENT */
     "\r\n\r\n", 		/* CR_LF_CR_LF */
     "\\", 		/* BACKSLASH */
     "readlink error for file %s, error 0x%x", 		/* READLINK_ERROR */
     "Warning! Invalid full paths data set format: error: %d",	/* INVALID_FULL_PATH_FORMAT */
     "Don't backup data", 		/* DONT_BACKUP_DATA_STREAMS */
     "Internal Error: Buffer state problem", /* BUFFER_STATE_PROBLEM */
     "respond: poll failed: errno:%d", 		/* POLL_FAILED */
     "respond: t_rcv failed: t_look() returned:%d", /* T_RCV_FAILED */
     "Permission denied",	/* TSA_ACCESS_DENIED */
     "Could not synchronize the TLI: t_error 0x%x",	/* T_SYNC_FAILED */
     "Buffer underflow. Not enough data to determine the field identifier",	/* TSA_BUFFER_UNDERFLOW */
     "",	/* TSA_CANT_ALLOC_DIR_HANDLE */
     "Could not set the transport for non blocking I/O: errno 0x%x",	/* T_NONBLOCKING_FAILED */
     "Could not create the directory",	/* TSA_CREATE_DIR_ENTRY_ERR */
     "Could not create the file",	/* TSA_CREATE_ERROR */
     "File already exists",	/* TSA_DATA_SET_ALREADY_EXISTS */
     "Dataset is excluded from scan",	/* TSA_DATA_SET_EXCLUDED */
     "",	/* TSA_DATA_SET_EXECUTE_ONLY */
     "",	/* TSA_DATA_SET_IN_USE */
     "",	/* TSA_DATA_SET_IS_OLDER */
     "Dataset is already open",	/* TSA_DATA_SET_IS_OPEN */
     "No such file or directory",	/* TSA_DATA_SET_NOT_FOUND */
     "Could not delete the file",	/* TSA_DELETE_ERR */
     "Invalid data. File or directory header was expected",	/* TSA_EXPECTING_HEADER */
     "Invalid data. File or directory trailer was expected",	/* TSA_EXPECTING_TRAILER */
     "",	/* TSA_GET_BIND_OBJ_NAME_ERR */
     "",	/* TSA_GET_DATA_STREAM_NAME_ERR */
     "",	/* TSA_GET_ENTRY_INDEX_ERR */
     "",	/* TSA_GET_NAME_SPACE_ENTRY_ERR */
     "",	/* TSA_GET_NAME_SPACE_SIZE_ERR */
     "dup() :error %d",	/* DUP_FAILED */
     "",	/* TSA_GET_VOL_NAME_SPACE_ERR */
     "Invalid connection",	/* TSA_INVALID_CONNECTION_HANDL */
     "Invalid data",	/* TSA_INVALID_DATA */
     "Invalid dataset handle",	/* TSA_INVALID_DATA_SET_HANDLE */
     "",	/* TSA_INVALID_DATA_SET_NAME */
     "Invalid dataset type",	/* TSA_INVALID_DATA_SET_TYPE */
     "",	/* TSA_INVALID_HANDLE */
     "Invalid message number",	/* TSA_INVALID_MESSAGE_NUMBER */
     "Invalid name space type",	/* TSA_INVALID_NAME_SPACE_TYPE */
     "",	/* TSA_INVALID_OBJECT_ID */
     "Invalid open mode",	/* TSA_INVALID_OPEN_MODE_TYPE */
     "Invalid parameter",	/* TSA_INVALID_PARAMETER */
     "Invalid path",	/* TSA_INVALID_PATH */
     "Invalid scan type",	/* TSA_INVALID_SCAN_TYPE */
     "",	/* TSA_INVALID_SEL_LIST_ENTRY */
     "Invalid selection type",	/* TSA_INVALID_SELECTION_TYPE */
     "invalid sequence number",	/* TSA_INVALID_SEQUENCE_NUMBER */
     "User or password incorrect. Login denied ",	/* TSA_LOGIN_DENIED */
     "",	/* TSA_LOGOUT_ERROR */
     "",	/* TSA_NAME_SP_PATH_NOT_UPDATED */
     "",	/* TSA_NOT_READY */
     "",	/* TSA_NO_CONNECTION */
     "No more data",	/* TSA_NO_MORE_DATA */
     "No more data sets",	/* TSA_NO_MORE_DATA_SETS */
     "No more data names",	/* TSA_NO_MORE_NAMES */
     "",	/* TSA_NO_SEARCH_PRIVILEGES */
     "",	/* TSA_NO_SUCH_PROPERTY */
     "Dataset open failed for restore",	/* TSA_OPEN_DATA_STREAM_ERR */
     "Could not open the file %s error 0x%x",	/* TSA_OPEN_ERROR */
     "Specified open mode is not used",	/* TSA_OPEN_MODE_TYPE_NOT_USED */
     "No space left on the device",	/* TSA_OUT_OF_DISK_SPACE */
     "Not enough memory",	/* TSA_OUT_OF_MEMORY */
     "Buffer overflow. Not enough data for the field in the current buffer",	/* TSA_OVERFLOW */
     "",	/* TSA_READ_EA_ERR */
     "Error reading a file",	/* TSA_READ_ERROR */
     "Specified filesystem or directory not found",	/* TSA_RESOURCE_NAME_NOT_FOUND */
     "Scan failed, probably due to an invalid path",	/* TSA_SCAN_ERROR */
     "Unable to scan file entry information",	/* TSA_SCAN_FILE_ENTRY_ERR */
     "Cannot alter resource list while scans are in progress",	/* TSA_SCAN_IN_PROGRESS */
     "Unable to scan name space specific information",	/* TSA_SCAN_NAME_SPACE_ERR */
     "Could not open the mounted filesystems table",/* OPEN_MNTTAB_FAILED */
     "Specified scan type is not used",	/* TSA_SCAN_TYPE_NOT_USED */
     "Specified selection type is not used",	/* TSA_SELECTION_TYPE_NOT_USED */
     "",	/* TSA_SET_FILE_INFO_ERR */
     "",	/* TSA_TRANSPORT_FAILURE */
     "",	/* TSA_TRANSPORT_PACKET_SIZE_ER */
     "",	/* TSA_TSA_NOT_FOUND */
     "function not supported",	/* TSA_UNSUPPORTED_FUNCTION */
     "Created a valid parent directory",	/* TSA_VALID_PARENT_HANDLE */
     "",	/* TSA_WRITE_EA_ERR */
     "",	/* TSA_WRITE_ERROR_SHORT */
     "write error",	/* TSA_WRITE_ERROR */
     "",	/* TSA_REDIRECT_TRANSPORT */
	 "", /* TSA_MAX_CONNECTIONS */
	 "Attempt to restore compressed data",	/* TSA_COMPRESSION_CONFLICT */
	 "An internal TSA error occured, see error log for details",
			/* TSA_INTERNAL_ERROR */
	 "usage: %s [ -l <log_file>]",
	 "No Data Streams",
	 "NONE",
	 "UNIX",
     0
};

#ifndef SINGLE_THREAD
char **messageTable = tsaMessages ;
#endif

/***
 *
 *  name	setMessageLocale - if message catalog is avaliable, reset the message
 *		table above as needed.
 *
 ***/

extern nl_catd	mCat ;

void
setMessageLocale()
    {
    register int	messageId = 0 ;

    //cannot i18n message number 0
    tsaMessages[0] = defaultTsaMessages[0] ;

    while ( defaultTsaMessages[++messageId] != NULL )
	tsaMessages[messageId] =
	catgets(mCat, MSG_SET, messageId, defaultTsaMessages[messageId]) ;
    }

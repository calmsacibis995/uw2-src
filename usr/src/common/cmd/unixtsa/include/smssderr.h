/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/smssderr.h	1.1"

#if !defined(_SMSSDERR_H_INCLUDED)
#define _SMSSDERR_H_INCLUDED
#define NWSMSD_ERROR_CODE(err)		   (0xFFFC0000L | err)
#define NWSMSD_BEGIN_ERROR_CODES		NWSMSD_ERROR_CODE(0xFFFF)

#define NWSMSD_ACCESS_DENIED			NWSMSD_ERROR_CODE(0xFFFF) /* An attempt to read or write when that access was not granted */
#define NWSMSD_BUFFER_INCORRECT			NWSMSD_ERROR_CODE(0xFFFE) /* Upon checking the buffer header on a read operation, the SDI determined that it is not was it was expecting */
#define NWSMSD_BUFFER_SIZE_INVALID		NWSMSD_ERROR_CODE(0xFFFD) /* bufferSize requested in not sufficient for application's and SDI's blockHeaderSize */
#define NWSMSD_CORRECTED_MEDIA_ERROR	NWSMSD_ERROR_CODE(0xFFFC) /* A media error was encountered, but was corrected by the driver */
#define NWSMSD_DEVICE_H_INVALID			NWSMSD_ERROR_CODE(0xFFFB) /* The deviceHandle passed is invalid */
#define NWSMSD_DEVICE_LIST_CHANGED		NWSMSD_ERROR_CODE(0xFFFA) /* While getting the list of devices available, a change in the available devices has occured.  Restart the NWSMSDListDevices */
#define NWSMSD_DEVICE_NOT_AVAIL			NWSMSD_ERROR_CODE(0xFFF9) /* Device exists but is not available (e.g. Already subjugated by another application in a non-sharing mode.) */
#define NWSMSD_DEVICE_NOT_EXIST			NWSMSD_ERROR_CODE(0xFFF8) /* Device does not exist */
#define NWSMSD_EARLY_WARNING			NWSMSD_ERROR_CODE(0xFFF7) /* Early warning was detected on the media */
#define NWSMSD_END_OF_MEDIA				NWSMSD_ERROR_CODE(0xFFF6) /* The end of media was detected */
#define NWSMSD_MEDIA_CHANGED			NWSMSD_ERROR_CODE(0xFFF5) /* The media has been unexpectedly changed since it was last identified */
#define NWSMSD_INTERNAL_ERROR			NWSMSD_ERROR_CODE(0xFFF4) /* An unexpected internal SDI error has occured */
#define NWSMSD_INVALID_CONNECTION		NWSMSD_ERROR_CODE(0xFFF3) /* An invalid connection number */
#define NWSMSD_INVALID_MESSAGE_NUMBER	NWSMSD_ERROR_CODE(0xFFF2) /* An invalid message number */
#define NWSMSD_INVALID_PARAMETER		NWSMSD_ERROR_CODE(0xFFF1) /* One or more of the paremeters is NULL or invalid */
#define NWSMSD_INVALID_SECTOR_COUNT		NWSMSD_ERROR_CODE(0xFFF0) /* NWSMSDWriteSector or NWSMSDReadSector requested fractional sector size */
#define NWSMSD_INVALID_SESSION_DATA_TYP NWSMSD_ERROR_CODE(0xFFEF) /* An invalid session data type was passed in a control block to NWSMSDWriteSessionData/NWSMSDReadSessionData */
#define NWSMSD_LOCATION_INVALID			NWSMSD_ERROR_CODE(0xFFEE) /* An invalid destination location was specified */ 
#define	NWSMSD_MEDIA_CORRECTED			NWSMSD_ERROR_CODE(0xFFED) /* Data from media is valid but error correction had to be used to read */
#define NWSMSD_MEDIA_FAILED				NWSMSD_ERROR_CODE(0xFFEC) /* Media Manager reported a failure when attempting the requested read or write operation */
#define NWSMSD_MEDIA_H_INVALID			NWSMSD_ERROR_CODE(0xFFEB) /* The mediaHandle passed is invalid */
#define NWSMSD_MEDIA_LABELED			NWSMSD_ERROR_CODE(0xFFEA) /* Attempt to label an already labeled media */
#define NWSMSD_MEDIA_LIST_CHANGED		NWSMSD_ERROR_CODE(0xFFE9) /* While getting the list of media available, a change in the available media has occured.  Restart the NWSMSDListMedia */
#define NWSMSD_FILE_MARK_DETECTED		NWSMSD_ERROR_CODE(0xFFE8) /* A file mark was encountered while reading media. This is not fatal and data in buffer up to number of sectors read is valid */
#define NWSMSD_SET_MARK_DETECTED		NWSMSD_ERROR_CODE(0xFFE7) /* A set mark was encountered while reading media. This is not fatal and data in buffer up to number of sectors read is valid */
#define NWSMSD_MEDIA_MOUNTED			NWSMSD_ERROR_CODE(0xFFE6) /* An attempt to perform an action (e.g. NWSMSDMoveMedia) on a mounted media which is illegal */
#define NWSMSD_MEDIA_NOT_AVAIL			NWSMSD_ERROR_CODE(0xFFE5) /* Media exists but is not available (e.g. Already subjugated by another application in a non-sharing mode.) */
#define NWSMSD_MEDIA_NOT_EXIST			NWSMSD_ERROR_CODE(0xFFE4) /* Media does not exist */
#define NWSMSD_MEDIA_NOT_LABELED		NWSMSD_ERROR_CODE(0xFFE3) /* Media does not have a valid SMS label */
#define NWSMSD_MEDIA_NOT_MOUNTED		NWSMSD_ERROR_CODE(0xFFE2) /* Media has not been mounted using the NWSMSDMountMedia function */
#define NWSMSD_NONSMS_COMPLIANT			NWSMSD_ERROR_CODE(0xFFE1) /* The requested operation , if completed, would have resulted in creating media which would not have been SMS compliant */
#define NWSMSD_NO_ALERT_CONNECTION		NWSMSD_ERROR_CODE(0xFFE0) /* No more connections available for alertRoutines */
#define NWSMSD_NO_BUFFER_MEMORY			NWSMSD_ERROR_CODE(0xFFDF) /* No memory available for any size block */
#define NWSMSD_NO_READ_MODE				NWSMSD_ERROR_CODE(0xFFDE) /* The application has requested an operation which requires READ access mode which has note been granted */
#define NWSMSD_NO_SESSION_DATA			NWSMSD_ERROR_CODE(0xFFDD) /* A NWSMSDReadSessionData call was made with no data to read (i.e. media is at the end of the session [including session and media index].) */
#define NWSMSD_NO_WRITE_MODE			NWSMSD_ERROR_CODE(0xFFDC) /* The application has requested an operation which requires WRITE access mode which has note been granted */
#define NWSMSD_OBJECT_SHARED			NWSMSD_ERROR_CODE(0xFFDB) /* The request for non-share subjugation failed because the device or media is already subjugated in share mode by another SMS application */
#define NWSMSD_OUT_OF_MEMORY			NWSMSD_ERROR_CODE(0xFFDA) /* SDI memory allocation failed */
#define NWSMSD_NOT_SMS_MEDIA			NWSMSD_ERROR_CODE(0xFFD9) /* The partition is not an SMS media or partition */
#define NWSMSD_POSITION_INVALID			NWSMSD_ERROR_CODE(0xFFD8) /* The position requested is not valid. e.g. A relative mode was specified but either the expected sessionNumber or the  expected blockNumber is NULL */
#define NWSMSD_POSITION_NOT_FOUND		NWSMSD_ERROR_CODE(0xFFD7) /* The postion requested was not located on the media */
#define NWSMSD_SESSION_H_INVALID		NWSMSD_ERROR_CODE(0xFFD6) /* The requested session handle is not valid either because the session was not opened or the media is no longer positioned within the requested session */
#define NWSMSD_SESSION_NOT_FOUND		NWSMSD_ERROR_CODE(0xFFD5) /* The specified session header was not found on the media, or if a location was not specified, there is not a session header which matchs the specified description */
#define NWSMSD_TIME_OUT					NWSMSD_ERROR_CODE(0xFFD4) /* The operation timed-out */
#define NWSMSD_TRANSFER_BUFFER_OVERFLOW	NWSMSD_ERROR_CODE(0xFFD3) /* The transfer buffer passed was not large enough to hold the entire transfer buffer from the media. SDI returned as much of the transfer buffer as would fit */
#define NWSMSD_UNABLE_TO_CANCEL_TRANSFR	NWSMSD_ERROR_CODE(0xFFD2) /* One or more of the requested tranfer requests was no able to be canceled. SDI will complete those requests normally */
#define NWSMSD_UNSUPPORTED_FUNCTION		NWSMSD_ERROR_CODE(0xFFD1) /* The function or specific parameter passed are not supported by the device driver */
#define NWSMSD_UNSUPPORTED_SERVICE		NWSMSD_ERROR_CODE(0xFFD0) /* The function or specific parameter passed are not supported by this version of SDI */
#define NWSMSD_OS_ERROR					NWSMSD_ERROR_CODE(0xFFCF) /* The Operating System returned an error */
#define NWSMSD_DRIVER_ERROR				NWSMSD_ERROR_CODE(0xFFCE) /* The device driver or device itself returned an error */
#define NWSMSD_IO_ABORT_SUCCESSFUL		NWSMSD_ERROR_CODE(0xFFCD) /* An I/O function was successfully aborted */
#define NWSMSD_IO_ABORT_DOTO_PREV_ERROR	NWSMSD_ERROR_CODE(0xFFCC) /* An I/O function was aborted due to a previous error */
#define NWSMSD_DRIVER_UNSUPPORT_FUNC	NWSMSD_ERROR_CODE(0xFFCB) /* An attempt to use a function which is not supported by the device driver */
#define NWSMSD_INVALID_DATA				NWSMSD_ERROR_CODE(0xFFCA) /* Invalid data was encountered on the media */
#define NWSMSD_MEDIA_WRITE_PROTECTED	NWSMSD_ERROR_CODE(0xFFC9) /* The media is write protected */
#define NWSMSD_UNKNOWN_DRIVER_ERROR		NWSMSD_ERROR_CODE(0xFFC8) /* The device driver or device itself returned an unknown error */
#define NWSMSD_HEADER_TOO_LARGE         NWSMSD_ERROR_CODE(0xFFC7) /* The header is too large to fit in a physical sector */
#define NWSMSD_MEDIA_NOT_FORMATED       NWSMSD_ERROR_CODE(0xFFC6) /* The media is not formatted */
#define NWSMSD_BLANK_MEDIA              NWSMSD_ERROR_CODE(0xFFC5) /* The media is blank */
#define NWSMSD_BEGINNING_OF_MEDIA       NWSMSD_ERROR_CODE(0xFFC4) /* The beginning of media was detected */
#define NWSMSD_SECTOR_SIZE_ERROR        NWSMSD_ERROR_CODE(0xFFC3) /* The sector size is not valid */
#define NWSMSD_MEDIA_NOT_APPENDABLE     NWSMSD_ERROR_CODE(0xFFC2) /* The media was written in old media format and is therefore not appendable */
#define NWSMSD_MEDIA_UNAVAILABLE        NWSMSD_ERROR_CODE(0xFFC1) /* The media is unavailable for use by an SMS application */


#define NWSMSD_END_ERROR_CODES			NWSMSD_ERROR_CODE(0xFFC0)
#endif


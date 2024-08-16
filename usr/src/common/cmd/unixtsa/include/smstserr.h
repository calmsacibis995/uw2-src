#ident	"%W%"
/*	
****************************************************************************
*
* Program Name:      Storage Management Services TSAPI Error Codes
*
* modname: smstserr.h     version: 1.5     date: 01/19/94
* PVCS          $Revision$    $date$
*
* Date Created:      March 2, 1992
*
* Version:           3.2
*
* Date Modified: 
*
* Modifications between 3.11c and 3.20a:
*
*					The following codes were added
*					NWSM_UNSUPPORTED_FUNCTION
*
*					The following codes were deleted
*					NWSM_DATA_REQUESTOR_NOT_FOUND
*					NWSM_DATA_REQUESTOR_NOT_RGSTRD
*					NWSM_INVALID_SMSP_FUNCTION
*					NWSM_NO_DATA_REQUESTORS_FOUND
*
*					The following codes were changed:
*		NWSM_                             -> NWSMTS_
*		NWSMTS_EXPECTING_DATA_STREAM_SIZE -> NWSMTS_EXPECTING_DATA_STREAM_SZ
*		NWSMTS_EXPIRED_PASSWORD_NO_GRACE  -> NWSMTS_EXPIRED_PASSWORD_NO_GRAC
*		NWSMTS_INVALID_CONNECTION_HANDLE  -> NWSMTS_INVALID_CONNECTION_HANDL
*		NWSMTS_INVALID_RETURN_NAME_SPACE  -> NWSMTS_INVALID_RETURN_NAME_SPAC
*		NWSMTS_NO_RESOURCE_READ_PRIVILEGE -> NWSMTS_NO_RESOURCE_READ_PRIVILE
*		NWSMTS_TRANSPORT_PACKET_SIZE_ERR  -> NWSMTS_TRANSPORT_PACKET_SIZE_ER
*
* Comments:          Error numbers for TSAPIs
*                    Prefix:   FFFD
*
* (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
*
* No part of this file may be duplicated, revised, translated, localized or
* modified in any manner or compiled, linked or uploaded or downloaded to or
* from any computer system without the prior written consent of Novell, Inc.
*
****************************************************************************
*/

#ifndef _SMSTSERR_H_INCLUDED      /* smstserr.h header latch */
#define _SMSTSERR_H_INCLUDED


#define NWSMTS_AUX_ERROR_RANGE            0xFFFA0000L

#define NWSMTS_AUX_ERROR_CODE(err)       (0xFFFA0000L | err)
#define NWSMTS_ERROR_CODE(err)           (0xFFFD0000L | err)
#define NWSMTS_BEGIN_ERROR_CODES          NWSMTS_ERROR_CODE(0xFFFF)

#define NWSMTS_ACCESS_DENIED              NWSMTS_ERROR_CODE(0xFFFF) 
   /* Invalid user Name or authentication                     */

#define NWSMTS_BINDERY_OBJECT_NAME_ERR    NWSMTS_ERROR_CODE(0xFFFE) 
   /* Unable to get bindery object name                       */

#define NWSMTS_BUFFER_UNDERFLOW           NWSMTS_ERROR_CODE(0xFFFD) 
   /* Buffer underflow, unable to get entire field            */

#define NWSMTS_CANT_ALLOC_DIR_HANDLE      NWSMTS_ERROR_CODE(0xFFFC) 
   /* Cannot allocate a directory handle                      */

#define NWSMTS_CLOSE_BINDERY_ERROR        NWSMTS_ERROR_CODE(0xFFFB) 
   /* Cannot close the bindery                                */

#define NWSMTS_CREATE_DIR_ENTRY_ERR       NWSMTS_ERROR_CODE(0xFFFA) 
   /* Cannot create directory entry                           */

#define NWSMTS_CREATE_ERROR               NWSMTS_ERROR_CODE(0xFFF9) 
   /* Cannot create a file                                    */

#define NWSMTS_DATA_SET_ALREADY_EXISTS    NWSMTS_ERROR_CODE(0xFFF8) 
   /* Data set name is already in use                         */

#define NWSMTS_DATA_SET_EXCLUDED          NWSMTS_ERROR_CODE(0xFFF7) 
   /* The Data Set was excluded by the Selection List         */

#define NWSMTS_DATA_SET_EXECUTE_ONLY      NWSMTS_ERROR_CODE(0xFFF6) 
   /* Cannot open an execute only file                        */

#define NWSMTS_DATA_SET_IN_USE            NWSMTS_ERROR_CODE(0xFFF5) 
   /* Data set is currently in use and cannot be accessed     */

#define NWSMTS_DATA_SET_IS_OLDER          NWSMTS_ERROR_CODE(0xFFF4) 
   /* The target data set is newer the one on the media, the  */
   /* data set will not be restored                           */

#define NWSMTS_DATA_SET_IS_OPEN           NWSMTS_ERROR_CODE(0xFFF3) 
   /* Attempt to open a data set when one is open or to alter */
   /* a scan when a data set is open                          */

#define NWSMTS_DATA_SET_NOT_FOUND         NWSMTS_ERROR_CODE(0xFFF2) 
   /* No data sets found                                      */

#define NWSMTS_DELETE_ERR                 NWSMTS_ERROR_CODE(0xFFF1) 
   /* Error deleting data set                                 */

#define NWSMTS_EXPECTING_HEADER           NWSMTS_ERROR_CODE(0xFFF0) 
   /* Processing a Record/Subrecord and unable to find the    */
   /* HEADER Field                                            */

#define NWSMTS_EXPECTING_TRAILER          NWSMTS_ERROR_CODE(0xFFEF) 
   /* Processing a Record/Subrecord and unable to find the    */
   /* TRAILER Field                                           */

#define NWSMTS_GET_BIND_OBJ_NAME_ERR      NWSMTS_ERROR_CODE(0xFFEE) 
   /* Unable to get bindery object name                       */

#define NWSMTS_GET_DATA_STREAM_NAME_ERR   NWSMTS_ERROR_CODE(0xFFED) 
   /* Unable to get data stream name                          */

#define NWSMTS_GET_ENTRY_INDEX_ERR        NWSMTS_ERROR_CODE(0xFFEC) 
   /* Unable to get the entry index                           */

#define NWSMTS_GET_NAME_SPACE_ENTRY_ERR   NWSMTS_ERROR_CODE(0xFFEB) 
   /* Unable to get name space entry name                     */

#define NWSMTS_GET_NAME_SPACE_SIZE_ERR    NWSMTS_ERROR_CODE(0xFFEA) 
   /* Unable to get name space size information               */

#define NWSMTS_GET_SERVER_INFO_ERR        NWSMTS_ERROR_CODE(0xFFE9) 
   /* Unable to get file server information                   */

#define NWSMTS_GET_VOL_NAME_SPACE_ERR     NWSMTS_ERROR_CODE(0xFFE8) 
   /* Unable to get volume supported name space information   */

#define NWSMTS_INVALID_CONNECTION_HANDL   NWSMTS_ERROR_CODE(0xFFE7) 
   /* An invalid connection handle was passed                 */

#define NWSMTS_INVALID_DATA               NWSMTS_ERROR_CODE(0xFFE6) 
   /* Invalid Data Set data                                   */

#define NWSMTS_INVALID_DATA_SET_HANDLE    NWSMTS_ERROR_CODE(0xFFE5) 
   /* Data set handle is invalid                              */

#define NWSMTS_INVALID_DATA_SET_NAME      NWSMTS_ERROR_CODE(0xFFE4) 
   /* Data set name is invalid                                */

#define NWSMTS_INVALID_DATA_SET_TYPE      NWSMTS_ERROR_CODE(0xFFE3) 
   /* Data set type is invalid                                */

#define NWSMTS_INVALID_HANDLE             NWSMTS_ERROR_CODE(0xFFE2) 
   /* Handle is tagged INVALID or ptr is NIL                  */

#define NWSMTS_INVALID_MESSAGE_NUMBER     NWSMTS_ERROR_CODE(0xFFE1) 
   /* Message number is invalid                               */

#define NWSMTS_INVALID_NAME_SPACE_TYPE    NWSMTS_ERROR_CODE(0xFFE0) 
   /* Name space type does not exist or is invalid            */

#define NWSMTS_INVALID_OBJECT_ID          NWSMTS_ERROR_CODE(0xFFDF) 
   /* The object id/name backed up doesn't match the current  */
   /* object id/name                                          */

#define NWSMTS_INVALID_OPEN_MODE_TYPE     NWSMTS_ERROR_CODE(0xFFDE) 
   /* Open mode option is out of range (0xi.e. <0 or >23)     */

#define NWSMTS_INVALID_PARAMETER          NWSMTS_ERROR_CODE(0xFFDD) 
   /* One or more of the paremeters is NULL or invalid        */

#define NWSMTS_INVALID_PATH               NWSMTS_ERROR_CODE(0xFFDC) 
   /* An invalid path was used                                */

#define NWSMTS_INVALID_SCAN_TYPE          NWSMTS_ERROR_CODE(0xFFDB) 
   /* Scan type is out of range (0xi.e. <0 or >31)            */

#define NWSMTS_INVALID_SEL_LIST_ENTRY     NWSMTS_ERROR_CODE(0xFFDA) 
   /* An invalid selection list entry was passed              */

#define NWSMTS_INVALID_SELECTION_TYPE     NWSMTS_ERROR_CODE(0xFFD9) 
   /* Selection type is out of range (0xi.e. <0 or >31)       */

#define NWSMTS_INVALID_SEQUENCE_NUMBER    NWSMTS_ERROR_CODE(0xFFD8) 
   /* The sequence number is invalid                          */

#define NWSMTS_LOGIN_DENIED               NWSMTS_ERROR_CODE(0xFFD7) 
   /* Login Denied                                            */

#define NWSMTS_LOGOUT_ERROR               NWSMTS_ERROR_CODE(0xFFD6) 
   /* Unable to logout                                        */

#define NWSMTS_NAME_SP_PATH_NOT_UPDATED   NWSMTS_ERROR_CODE(0xFFD5) 
   /* The name space path has not been updated                */

#define NWSMTS_NOT_READY                  NWSMTS_ERROR_CODE(0xFFD4) 
   /* The specified server is unable to service the request   */
   /* at this time                                            */

#define NWSMTS_NO_CONNECTION              NWSMTS_ERROR_CODE(0xFFD3) 
   /* Connection is invalid or does not exist                 */

#define NWSMTS_NO_MORE_DATA               NWSMTS_ERROR_CODE(0xFFD2) 
   /* No more data exists                                     */

#define NWSMTS_NO_MORE_DATA_SETS          NWSMTS_ERROR_CODE(0xFFD1) 
   /* There are no more data sets to be scanned               */

#define NWSMTS_NO_MORE_NAMES              NWSMTS_ERROR_CODE(0xFFD0) 
   /* No more entries in list or nameSpace does not exist     */

#define NWSMTS_NO_SEARCH_PRIVILEGES       NWSMTS_ERROR_CODE(0xFFCF) 
   /* No search privilege on client service                   */

#define NWSMTS_NO_SUCH_PROPERTY           NWSMTS_ERROR_CODE(0xFFCE) 
   /* No Such Property                                        */

#define NWSMTS_OPEN_DATA_STREAM_ERR       NWSMTS_ERROR_CODE(0xFFCD) 
   /* Unable to open a data stream                            */

#define NWSMTS_OPEN_ERROR                 NWSMTS_ERROR_CODE(0xFFCC) 
   /* Can't open a file                                       */

#define NWSMTS_OPEN_MODE_TYPE_NOT_USED    NWSMTS_ERROR_CODE(0xFFCB) 
   /* Open mode option is not used                            */

#define NWSMTS_OUT_OF_DISK_SPACE          NWSMTS_ERROR_CODE(0xFFCA) 
   /* Can't restore, out of disk space                        */

#define NWSMTS_OUT_OF_MEMORY              NWSMTS_ERROR_CODE(0xFFC9) 
   /* Server out of memory or memory allocation failed        */

#define NWSMTS_OVERFLOW                   NWSMTS_ERROR_CODE(0xFFC8) 
   /* A UINT64 value has overflowed                           */

#define NWSMTS_READ_EA_ERR                NWSMTS_ERROR_CODE(0xFFC7) 
   /* Unable to read extended attributes                      */

#define NWSMTS_READ_ERROR                 NWSMTS_ERROR_CODE(0xFFC6) 
   /* Error reading a file                                    */

#define NWSMTS_RESOURCE_NAME_NOT_FOUND    NWSMTS_ERROR_CODE(0xFFC5) 
   /* No resource name is found or all resource names have    */
   /* been found                                              */

#define NWSMTS_SCAN_ERROR                 NWSMTS_ERROR_CODE(0xFFC4) 
   /* Scan failed, probably due to an invalid path            */

#define NWSMTS_SCAN_FILE_ENTRY_ERR        NWSMTS_ERROR_CODE(0xFFC3) 
   /* Unable to scan file entry information                   */

#define NWSMTS_SCAN_IN_PROGRESS           NWSMTS_ERROR_CODE(0xFFC2) 
   /* Cannot alter resource list while scans are in progress  */

#define NWSMTS_SCAN_NAME_SPACE_ERR        NWSMTS_ERROR_CODE(0xFFC1) 
   /* Unable to scan name space specific information          */

#define NWSMTS_SCAN_TRUSTEE_ERR           NWSMTS_ERROR_CODE(0xFFC0) 
   /* Unable to scan for trustees                             */

#define NWSMTS_SCAN_TYPE_NOT_USED         NWSMTS_ERROR_CODE(0xFFBF) 
   /* Scan type is not used                                   */

#define NWSMTS_SELECTION_TYPE_NOT_USED    NWSMTS_ERROR_CODE(0xFFBE) 
   /* Selection type is not used                              */

#define NWSMTS_SET_FILE_INFO_ERR          NWSMTS_ERROR_CODE(0xFFBD) 
   /* Unable to set file information                          */

#define NWSMTS_TRANSPORT_FAILURE          NWSMTS_ERROR_CODE(0xFFBC) 
   /* The transport mechanism has failed                      */

#define NWSMTS_TRANSPORT_PACKET_SIZE_ER   NWSMTS_ERROR_CODE(0xFFBB) 
   /* The read/write request exceeds 128K                     */

#define NWSMTS_TSA_NOT_FOUND              NWSMTS_ERROR_CODE(0xFFBA) 
   /* Invalid or inactive TSA specified                       */

#define NWSMTS_UNSUPPORTED_FUNCTION       NWSMTS_ERROR_CODE(0xFFB9) 
   /* The requested function is not supported by this TSA     */

#define NWSMTS_VALID_PARENT_HANDLE        NWSMTS_ERROR_CODE(0xFFB8) 
   /* A valid parent handle is created                        */

#define NWSMTS_WRITE_EA_ERR               NWSMTS_ERROR_CODE(0xFFB7) 
   /* Unable to write extended attribute(0xs)                 */

#define NWSMTS_WRITE_ERROR_SHORT          NWSMTS_ERROR_CODE(0xFFB6) 
   /* Error writing to a file, didn't write full request      */

#define NWSMTS_WRITE_ERROR                NWSMTS_ERROR_CODE(0xFFB5) 
   /* Error writing to a file                                 */

#define NWSMTS_REDIRECT_TRANSPORT         NWSMTS_ERROR_CODE(0xFFB4) 
   /* Indicates reconnection requirement                      */

#define NWSMTS_MAX_CONNECTIONS            NWSMTS_ERROR_CODE(0xFFB3) 
   /* All available connections to TSA are in use.            */

#define NWSMTS_COMPRESSION_CONFLICT       NWSMTS_ERROR_CODE(0xFFB2)
   /* Attempt to put compressed data on non-compressed volume      */
	
#define NWSMTS_INTERNAL_ERROR             NWSMTS_ERROR_CODE(0xFFB1)
   /* An internal TSA error occured, see error log for details      */

#define NWSMTS_END_ERROR_CODES            NWSMTS_ERROR_CODE(0xFFB0)


#endif                            /* smstserr.h header latch */
/***************************************************************************/

/* START BLOCK COMMENT
**	   #define NWSMTS_ACCT_LIMIT_EXCEEDED      NWSMTS_ERROR_CODE(0xFFFE) 
#* Account Credit Limit Exceeded #/
**	   #define NWSMTS_ALREADY_CONNECTED        NWSMTS_ERROR_CODE(0xFFFD) 
#* SME is currently connected to a TSA #/
**	   #define NWSMTS_DATA_SET_DETACHED        NWSMTS_ERROR_CODE(0xFFF5) 
#* Data set is detached #/
**	   #define NWSMTS_DATA_SET_LOCKED          NWSMTS_ERROR_CODE(0xFFF0) 
#* Read with data set locked #/
**	   #define NWSMTS_DATA_SET_READ_ONLY       NWSMTS_ERROR_CODE(0xFFEE) 
#* Data set is read only #/
**	   #define NWSMTS_DIRECTORY_LOCKED         NWSMTS_ERROR_CODE(0xFFEC) 
#* Directory Locked #/
**	   #define NWSMTS_DISK_MAP_ERROR           NWSMTS_ERROR_CODE(0xFFEB) 
#* Disk mapping error or volume does not exist #/
**	   #define NWSMTS_EAS_NOT_SUPPORTED        NWSMTS_ERROR_CODE(0xFFEA) 
#* Extended Attributes are not supported, this information will not be 
   restored #/
**	   #define NWSMTS_EXCEED_MAX_LOGINS        NWSMTS_ERROR_CODE(0xFFE9) 
#* Maximum Logins Exceeded - Login Denied #/
**	   #define NWSMTS_EXPECTING_BEGIN          NWSMTS_ERROR_CODE(0xFFE8) 
#* Processing a Record/Subrecord and didn't find the BEGIN Field #/
**	   #define NWSMTS_EXPECTING_DATA_STREAM_SZ NWSMTS_ERROR_CODE(0xFFE7) 
#* Processing a Data Stream Header and didn't find the Size Field #/
**	   #define NWSMTS_EXPECTING_END            NWSMTS_ERROR_CODE(0xFFE6) 
#* Processing a Record/Subrecord and didn't find the END Field #/
**	   #define NWSMTS_EXPIRED_PASSWORD         NWSMTS_ERROR_CODE(0xFFE3) 
#* Old Password #/
**	   #define NWSMTS_EXPIRED_PASSWORD_NO_GRAC NWSMTS_ERROR_CODE(0xFFE2) 
#* Bad Password - No grace period #/
**	   #define NWSMTS_HARD_FAILURE             NWSMTS_ERROR_CODE(0xFFDA) 
#* Hard Failure, Failure #/
**	   #define NWSMTS_ILLEGAL_WILD_CARD        NWSMTS_ERROR_CODE(0xFFD9) 
#* Illegal Wild Card #/
**	   #define NWSMTS_INTRUDER_DETECTION_LOCK  NWSMTS_ERROR_CODE(0xFFD8) 
#* Intruder Detection Lock #/
**	   #define NWSMTS_INVALID_BINDERY_SECURITY NWSMTS_ERROR_CODE(0xFFD7) 
#* Invalid Bindery Security #/
**	   #define NWSMTS_INVALID_MODE             NWSMTS_ERROR_CODE(0xFFCF) 
#* Open Mode setting is invalid #/
**	   #define NWSMTS_INVALID_RETURN_NAME_SPAC NWSMTS_ERROR_CODE(0xFFC9) 
#* The returned name space type is invalid #/
**	   #define NWSMTS_INVALID_SCAN_PATTERN     NWSMTS_ERROR_CODE(0xFFC8) 
#* Invalid scan pattern #/
**	   #define NWSMTS_INVALID_SET_FLAG         NWSMTS_ERROR_CODE(0xFFC4) 
#* Set flag value is invalid #/
**	   #define NWSMTS_INVALID_STATION_NUMBER   NWSMTS_ERROR_CODE(0xFFC3) 
#* Station number is invalid #/
**	   #define NWSMTS_INVALID_TS_TYPE          NWSMTS_ERROR_CODE(0xFFC2) 
#* An invalid Transaction Set Type was passed to the TSA #/
**	   #define NWSMTS_IO_ERROR                 NWSMTS_ERROR_CODE(0xFFC1) 
#* IO error #/
**	   #define NWSMTS_LOCK_ERROR               NWSMTS_ERROR_CODE(0xFFC0) 
#* Lock Error #/
**	   #define NWSMTS_NEW_DATA_SET_NAME_EXISTS NWSMTS_ERROR_CODE(0xFFBC) 
#* The specified new data set name already exists #/
**	   #define NWSMTS_NO_CONNECTION_TO_RELEASE NWSMTS_ERROR_CODE(0xFFB9) 
#* SME is not connected to a TSA #/
**	   #define NWSMTS_NO_LOGIN_NO_ACCT_BALANCE NWSMTS_ERROR_CODE(0xFFB8) 
#* Login Denied No account Balance #/
**	   #define NWSMTS_NO_MODIFY_PRIVILEGES     NWSMTS_ERROR_CODE(0xFFB7) 
#* No modify privileges #/
**	   #define NWSMTS_NO_PROP_DELETE_PRIVILEGE NWSMTS_ERROR_CODE(0xFFB3) 
#* No Property Delete Privilege #/
**	   #define NWSMTS_NO_READ_PRIVILEGES       NWSMTS_ERROR_CODE(0xFFB2) 
#* No read privileges #/
**	   #define NWSMTS_NO_RENAME_PRIVILEGES     NWSMTS_ERROR_CODE(0xFFB1) 
#* No data set name rename privileges #/
**	   #define NWSMTS_NO_RESOURCE_READ_PRIVILE NWSMTS_ERROR_CODE(0xFFB0) 
#* No Resource Read Privilege #/
**	   #define NWSMTS_NO_SUCH_OBJECT           NWSMTS_ERROR_CODE(0xFFAE) 
#* No Such Object #/
**	   #define NWSMTS_NO_SUCH_SEGMENT          NWSMTS_ERROR_CODE(0xFFAC) 
#* No Such Segment SPX Terminated Poorly #/
**	   #define NWSMTS_NO_WILD_CARDS_ALLOWED    NWSMTS_ERROR_CODE(0xFFAB) 
#* Wild cards exist in data set name #/
**	   #define NWSMTS_OUT_OF_SPACE             NWSMTS_ERROR_CODE(0xFFA5) 
#* Not enough space to restore the data set #/
**	   #define NWSMTS_PASSWORD_NOT_UNIQUE      NWSMTS_ERROR_CODE(0xFFA3) 
#* Password Is Not Unique #/
**	   #define NWSMTS_PROPERTY_EXISTS          NWSMTS_ERROR_CODE(0xFFA2) 
#* Property Already Exists #/
**	   #define NWSMTS_RENAMING_ACROSS_VOLUME   NWSMTS_ERROR_CODE(0xFF9F) 
#* Renaming not allowed across volumes #/
**	   #define NWSMTS_SET_DIR_ENTRY_ERR        NWSMTS_ERROR_CODE(0xFF96) 
#* Unable to set the directory entry information #/
**	   #define NWSMTS_SET_FILE_ENTRY_ERR       NWSMTS_ERROR_CODE(0xFF95) 
#* Unable to set the file entry information #/
**	   #define NWSMTS_SPX_CONN_TABLE_FULL      NWSMTS_ERROR_CODE(0xFF93) 
#* Invalid Name SPX Connection Table Full #/
**	   #define NWSMTS_TARGET_NAME_NOT_FOUND    NWSMTS_ERROR_CODE(0xFF92) 
#* No client name found #/
**	   #define NWSMTS_TARGET_SERVICE_NOT_FOUND NWSMTS_ERROR_CODE(0xFF91) 
#* Invalid or inactive server/services specified #/
**	   #define NWSMTS_TERMINATE_BACKUP         NWSMTS_ERROR_CODE(0xFF90) 
#* The Backup cannot continue #/
**	   #define NWSMTS_TIME_OUT_FAILURE         NWSMTS_ERROR_CODE(0xFF8F) 
#* Connection time out failure #/
**	   #define NWSMTS_TSA_ALREADY_IN_USE       NWSMTS_ERROR_CODE(0xFF8C) 
#* The TSA is being used by another engine #/
**	   #define NWSMTS_UNAUTHORIZED_LOGIN_TIME  NWSMTS_ERROR_CODE(0xFF8A) 
#* Unauthorized Login Time #/
**	   #define NWSMTS_UNAUTHORIZED_LOG_STATION NWSMTS_ERROR_CODE(0xFF89) 
#* Unauthorized Login Station #/
**	   #define NWSMTS_WRITE_TO_GROUP           NWSMTS_ERROR_CODE(0xFF83) 
#* Write Property to Group error #/

END BLOCK COMMENT */
/*********************************************************************/

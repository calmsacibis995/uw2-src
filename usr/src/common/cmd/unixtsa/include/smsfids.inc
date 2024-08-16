#ident	"@(#)unixtsa:common/cmd/unixtsa/include/smsfids.inc	1.3"
/*  Available Standard FIDs: */
/*    Small Variable Size: (0x00)     0x30 - 0x3F */
/*    Small Fixed Size:    1-Byte     0x70 */
/*                         2-Byte     0x61, 0x71 */
/*                         4-Byte     ---- */
/*                         8-Byte     0x43, 0x53, 0x63, 0x73 */
/*                         16-Byte    ---- */
/*                         32-Byte    0x55, 0x65, 0x75 */
/*                         64-Byte    0x46, 0x56, 0x66, 0x76 */
/*                         128-Byte   0x47, 0x57, 0x67, 0x77 */
/*                         256-Byte   0x48, 0x58, 0x68, 0x78 */
/*                         512-Byte   0x49, 0x59, 0x69, 0x79 */
/*                         1K-Byte    0x4A, 0x5A, 0x6A, 0x7A */
/*                         2K-Byte    0x4B, 0x5B, 0x6B, 0x7B */
/*                         4K-Byte    0x4C, 0x5C, 0x6C, 0x7C */
/*                         8K-Byte    0x4D, 0x5D, 0x6D, 0x7D */
/*                         16K-Byte   0x4E, 0x5E, 0x6E, 0x7E */
/*                         32K-Byte   0x4F, 0x5F, 0x6F, 0x7F */

/*    Short Variable Size: (0x80)     0x08 - 0x3F */
/*    Short Fixed Size:    1-Byte     0x40, 0x50, 0x60, 0x70 */
/*                         2-Byte     0x41, 0x51, 0x61, 0x71 */
/*                         4-Byte     0x62, 0x72 */
/*                         8-Byte     0x43, 0x53, 0x63, 0x73 */
/*                         16-Byte    0x44, 0x54, 0x64, 0x74 */
/*                         32-Byte    0x45, 0x55, 0x65, 0x75 */
/*                         64-Byte    0x46, 0x56, 0x66, 0x76 */
/*                         128-Byte   0x47, 0x57, 0x67, 0x77 */
/*                         256-Byte   0x48, 0x58, 0x68, 0x78 */
/*                         512-Byte   0x49, 0x59, 0x69, 0x79 */
/*                         1K-Byte    0x4A, 0x5A, 0x6A, 0x7A */
/*                         2K-Byte    0x4B, 0x5B, 0x6B, 0x7B */
/*                         4K-Byte    0x4C, 0x5C, 0x6C, 0x7C */
/*                         8K-Byte    0x4D, 0x5D, 0x6D, 0x7D */
/*                         16K-Byte   0x4E, 0x5E, 0x6E, 0x7E */
/*                         32K-Byte   0x4F, 0x5F, 0x6F, 0x7F */

/*    Long Variable Size: (0x80)      0x8024 - 0xEFFE */
/*    Long Fixed Size:     1-Byte     0xF003 - 0xF0FF */
/*                         2-Byte     0xF105 - 0xF1FF */
/*                         4-Byte     0xF211 - 0xF2FF */
/*                         8-Byte     0xF301 - 0xF3FF */
/*                         16-Byte    0xF404 - 0xF4FF */
/*                         32-Byte    0xF500 - 0xF5FF */
/*                         64-Byte    0xF600 - 0xF6FF */
/*                         128-Byte   0xF700 - 0xF7FF */
/*                         256-Byte   0xF800 - 0xF8FF */
/*                         512-Byte   0xF900 - 0xF9FF */
/*                         1K-Byte    0xFA00 - 0xFAFF */
/*                         2K-Byte    0xFB00 - 0xFBFF */
/*                         4K-Byte    0xFC00 - 0xFCFF */
/*                         8K-Byte    0xFD00 - 0xFDFF */
/*                         16K-Byte   0xFE00 - 0xFEFF */
/*                         32K-Byte   0xFF00 - 0xFFFF */


/*  Available Novell FIDs:   */
/*    Short Variable Size: (0x81)     0x00 - 0x12 */
/*    Short Fixed Size:    1-Byte     0x40, 0x50, 0x60, 0x70 */
/*                         2-Byte     0x41, 0x51, 0x61, 0x71 */
/*                         4-Byte     0x42, 0x62, 0x72 */
/*                         8-Byte     0x43, 0x53, 0x63, 0x73 */
/*                         16-Byte    0x44, 0x54, 0x64, 0x74 */
/*                         32-Byte    0x45, 0x55, 0x65, 0x75 */
/*                         64-Byte    0x46, 0x56, 0x66, 0x76 */
/*                         128-Byte   0x47, 0x57, 0x67, 0x77 */
/*                         256-Byte   0x48, 0x58, 0x68, 0x78 */
/*                         512-Byte   0x49, 0x59, 0x69, 0x79 */
/*                         1K-Byte    0x4A, 0x5A, 0x6A, 0x7A */
/*                         2K-Byte    0x4B, 0x5B, 0x6B, 0x7B */
/*                         4K-Byte    0x4C, 0x5C, 0x6C, 0x7C */
/*                         8K-Byte    0x4D, 0x5D, 0x6D, 0x7D */
/*                         16K-Byte   0x4E, 0x5E, 0x6E, 0x7E */
/*                         32K-Byte   0x4F, 0x5F, 0x6F, 0x7F */

/*    Long Variable Size: (0x81)      0x8000 - 0xEFE5 */
/*    Long Fixed Size:     1-Byte     0xF000 - 0xF0FB */
/*                         2-Byte     0xF100 - 0xF1FF */
/*                         4-Byte     0xF200 - 0xF2F5 */
/*                         8-Byte     0xF300 - 0xF3FF */
/*                         16-Byte    0xF400 - 0xF4FF */
/*                         32-Byte    0xF500 - 0xF5FF */
/*                         64-Byte    0xF600 - 0xF6FF */
/*                         128-Byte   0xF700 - 0xF7FF */
/*                         256-Byte   0xF800 - 0xF8FF */
/*                         512-Byte   0xF900 - 0xF9FF */
/*                         1K-Byte    0xFA00 - 0xFAFF */
/*                         2K-Byte    0xFB00 - 0xFBFF */
/*                         4K-Byte    0xFC00 - 0xFCFF */
/*                         8K-Byte    0xFD00 - 0xFDFF */
/*                         16K-Byte   0xFE00 - 0xFEFF */
/*                         32K-Byte   0xFF00 - 0xFFFF */
/*  */
/* Extended Developer FIDS  0xC002 - 0xFFFF */
/*  */
/* 	Palindrome	0xC000 */
/* 	Mountain	0xC001 */
/*  */
/*  */


/*  General Fields */
#define NWSM_OFFSET_TO_END                0x00000001L
#define NWSM_HEADER_DEBUG_STRING          0x0081EFFFL
/*  Blank Space, Auth., and Code Page need to be documented in SMSDF */
#define NWSM_BLANK_SPACE                  0x00808019L
#define NWSM_AUTHENTICATION               0x00008002L
#define NWSM_CODE_PAGE                    0x0000002AL

/*  Media Header/ Trailer fields */
#define NWSM_MEDIA_HEADER                 0x00808000L
#define NWSM_REVISION_LEVEL               0x0080F300L
#define NWSM_NAME_ID_STRING               0x00008052L
#define NWSM_VERSION_NUMBER               0x00008062L
#define NWSM_MS_OPEN_DATE_TIME            0x0081F2FFL
#define NWSM_MS_OPEN_DATE_TIME_CAL        0x0080F20CL
#define NWSM_MS_OPEN_DATE_TIME_ECMA       0x0080F400L
#define NWSM_MEDIA_OPEN_DATE_TIME         0x0080F200L
#define NWSM_MEDIA_OPEN_DATE_TIME_CAL     0x0080F20BL
#define NWSM_MEDIA_OPEN_DATE_TIME_ECMA    0x0080F401L
#define NWSM_MEDIA_SET_LABEL              0x0081EFFEL
#define NWSM_MEDIA_NUMBER                 0x0080F100L
#define NWSM_MEDIA_TYPE                   0x0080F101L
#define NWSM_MEDIA_SIZE                   0x0080F201L
#define NWSM_SPANNED_SECTOR_COUNT         0x0080F102L
#define NWSM_MEDIA_OWNER                  0x00808001L
#define NWSM_EXPIRATION_DATE              0x0080F103L
#define NWSM_MEDIA_USAGE_COUNT            0x0080F104L
#define NWSM_CRC_TYPE                     0x00808002L
#define NWSM_MEDIA_TRAILER                0x00808003L
#define NWSM_CLOSE_DATE_TIME              0x0080F202L
#define NWSM_CLOSE_DATE_TIME_ECMA         0x0080F402L
#define NWSM_ATOMIC_SECTOR_SIZE           0x0080800EL
#define NWSM_DATABASE_LOCATION_METHOD     0x0080800FL
/*  Media Mark Type needs to be documented in SMSDF */
#define NWSM_MEDIA_MARK_TYPE              0x00808020L

#define SIDF_PARTITION_SET_LABEL          0x00808030L


/*  Session Header Fields */
#define NWSM_SESSION_HEADER               0x00808004L
#define NWSM_SESSION_ID                   0x00008072L
#define NWSM_SESSION_DATE_TIME            0x00008042L
#define NWSM_SESSION_DATE_TIME_ECMA       0x0080F403L
#define NWSM_SESSION_DESCRIPTION          0x00808005L
#define NWSM_SOFTWARE_NAME                0x00808006L
#define NWSM_SOFTWARE_TYPE                0x00808007L
#define NWSM_SOFTWARE_VERSION             0x00808008L
#define NWSM_SOURCE_NAME                  0x00000002L
#define NWSM_SOURCE_TYPE                  0x00000003L
#define NWSM_SOURCE_VERSION               0x00000004L

/*  Session Trailer */
#define NWSM_SESSION_TRAILER              0x00808009L

/*  Transfer Buffer Header */
#define NWSM_TRANSFER_BUFFER_HEADER       0x00000005L
#define NWSM_TRANSFER_BUFFER_SIZE         0x00000006L
#define NWSM_UNUSED_IN_THIS_BUFFER        0x00008000L
#define NWSM_TRANSFER_BUFFER_SEQUENCE     0x00000007L
#define NWSM_TRANSFER_BUFFER_TYPE         0x00000060L
#define NWSM_LAST_BUFFER_IN_SESSION       0x00000040L
#define NWSM_SESSION_SECTOR_ADDRESS       0x00000008L
#define NWSM_ATOMIC_SECTOR_ADDRESS        0x0000002EL

/*  Session/Media Index */
#define NWSM_SESSION_INDEX                0x00808010L
#define NWSM_MEDIA_INDEX                  0x00808011L
#define NWSM_MEDIA_PARTITION              0x00808012L
#define NWSM_MEDIA_SECTOR_ADDRESS         0x00808013L
#define NWSM_MEDIA_BUFFER_OFFSET          0x00808014L
#define NWSM_NUMBER_OF_SESSIONS           0x00808015L
#define NWSM_NUMBER_OF_DATABASES          0x00808016L
#define NWSM_PREVIOUS_MEDIA_INDEX         0x00808017L
#define NWSM_DATABASE_NAME                0x00808018L
#define NWSM_NUMBER_OF_DATA_SETS          0x00808021L
#define NWSM_TOTAL_SESSION_SIZE           0x00808022L
#define NWSM_RESOURCE_NAME                0x00808023L


/*  Data Record Header */
#define NWSM_DATA_RECORD_HEADER           0x00000009L
#define NWSM_ARCHIVE_DATE_TIME            0x00000042L
#define NWSM_DATA_RECORD_SEQUENCE         0x0000000AL
#define NWSM_RECORD_SIZE                  0x0000000BL
#define NWSM_BACKUP_OPTIONS               0x00000029L

/*  Data Subrecord Header */
#define NWSM_DATA_SUBRECORD_HEADER        0x00008001L

/*  Directory Information */
#define NWSM_DIRECTORY_INFORMATION        0x0000813FL
#define NWSM_SCAN_INFO_ATTRIBUTES         0x0081F2FEL
#define NWSM_SCAN_INFO_CREATOR_ID         0x0081F2FDL
#define NWSM_SCAN_INFO_CREATOR_NS         0x0081F2FCL
#define NWSM_SCAN_INFO_PRIM_DS_SIZE       0x0081F2FBL
#define NWSM_SCAN_INFO_TOTAL_DS_SIZE      0x0081F2FAL
#define NWSM_SCAN_INFO_MODIFY_FLAG        0x0081F0FFL
#define NWSM_SCAN_INFO_DELETE_FLAG        0x0081F0FEL
#define NWSM_SCAN_INFO_PARENT_FLAG        0x0081F0FDL
#define NWSM_SCAN_INFO_ACC_DATE_TIME      0x0081F2F9L
#define NWSM_SCAN_INFO_CREAT_DATE_TIME    0x0081F2F8L
#define NWSM_SCAN_INFO_MOD_DATE_TIME      0x0081F2F7L
#define NWSM_SCAN_INFO_ARC_DATE_TIME      0x0081F2F6L
#define NWSM_SCAN_INFO_OTHER_INFO         0x0081EFFDL
#define NWSM_DATA_SET_NAME_FID            0x0000813EL

/*  Record Headers/Trailers */
#define NWSM_VOLUME_HEADER                0x0081EFFCL
#define NWSM_VOLUME_TRAILER               0x0081EFFBL
#define NWSM_DIRECTORY_HEADER             0x0000000CL
#define NWSM_DIRECTORY_TRAILER            0x0000000DL
#define NWSM_FILE_HEADER                  0x0000000EL
#define NWSM_FILE_TRAILER                 0x0000000FL
#define NWSM_BINDERY_HEADER               0x0081EFFAL
#define NWSM_BINDERY_TRAILER              0x0081EFF9L
#define NWSM_286_BINDERY_HEADER           0x0081EFEBL
#define NWSM_286_BINDERY_TRAILER          0x0081EFEAL

/*  Full Paths Section */
#define NWSM_FULL_PATHS                   0x00000010L
#define NWSM_PATH_IS_FULLY_QUALIFIED      0x00000050L
#define NWSM_NAME_SPACE_TYPE              0x00000011L
#define NWSM_NAME_POSITIONS               0x00000027L
#define NWSM_SEPARATOR_POSITIONS          0x00000028L
#define NWSM_PATH_NAME                    0x00000012L

/*  Characteristics Section */
#define NWSM_CHARACTERISTICS              0x00000013L
#define NWSM_ACCESS_DATE                  0x00000041L
#define NWSM_ACCESS_TIME                  0x00000051L
#define NWSM_ACCESS_DATE_TIME_ECMA        0x00000044L
#define NWSM_ARCHIVER_ID                  0x0000813DL
#define NWSM_ARCHIVER_NAME                0x00008117L
#define NWSM_ARCHIVE_DATE_AND_TIME        0x00000052L
#define NWSM_ARCHIVE_DATE_TIME_ECMA       0x00000054L
#define NWSM_AUDIT_READ_AND_WRITE         0x0000813CL
#define NWSM_CREATION_DATE_AND_TIME       0x00000062L
#define NWSM_CREATION_DATE_TIME_ECMA      0x00000064L
#define NWSM_DIRECTORY                    0x00000014L
#define NWSM_EXECUTE_ONLY                 0x0000813BL
#define NWSM_HIDDEN                       0x00000015L
#define NWSM_INDEXED                      0x0081EFF8L
#define NWSM_INHERITED_RIGHTS_MASK        0x00008152L
#define NWSM_INHIBIT_REN_DEL_COPY         0x0000813AL
#define NWSM_LOW_MED_HIGH_SEARCH          0x00008139L
#define NWSM_MODIFIER_ID                  0x00008138L
#define NWSM_MODIFIER_NAME                0x00008120L
#define NWSM_MODIFY_DATE_AND_TIME         0x00000072L
#define NWSM_MODIFY_DATE_TIME_ECMA        0x00000074L
#define NWSM_NEEDS_ARCHIVE                0x00000016L
#define NWSM_NEEDS_ARCHIVE_CHAR           0x0000002DL
#define NWSM_OWNER_ID                     0x00008137L
#define NWSM_OWNER_NAME                   0x00008119L
#define NWSM_PURGE                        0x00008136L
#define NWSM_READ_ONLY                    0x00000017L
#define NWSM_SHARABLE                     0x00000018L
#define NWSM_SYSTEM                       0x00000019L
#define NWSM_TRANSACTIONAL                0x00008135L
#define NWSM_REMOTE_DATA_INHIBIT          0x0081EFE8L
#define NWSM_REMOTE_DATA_ACCESS           0x0081EFE7L
#define NWSM_COMPRESS_FILE_IMMEDIATE      0x00008116L
#define NWSM_DO_NOT_COMPRESS_FILE         0x00008115L
#define NWSM_CANT_COMPRESS_DATA           0x0081EFE6L

#define NWSM_OTHER_CHARACTERISTICS        0x00008134L
#define NWSM_DIR_SPACE_RESTRICTIONS       0x00008133L

/*  MAC Name Space Specific Characteristics */
#define NWSM_MAC_CHARACTERISTICS          0x00000026L
#define NWSM_MAC_FINDER_INFO              0x00000045L
#define NWSM_MAC_PRO_DOS_INFO             0x0000812DL
#define NWSM_MAC_DIR_RIGHTS_MASK          0x0000812CL
#define NWSM_MAC_SYSTEM_SCRIPT            0x00008003L
#define NWSM_MAC_BLESSED_FOLDER           0x00008004L

/*  NFS Name Space Specific Characteristics */
#define NWSM_NFS_CHARACTERISTICS          0x0080800AL
#define NWSM_NFS_FILE_ACCESS_MODE         0x0080F203L
#define NWSM_NFS_GROUP_OWNER_ID           0x0080F204L
#define NWSM_NFS_RDEVICE_OLD              0x0080F205L
#define NWSM_NFS_NUMBER_OF_LINKS_OLD      0x0080F206L
#define NWSM_NFS_LINKED_FLAG              0x0080F000L
#define NWSM_NFS_FIRST_CREATED_FLAG       0x0081F0FCL
#define NWSM_NFS_HARD_LINK_PATHS          0x0080F001L
#define NWSM_NFS_HL_PATHNAME              0x0080800BL
#define NWSM_NFS_HL_USER_ID               0x0080800CL
#define NWSM_NFS_HL_USER_NAME             0x0080EFFFL
#define NWSM_NFS_HL_GROUP_OWNER_ID        0x0080F207L
#define NWSM_NFS_HL_FILE_ACCESS_MODE      0x0080F208L
#define NWSM_NFS_HL_TERMINATOR            0x0080800DL
#define NWSM_NFS_USER_ID                  0x0080F209L
#define NWSM_NFS_MY_FLAGS                 0x0080F20AL
#define NWSM_NFS_ACS_FLAGS                0x0080F002L
#define NWSM_NFS_NUMBER_OF_LINKS          0x0080F20DL
#define NWSM_NFS_RDEVICE                  0x0080F20EL
#define NWSM_NFS_FSID                     0x0080F20FL
#define NWSM_NFS_FILEID                   0x0080F210L


/*  Extended Attributes */
#define NWSM_EXTENDED_ATTRIBUTES          0x0000001AL
#define NWSM_EA_ACCESS                    0x00008132L
#define NWSM_EA_KEY                       0x0000001BL
#define NWSM_EA_VALUE                     0x0000001CL

/*  Trustees */
#define NWSM_TRUSTEE_HEADER               0x00008131L
#define NWSM_TRUSTEE_ID                   0x00008130L
#define NWSM_TRUSTEE_NAME                 0x00008118L
#define NWSM_TRUSTEE                      0x0000812FL

/*  Volume Restrictions */
#define NWSM_VOLUME_RESTRICTIONS          0x0081EFF7L
#define NWSM_VOLUME_NAME                  0x0081EFF6L
#define NWSM_VOLUME_RSTRCTNS_ID           0x0081EFF5L
#define NWSM_VOLUME_RSTRCTNS_NAME         0x0081EFE9L
#define NWSM_VOLUME_RSTRCTNS_LIMIT        0x0081EFF4L

/*  Data Streams Sections */
#define NWSM_DATA_STREAM_HEADER           0x0000001DL
#define NWSM_DATA_STREAM_TRAILER          0x0000001EL
#define NWSM_DATA_STREAM_NAME             0x0000001FL
#define NWSM_DATA_STREAM_NUMBER           0x0000002BL
#define NWSM_DATA_STREAM_TYPE             0x0000002CL
#define NWSM_DATA_STREAM_SIZE             0x00000020L
#define NWSM_DATA_STREAM_IS_INVALID       0x00000021L
#define NWSM_DATA_STREAM_CRC              0x00000022L
#define NWSM_DATA_STREAM_COMPRESS_TYPE    0x00008005L
#define NWSM_DATA_STREAM_EXPANDED_SIZE    0x00008006L
#define NWSM_DATA_STREAM_MIGRATION_KEY    0x00008007L

/*  Sparse Data Streams Sections */
#define NWSM_SPARSE_DATA_STREAM_HEADER    0x00000023L
#define NWSM_SPARSE_DATA_BLOCK_SIZE       0x00000024L
#define NWSM_SPARSE_DATA_STREAM_MAP       0x00000025L

/*  BTrieve TSA Section */
#define NWSM_TS_HEADER                    0x0081EFF3L
#define NWSM_TS_TRAILER                   0x0081EFF2L
#define NWSM_TS_FILE_HEADER               0x0081EFF1L
#define NWSM_TS_FILE_TRAILER              0x0081EFF0L
#define NWSM_TS_NAME                      0x0081EFEFL
#define NWSM_TS_TYPE                      0x0081EFEEL
#define NWSM_TS_REQUESTED_NS_TYPE         0x0000812EL
#define NWSM_TS_NAME_POSITIONS            0x0000812BL
#define NWSM_TS_SEPARATOR_POSITIONS       0x0000812AL
#define NWSM_TS_NAME_SPACE_TYPE           0x00008129L
#define NWSM_TS_PATH_NAME                 0x00008128L
#define NWSM_TS_CHARACTERISTICS           0x00008127L
#define NWSM_TS_MAC_CHARACTERISTICS       0x00008126L
#define NWSM_TS_NFS_CHARACTERISTICS       0x0081EFEDL
#define NWSM_TS_EXTENDED_ATTRIBUTES       0x00008125L
#define NWSM_TS_TRUSTEE_HEADER            0x00008124L
#define NWSM_TS_VOLUME_RESTRICTIONS       0x0081EFECL
#define NWSM_TS_DATA_STREAM_HEADER        0x00008123L
#define NWSM_TS_DATA_STREAM_TRAILER       0x00008122L
#define NWSM_TS_SP_DATA_STREAM_HEADER     0x00008121L

/*  Soft Media Mark */
/*  Soft Media Mark needs to be documented in SMSDF */
#define NWSM_SMM_HEADER                   0x0080801AL
#define NWSM_SMM_TYPE                     0x0080801BL
#define NWSM_SMM_PREV_FILE_MARK           0x0080801CL
#define NWSM_SMM_NEXT_FILE_MARK           0x0080801DL
#define NWSM_SMM_PREV_SET_MARK            0x0080801EL
#define NWSM_SMM_NEXT_SET_MARK            0x0080801FL

/*  Directory Services TSA  */
#define NWSM_DIRECTORY_OBJECT_NAME        0x00008114L
#define NWSM_DIRECTORY_OBJECT_DATA        0x00008113L
#define NWSM_DIRECTORY_OBJECT_TRAILER     0x0000811AL


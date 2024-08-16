/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_TP_TP_H	/* wrapper symbol for kernel use */
#define _IO_TP_TP_H	/* subject to change without notice */

#ident	"@(#)kern:io/tp/tp.h	1.10"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/termios.h>	/* REQUIRED */
#include <io/stream.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/termios.h>	/* REQUIRED */
#include <sys/stream.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * tp data structure used by both kernel and user level
 */

/*
 * -internal format of Secure Attention Key (SAK)
 */


enum saktype {
	saktypeUNDEF,saktypeNONE,saktypeCHAR,saktypeLINECOND,saktypeDATA
};
enum saklinecond {
	saklinecondLINEDROP, saklinecondBREAK
};
enum saksecondary {
	saksecNO,saksecYES
};

struct sak{
	enum saktype		sak_type;
	ulong			sak_char;
	enum saklinecond	sak_linecond;
	enum saksecondary	sak_secondary;
};

/*
 * tp protocol 
 *
 * -used by ctrl channel to post SAK and Hangup notification via M_PCPROTO msg
 */

struct tpproto{
	ulong tpp_type;
};
typedef	struct tpproto	tpproto_t;


/*
 * protocol types 
 */

#define	TP_M_SAK		(1)
#define	TP_M_HANGUP		(2)


/*
 * trusted path ioctls
 *
 *	TP_CONNECT		Connect the specified ctrl channel to the TP
 *				device associated with the specified real
 *				device.
 *
 *	TP_DATACONNECT		Connect the specified data channel to the
 *				tp device associated with the specified real
 *				device.
 *
 *	TP_DATADISCONNECT	Disconnect the data channel from a tp device.
 *
 *	TP_DEFSAK		Set the specified SAK on a tp device and
 *				fill out the valid and mask termios.
 *
 *	TP_CONSCONNECT		Connect the tp chan associated with /dev/tp/cons
 *				to the real device that is the current 'console'
 *				TP device.  After connected, all messages coming
 *				upstream from the 'console' TP device is
 *				redirected to the tp chan associated with
 *				/dev/tp/cons.
 *
 *	TP_CONSDISCONNECT	Disconnect the tp chan associated with
 *				/dev/tp/cons from the current 'console' TP
 *				device.  after disconnected, all messages
 *				coming upstream from the 'console' TP device
 *				will be directed up the data chan (if one
 *				exists).
 *
 *	TP_CONSSET		Switch the current 'console' TP device to the
 *				real device specified in the ioctls argument
 *				(tpinf_rdev).  the new 'console' TP device
 *				does not inherit the "connected for input"
 *				status of the previously current 'console' TP
 *				device.  The default for the new 'console' TP
 *				device is not to be "connected for input".
 *
 *	TP_GETINF		Get TP device's information structure.
 *				P_DRIVER privilege is required if ioctl is
 *				issued on the data or cons channel.
 */

#define TPIOC			(('T' << 16) | ('P' << 8))
#define	TP_CONNECT		(TPIOC | 1)
#define	TP_DATACONNECT		(TPIOC | 2)
#define	TP_DATADISCONNECT	(TPIOC | 3)
#define	TP_DEFSAK		(TPIOC | 4)
#define	TP_CONSCONNECT		(TPIOC | 5)
#define	TP_CONSDISCONNECT	(TPIOC | 6)
#define	TP_CONSSET		(TPIOC | 7)
#define	TP_GETINF		(TPIOC | 8)

/*
 * The following structure is passed to TP ioctls.  All fields are filled
 * in with the appropriate information when the ioctl returns.
 * The tag (out) in the descriptions of the fields indicates that the field
 * is only used for outputting information and the field is always ignored
 * when the ioctl is processed in the driver.
 */
struct	tp_info {
	dev_t		tpinf_rdev;	/* Device # of the real device (ie. the
					 * device that is linked or will be
					 * linked under a TP device)
					 */
	dev_t		tpinf_rdevfsdev;/* Device # of the File System in which
					 * the real device's Device Special
					 * File (DSF) resides
					 */
	ino_t		tpinf_rdevino;	/* Inode # of the real device's DSF */
	mode_t		tpinf_rdevmode;	/* File Attributes of real device's
					 * DSF
					 */
	dev_t		tpinf_cdev;	/* Device # of ctrl chan (out) */
	dev_t		tpinf_ddev;	/* Device # of data chan */
	dev_t		tpinf_dev;	/* Device # of chan issuing ioctl */
	int		tpinf_muxid;	/* MUX id. of linked TP MUX (out) */
	int		tpinf_cconnid;	/* Connection id of ctrl chan (out) */
	int		tpinf_dconnid;	/* Connection id of data chan */
	int		tpinf_connid;	/* Connection id of chan issuing
					 * ioctl (out)	
					 */
	struct sak	tpinf_sak;	/* SAK information for TP MUX */
	int		tpinf_flags;	/* See below for description of flags */
	struct termios	tpinf_mask;	/* Legal termios mask for TP MUX (out)*/
	struct termios	tpinf_valid;	/* Valid termios for TP MUX (out)*/
};


/*
 * tp_info flags
 */
#define	TPINF_DISCONNDATA_ONHANGUP	(0x01)
#define	TPINF_FAILUNLINK_IFDATA		(0x02)
#define	TPINF_CONSOLE			(0x04)
#define	TPINF_ONLYIFLINKED		(0x08)

/*
 * TPINF_DISCONNDATA_ONHANGUP
 *
 *	-valid for TP_DATACONNECT ioctl
 *	-disconnect data channel when hanup is detected
 *	-flag is cleared when data channel is disconnected
 *
 *
 * TPINF_FAILUNLINK_IFDATA
 *
 *	-valid for TP_DATACONNECT ioctl
 *	-will cause I_PUNLINK to fail if a data channel is connected
 *	-flag is cleared when data channel is disconnected
 *	-NOTE: not valid for I_UNLINK since the driver can not determine
 *	 if the I_UNKLINK is directly called via an ioctl or it was called
 *	 because the multiplexing Stream is being closed.  Failing
 *	 I_UNLINK in the TP driver would cause problems if the I_UNLINK was
 *	 caused as a result the multiplexing Stream being closed because the
 *	 stream head would still "unlink" the lower multiplexed Stream from
 *	 the TP driver.  The stream head functions would re-assign the Top
 *	 most Queue of the lower multiplexed Stream to a Stream Head Queue
 *	 (ie. reset Queue's q_ptr field etc.) and then call closef() which
 *	 descrements the open refernce count and may actually close the Stream
 *	 if the reference count becomes zero.  If the TP driver failed the
 *	 I_UNLINK and later tried to access(read from/write to) what it
 *	 thought was the address of the lower Stream, all kinds of unpleasant
 *	 events could happen from data corrurption to system panics!!
 *
 *
 * TPINF_CONSOLE
 *
 *	-valid for TP_CONNECT ioctl
 *	If this flag is set when the TP_CONNECT ioctl is made, the tp device of
 *	the the real device to which the connection is made is marked as a
 *	candidate to become the console device.  When the real device is
 *	linked under this device, the device becomes the console device.
 *	This means that all writes to the /dev/tp/cons channel will go to
 *	this device.  All messages coming upstream from the 'console' device
 *	will be re-directed to /dev/tp/cons if /dev/tp/cons gets connected
 *	(via TP_CONSCONNECT ioctl) to 'console' TP device.
 *	If /dev/tp/cons is not yet open, the console device will be remembered.
 *	Only one device at a time can be 'console' so the last device linked
 *	under a tp device with the TPINF_CONSOLE flag set becomes the 'console'
 *	device.
 *
 * TPINF_ONLYIFLINKED
 *	-valid for TP_CONSSET ioctl
 *	If this flag is set for the TP_CONSSET ioctl,  the console device is
 *	only switched to the real device if the real device is linked under
 *	a TP device.
 */

/*
 * macros to load a tp_info structure and to unload a tp_info structure
 */

#define TP_LOADINF(tpinf,rdev,rdevfsdev,rdevino,rdevmode,cdev,ddev,dev,muxid,cconnid,dconnid,connid,sak,flags,masktermios,validtermios) { \
		(tpinf).tpinf_rdev	= rdev; \
		(tpinf).tpinf_rdevfsdev	= rdevfsdev; \
		(tpinf).tpinf_rdevino	= rdevino; \
		(tpinf).tpinf_rdevmode	= rdevmode; \
		(tpinf).tpinf_cdev	= cdev; \
		(tpinf).tpinf_ddev	= ddev; \
		(tpinf).tpinf_dev	= dev; \
		(tpinf).tpinf_muxid	= muxid; \
		(tpinf).tpinf_cconnid	= cconnid; \
		(tpinf).tpinf_dconnid	= dconnid; \
		(tpinf).tpinf_connid	= connid; \
		(tpinf).tpinf_sak	= sak; \
		(tpinf).tpinf_flags	= flags; \
		(tpinf).tpinf_mask	= masktermios; \
		(tpinf).tpinf_valid	= validtermios; \
}


#define TP_UNLOADINF(tpinf,rdev,rdevfsdev,rdevino,rdevmode,cdev,ddev,dev,muxid,cconnid,dconnid,connid,sak,flags,masktermios,validtermios) { \
		rdev		= (tpinf).tpinf_rdev; \
		rdevfsdev	= (tpinf).tpinf_rdevfsdev; \
		rdevino		= (tpinf).tpinf_rdevino; \
		rdevmode	= (tpinf).tpinf_rdevmode; \
		cdev		= (tpinf).tpinf_cdev; \
		ddev		= (tpinf).tpinf_ddev; \
		dev		= (tpinf).tpinf_dev; \
		muxid		= (tpinf).tpinf_muxid; \
		cconnid		= (tpinf).tpinf_cconnid; \
		dconnid		= (tpinf).tpinf_dconnid; \
		connid		= (tpinf).tpinf_connid; \
		sak		= (tpinf).tpinf_sak; \
		flags		= (tpinf).tpinf_flags; \
		masktermios	= (tpinf).tpinf_mask; \
		validtermios	= (tpinf).tpinf_valid; \
}

/*
 * The following list inputs of the tpinf fields for each TP ioctl.  The
 * input fields are tagged with (input) in their description.  All fields are
 * filled in when the ioctl completes successfully.
 *
 * TP_CONNECT
 *	tpinf_rdev	Device number of the real device associated with the
 *			TP device that the ctrl channel is to be connected
 *			(input).
 *	tpinf_rdevfsdev	Device number of the file system in which the real
 *			device's DSF resides (input).
 *	tpinf_rdevino	Inode number of the real device's DSF (input).
 *	tpinf_rdevmode	File attributes of the real device's DSF (input).
 *	tpinf_cdev	Device number of the control channel.
 *	tpinf_ddev	Device number of data channel, NODEV if not connected.
 *	tpinf_dev	Device number of tp channel issuing the ioctl.
 *	tpinf_muxid	MUX ID of TP MUX or 0 if not linked.
 *	tpinf_cconnid	Connection id of ctrl channel.
 *	tpinf_dconnid	Connection id of data channel, 0 if no data channel is
 *			connected.
 *	tpinf_connid	Connection id of tp channel (ctrl) issuing the ioctl.
 *	tpinf_sak	Current SAK on TP MUX, saktypeUNDEF if SAK is not
 *			defined.
 *	tpinf_flags	See descriptions on tp_info flags (input).
 *	tpinf_mask	Current legal termios change mask, all
 *			zeros if no SAK is defined.
 *	tpinf_valid	Current valid termios for TP MUX, all zeros
 *			if no SAK is defined.
 *
 * TP_DATACONNECT
 *	tpinf_rdev	Device number of the real device associated with the
 *			TP device that the ctrl channel is to be connected
 *			(input).  rdev* information is only used if connecting
 *			via adm channel.  If connecting via ctrl channel the
 *			rdev associated with the ctrl channel is used.
 *	tpinf_rdevfsdev	Device number of the file system in which the real
 *			device's DSF resides (input).
 *	tpinf_rdevino	Inode number of the real device's DSF (input).
 *	tpinf_rdevmode	File attributes of the real device's DSF (input).
 *	tpinf_cdev	Device number of the control channel.
 *	tpinf_ddev	Device number of data channel (input).
 *	tpinf_dev	Device number of tp channel issuing the ioctl.
 *	tpinf_muxid	MUX ID of TP MUX or 0 if not linked.
 *	tpinf_cconnid	Connection id of ctrl channel.
 *	tpinf_dconnid	Connection id of data channel.
 *	tpinf_connid	Connection id of tp channel (ctrl) issuing the ioctl.
 *	tpinf_sak	Current SAK on TP MUX, saktypeUNDEF is no SAK is
 *			defined.
 *	tpinf_flags	See descriptions on tp_info flags (input).
 *	tpinf_mask	Current legal termios change mask for TP MUX.
 *	tpinf_valid	Current valid termios for TP MUX.
 *
 * TP_DATADISCONNECT
 *	tpinf_rdev	Device number of the real device associated with the
 *			TP device that the ctrl channel is to be connected.
 *	tpinf_rdevfsdev	Device number of the file system in which the real
 *			device's DSF resides.
 *	tpinf_rdevino	Inode number of the real device's DSF.
 *	tpinf_rdevmode	File attributes of the real device's DSF.
 *	tpinf_cdev	Device number of the control channel.
 *	tpinf_ddev	NODEV, data channel is disconnected.
 *	tpinf_dev	Device number of tp channel issuing the ioctl.
 *	tpinf_muxid	MUX ID of TP MUX or 0 if not linked.
 *	tpinf_cconnid	Connection id of ctrl channel.
 *	tpinf_dconnid	Connection id of data channel (input). value is 0 when
 *			ioctl returns.
 *	tpinf_connid	Connection id of tp channel (ctrl) issuing the ioctl.
 *	tpinf_sak	Current SAK on TP MUX, saktypeUNDEF if no SAK is
 *			defined.
 *	tpinf_flags	See descriptions on tp_info flags (input).
 *	tpinf_mask	Current legal termios change mask for TP MUX.
 *	tpinf_valid	Current valid termios for TP MUX.
 *
 * TP_DEFSAK
 *	tpinf_rdev	Device number of the real device associated with the
 *			TP device that the ctrl channel is to be connected.
 *	tpinf_rdevfsdev	Device number of the file system in which the real
 *			device's DSF resides.
 *	tpinf_rdevino	Inode number of the real device's DSF.
 *	tpinf_rdevmode	File attributes of the real device's DSF.
 *	tpinf_cdev	Device number of the control channel.
 *	tpinf_ddev	Device number of the data channel connected to the
 *			TP MUX, NODEV if not connected. If a data channel
 *			is connected, the TP_DEFSAK will take effect after the
 *			data channel disconnects.
 *	tpinf_dev	Device number of tp channel issuing the ioctl.
 *	tpinf_muxid	MUX ID of TP MUX or 0 if not linked.
 *	tpinf_cconnid	Connection id of ctrl channel.
 *	tpinf_dconnid	Connection id of data channel, 0 if not connected.
 *	tpinf_connid	Connection id of tp channel (ctrl) issuing the ioctl.
 *	tpinf_sak	Specified SAK for TP MUX (input).  On output it will
 *			be the current \fBdefault SAK\fP if specified \fBSAK\fP
 *			is held.  This can happen if \fBTP device\fP has a
 *			channel connected for input.  If no channel connected
 *			for input, specified \fBSAK\fP becomes the
 *			\fBTP device's default SAK\fP.
 *	tpinf_flags
 *	tpinf_mask	Legal termios change mask based on \fBdefault SAK\fP.
 *	tpinf_valid	Valid termios based on \fBdefault SAK\fP.
 *
 * TP_CONSCONNECT
 *	tpinf_rdev	Device number of the real device associated with the
 *			TP device that the ctrl channel is to be connected.
 *	tpinf_rdevfsdev	Device number of the file system in which the real
 *			device's DSF resides.
 *	tpinf_rdevino	Inode number of the real device's DSF.
 *	tpinf_rdevmode	File attributes of the real device's DSF.
 *	tpinf_cdev	Device number of the control channel, NODEV if not
 *			connected.
 *	tpinf_ddev	Device number of data channel, NODEV if not connected.
 *	tpinf_dev	Device number of tp channel issuing the ioctl.
 *	tpinf_muxid	MUX ID of TP MUX or 0 if not linked.
 *	tpinf_cconnid	Connection id of ctrl channel, 0 if not connected.
 *	tpinf_dconnid	Connection id of data channel, 0 if not connected.
 *	tpinf_connid	Connection id of tp channel (ctrl) issuing the ioctl.
 *	tpinf_sak	Current SAK on TP MUX, saktypeUNDEF if no SAK is
 *			defined.
 *	tpinf_flags
 *	tpinf_mask	Current legal termios change mask, all zeros if no
 *			SAK defined.
 *	tpinf_valid	Current valid termios for TP MUX, all zeros if no
 *			SAK defined.
 *
 * TP_CONSDISCONNECT
 *	tpinf_rdev	Device number of the real device associated with the
 *			TP device that the ctrl channel is to be connected.
 *	tpinf_rdevfsdev	Device number of the file system in which the real
 *			device's DSF resides.
 *	tpinf_rdevino	Inode number of the real device's DSF.
 *	tpinf_rdevmode	File attributes of the real device's DSF.
 *	tpinf_cdev	Device number of the control channel, NODEV if not
 *			connected.
 *	tpinf_ddev	Device number of data channel, NODEV if not connected.
 *	tpinf_dev	Device number of tp channel issuing the ioctl.
 *	tpinf_muxid	MUX ID of TP MUX or 0 if not linked.
 *	tpinf_cconnid	Connection id of ctrl channel, 0 if not connected.
 *	tpinf_dconnid	Connection id of data channel, 0 if not connected.
 *	tpinf_connid	Connection id of tp channel (ctrl) issuing the ioctl.
 *	tpinf_sak	Current SAK on TP MUX, saktypeUNDEF if no SAK is
 *			defined.
 *	tpinf_flags
 *	tpinf_mask	Current legal termios change mask.
 *	tpinf_valid	Current valid termios for TP MUX.
 *
 * TP_CONSSET
 *	tpinf_rdev	Device number of the real device to switch to the
 *			'console' TP device.(input)
 *	tpinf_rdevfsdev	Device number of the file system in which the real
 *			device's DSF resides.(input)
 *	tpinf_rdevino	Inode number of the real device's DSF.(input)
 *	tpinf_rdevmode	File attributes of the real device's DSF.(input)
 *	tpinf_cdev	Device number of the control channel, NODEV if not
 *			connected.
 *	tpinf_ddev	Device number of data channel, NODEV if not connected.
 *	tpinf_dev	Device number of tp channel issuing the ioctl.
 *	tpinf_muxid	MUX ID of TP MUX or 0 if not linked.
 *	tpinf_cconnid	Connection id of ctrl channel, 0 if not connected.
 *	tpinf_dconnid	Connection id of data channel, 0 if not connected.
 *	tpinf_connid	Connection id of tp channel (ctrl) issuing the ioctl.
 *	tpinf_sak	Current SAK on TP MUX, saktypeUNDEF if no SAK is
 *			defined.
 *	tpinf_flags
 *	tpinf_mask	Current legal termios change mask, all zeros if no
 *			SAK defined.
 *	tpinf_valid	Current valid termios for TP MUX, all zeros if no
 *			SAK defined.
 *
 * TP_GETINF
 *	tpinf_rdev	Device number of the real device associated with the
 *			TP device that the ctrl channel is to be connected
 *			(input).  rdev* information is only useful when ioctl
 *			is being issued from the adm channel.
 *	tpinf_rdevfsdev	Device number of the file system in which the real
 *			device's DSF resides (input).
 *	tpinf_rdevino	Inode number of the real device's DSF (input).
 *	tpinf_rdevmode	File attributes of the real device's DSF (input).
 *
 *	Following output only if the invoker has privilige (P_DRIVER).
 *	tpinf_cdev	Device number of the control channel, NODEV if not
 *			connected.
 *	tpinf_ddev	Device number of data channel, NODEV if not connected.
 *	tpinf_dev	Device number of tp channel issuing the ioctl.
 *	tpinf_muxid	MUX ID of TP MUX or 0 if not linked.
 *	tpinf_cconnid	Connection id of ctrl channel, 0 if not connected.
 *	tpinf_dconnid	Connection id of data channel, 0 if not connected.
 *	tpinf_connid	Connection id of tp channel (ctrl) issuing the ioctl.
 *	tpinf_sak	Current SAK on TP MUX, saktypeUNDEF if no SAK is
 *			defined.
 *	tpinf_flags
 *	tpinf_mask	Current legal termios change mask, saktypeUNDEF if no
 *			SAK is defined.
 *	tpinf_valid	Current valid termios for TP MUX, saktypeUNDEF if no
 *			SAK is defined.
 */

/*
 * END (tp data structure used by both kernel and user level)
 * tp data structure used only by kernel
 */


/*
 * TP channel and device data structures are managed by centralized list
 * management functions.  If new list oriented data structures want to
 * use these facilities in the future they must obey the following rules:
 *
 *	1) Declare a flags field as the first ulong in the structure.
 *
 *	2) Reserve the low order bit (0x00000001) of the flags field for
 *	   use by the list manangement routines.
 *
 *	3) Declare a key field as the second ulong in the structure, this
 *	   key can be named anything, but should be the primary entry used
 *	   to search the list.  It helps if the key values are unique
 *	   within the list since the general purpose search routines
 *	   will return the first match.
 */

#define	TP_IDLE		0x00000001	/* indicates the channel is available*/

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * TP Channel Structure
 */

struct tpchan {
	lock_t		*tpc_data_mutex;/* lock for data in tpchan structure*/
	sv_t		*tpc_close_sv;	/* sync variable to sleep on close*/
	ulong	 	tpc_flags;	/* Channel flags*/
	dev_t	 	tpc_dev;	/* minor device number of tp channel */
	ulong		tpc_type;	/* Type of channel (ctrl, data, etc.)*/
	ulong		tpc_connid;	/* Unique connection id of channel*/
	queue_t		*tpc_rq;	/* read queue of the tp device */
	struct	 tpdev	*tpc_devp;	/* Pointer to connected TP device */
	mblk_t		*tpc_sakmsg;	/* Pointer to message to send on SAK*/
	mblk_t		*tpc_hupmsg;	/* Pointer to message to send on HUP*/
	mblk_t		*tpc_ioctlmp;	/* Pointer to ioctl message to be sent
					 * downstream.
					 */
	mblk_t		*tpc_trailmsg;	/* Pointer to M_TRAILER to send
					 * with HUP
					 */
	queue_t		*tpc_putnextctl1q;/*queue for deferred putnextctl*/
	int		tpc_putnextctl1type; /*type for deferred putnextctl*/
	int		tpc_putnextctl1param; /*param for deferred putnextctl*/
	
};

#endif /* _KERNEL || _KMEMUSER */

/*
 * tpc_flags 
 */
/*Flag 0x00000001 is reserved for list management use*/
#define	TPC_ISOPEN	0x00000002	/* indicates the channel is open */
#define	TPC_BLOCKED	0x00000004	/* Set on data channel when a SAK is
					 * detected.  In this state no data may
					 * flow.
					 */
#define	TPC_DIRTY	0x00000008	/* Data clone is not yet available*/
#define	TPC_BUSYNEXT	0x00000010	/* Lower is busy using this upper queue
					 * we cause a close to sleep if this
					 * flag is set
					 */
#define	TPC_CLOSING	0x00000020	/* Set if we attempt to do a close on
					 * an upper and TPC_BUSYNEXT is active
					 * in this case we would be 'sleeping'
					 * waiting for that putnext to finish
					 */

/*
 * tpc_types 
 */
#define	TPC_CONS		0
#define	TPC_ADM			1
#define	TPC_CTRL		2
#define	TPC_DATA		3

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * tp device structure
 */

struct tpdev {
	lock_t		*tpd_data_mutex;/* general lock for tpdev structure*/
	ulong	 	tpd_flags;	/* Device flags*/
	dev_t		tpd_realdev;	/* major/minor device number of the
					 * real physical device.
					 */
	dev_t		tpd_realdevfsdev;/* Device number of the file system in
					  * which the real device's DSF resides
					  */
	ino_t		tpd_realdevino;	/* Inode # of the real device's DSF */
	mode_t		tpd_realdevmode;/* File attributes of the real device's
					 * DSF
					 */
	queue_t		*tpd_realrq;	/* read queue of the real physical
					 * device. aka the (lower) ctrl channel
					 */
	queue_t		*tpd_ctrlrq;	/* read queue of the (upper) ctrl
					 * channel
					 */
	queue_t		*tpd_datarq;	/* read queue of the data channel */
	queue_t		*tpd_inputrq;	/* read queue of the channel
					 * to route upstream data.
					 * currently can only be a data or the
					 * cons channel.
					 */
	ulong		tpd_userflags;	/* User supplied flags (from tp_info)*/
        int             tpd_timeoutid;  /* indicates a pending function to be
                                         * called with a pointer to the TP
                                         * device structure as an argument.
                                         */
	struct	 sak	tpd_sak;	/* Secure Attention Key */
	struct	 sak	tpd_heldsak;	/* Held Secure Attention Key */
	mblk_t		*tpd_tcsetp;	/* Held TCSET* ioctl message*/
	ulong	 	tpd_muxid;	/* mux link id of the lower stream */
	minor_t		tpd_minordev;	/* "minor" device number of tp device
					 * NOTE: this number is internal to
					 * tp driver code.
					 */
	struct tpchan	*tpd_ioctlchan;	/* indicates which channel is doing a
					 * termio ioctl when TPD_BUSY_IOCTL is
					 * set
					 */
	uint		tpd_ioctlid;	/* ioctl id of last ioctl sent down to
					 * the real/physical device driver.
					 */
	struct tpchan	*tpd_ctrlchp;	/* ctrl channel associated with
					 * with this tpdev
					 */ 
	struct tpchan	*tpd_datachp;	/* data channel associated with
					 * with this tpdev
					 */ 
	struct tpchan	*tpd_inputchp;	/* channel associated with
					 * this tpdev to which upstream
					 * data is routed.
					 * currently can only be a data or the
					 * cons channel.
					 */ 

	struct termios	tpd_mask; 	/* See explanation below */
	struct termios	tpd_valid;	/* See explanation below */
	struct termios	tpd_curterm;	/* indicates current termio(s) setting*/
	struct termios	tpd_nextterm;	/* indicates the next termios setting.
					 * curtermios will be set to
					 * nexttermios when a positive
					 * acknowledgement is detected
					 * M_IOCACK on the read queue 
					 * and the ioctl cmd issued was a
					 * TCSET* type.
					 */
	mblk_t		*tpd_discioctl;	/* An internal created M_IOCTL message
					 * that is created when the data 
					 * get connected and sent when all
					 * channels that can receive input
					 * messages are disconnected.  The
					 * purpose of sending the M_IOCTL is
					 * to drop DTR on the physical/real
					 * device.
					 */
	queue_t		*tpd_putnextq;	/* queue for deferred putnext */
	mblk_t		*tpd_putnextmp; /* mp for deferred putnext */
};

#endif /* _KERNEL || _KMEMUSER */

/*
 * -Explanation of certain fields in tpdev
 *
 * tpd_mask
 *
 *	each set bit in the mask indicates termios flag(s) that must be
 *	checked whenever a TCSET* termio(s) ioctl is sent.
 *
 *	If the termio(s) ioctl is sent via the data channel the protection
 *	algorithm is as follows:
 *		-XOR the new termio(s) values with the current termio(s) values
 *		-AND result with data channel termios mask
 *		-if the result is positive, the ioctl is failed
 *
 *	NOTE: the mask values for the special control characters c_cc are not
 *	used and have zero filled values.  The special control characters
 *	protection checks are handled a different way inside the tp device.
 * 
 * tpd_valid
 *
 *	Contains the termios flag values that should be (absolute values)
 *	inorder to prevent the possibility of defeating SAK recognition,
 *	when a TCSET* termio(s) ioctl is sent via the ctrl channel.
 *	NOTE: Only flags values indicated by the ctrl channel termios mask
 *	need to be reflected in tpd_valid.  All other flags values
 *	must be zero.
 */


/*
 * tpd_flags
 */
/*Flag 0x00000001 is reserved for list management use*/
#define	TPD_PERSIST	0x00000002
#define	TPD_SAKSET	0x00000004
#define	TPD_HANGUPSET	0x00000008
#define	TPD_DEFSAK	0x00000010
#define	TPD_BUSY_IOCTL	0x80000000
#define	TPD_WAIT_IOCTL	0x40000000
#define	TPD_WAITTCSET	0x20000000
#define	TPD_BUSYNEXT	0x10000000
#define	TPD_TRANSPARENT	0x08000000

/*
 * TPD_PERSIST
 *	-Set when RDEV is PLINKed
 *
 * TPD_SAKSET
 *	-Set when a SAK has been detected
 *
 * TPD_HANGUPSET
 *	-Set when a HUP has been detected
 *
 * TPD_DEFSAK
 *	-Set when a TP_DEFSAK is pending
 *
 * TPD_BUSY_IOCTL
 *	-indicates that the TP device is currently doing a real tty device
 *	 ioctl and that any other tty ioctl must wait until TPD_BUSY_IOCTL is
 *	 cleared.  This flow control is necessary because both the ctrl and
 *	 data channel can issue tty termio ioctls ...when ACK or NAK comes back
 *	 up the read side, need to know which channel sent it.
 *
 * TPD_WAIT_IOCTL
 *	-indicate a tp channel has a pending downstream ioctl and a
 *	 downstream ioctl is currently in progress.  When the ioctl
 *	 acknowledgement is detected, the appropriated queue is enabled.
 *
 * TPD_WAITTCSET
 *	-indicate a tp device has had its SAK set and is awaiting 
 *	 confirmation of a TCSET* ioctl before it will allow a dataconnect.
 *
 * TPD_BUSYNEXT
 *	-indicates that an upper routine is busy in a putnext on this lower
 *	 device. in this case we would fail an I_UNLINK if one occured
 */

/*
 * minor device numbers
 */

#define	TP_CONSDEV	0	/*Console output device (write only)*/
#define	TP_CTRLCLONE	1	/*TP Control clone master device*/
#define	TP_DATACLONE	2	/*TP Data clone master device*/
#define	TP_ADMDEV	3	/*TP Administration device*/
#define	TP_RESERVDEV1	4	/*Minor # 4 reserved for future use*/
#define	TP_RESERVDEV2	5	/*Minor # 5 reserved for future use*/
#define	TP_RESERVDEV3	6	/*Minor # 6 reserved for future use*/
#define	TP_RESERVDEV4	7	/*Minor # 7 reserved for future use*/
#define	TP_RESERVDEV5	8	/*Minor # 8 reserved for future use*/
#define	TP_RESERVDEV6	9	/*Minor # 9 reserved for future use*/
#define	TP_NONCLONE	10	/*First TP clone minor number*/

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TP_TP_H */

;*****************************************************************************
;
;	(C) Unpublished Copyright of Novell, Inc. All Rights Reserved.
;
;	No part of this file may be duplicated, revised, translated,
;	localized or modified in any manner or compiled, linked or uploaded
;	or downloaded to or from any computer system without the prior
;	written consent of Novell, Inc.
;
;*****************************************************************************

DedIPXMajVersion	equ	3		;IPX Support level v3.30
DedIPXMinVersion	equ	30		; Interface version level

VIPXSupportLevel	equ	7		;Bumped when interface changes

GNMAVersion		equ	1		;GNMA interface version
						;Bumped when new functions are defined

NovellMajVersion	equ	2		;Our version
NovellMinVersion	equ	00
NovellVerText		equ	'v2.00'		;Matchs Novell???Version

TestingVer		equ	' '	;Set to NULL if release version

;	ModuleFlags bit definitions.

IPXActive	equ	01h
SPXActive	equ	02h
DiagActive	equ	04h

;	Feature Bits.

ThisIsIPXODIBit		equ	0001h		;Set if this is IPXODI.COM
ChecksumAPISupported	equ	0002h		;Functions 20h, 21h & 22h supported
SPXDiagsSupported	equ	0004h		;Set if SPX diags loaded.

;	GetLocalTarget stack structure.

GetLocalTargetStackStructure	struc
	StackInUse		dw	?
				db	(384-7) dup (?)
	StackData		db	?
	OldSP			dw	?
	OldSS			dw	?
GetLocalTargetStackStructure	ends

; SwitchBitMap bit definitions.

SwitchNoDiag		equ	01h
SwitchNoSPX		equ	02h
SwitchCustomConfigFile	equ	04h
SwitchUnload		equ	08h
SwitchForceUnload	equ	10h
SwitchInfo		equ	20h



; Min,Max values for a few things.

MinAllowedIPXSockets	equ	10
MaxAllowedIPXSockets	equ	150

MinSPXSessions		equ	5
MaxSPXSessions		equ	150

MinGLTStacks 		equ	1
MaxGLTStacks		equ	10

; Task switcher instance data structure.

InstanceStruc	struc
	IVersion	db	3,0		;Not used
	INext		dw	2 dup (0)
	IVxDFile	dd	0		;Not used
	IRefData	dd	0		;Not used
	IDataPtr	dw	2 dup (0)
InstanceStruc	ends

IDataStruc	struc
	InstancePtr	dw	2 dup (0)
	InstanceSize	dw	0
IDataStruc	ends


DUMPREGS		MACRO 	val
	push	ax
	mov	ax, val
	db	0fh, 5fh, 0ah
	pop	ax
ENDM

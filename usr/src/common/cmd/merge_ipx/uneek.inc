;*****************************************************************************
;UNEEK.INC (UNIQUE)
;
;	DOS Assembly-Language Services Customization Include File
;	for interfacing NWASMLIB to pure assembler modules that have
;	custom segments and memory models.
;
;	Module Name:	IPXODI.COM
;	Written By:	Drex C. Dixon
;	Date:		2-5-92
;
;*****************************************************************************
;
; This file contains module dependent information such as the segment names
; align type, etc.
;
; This file must be customized for each executable module that uses the
; NWASMLIB Services.
;
; At this time NWASMLIB does not support the Intel ASM86 assembler or
; the PharLap assembler.
;
;*****************************************************************************


;*****************************************************************************
;           S E G M E N T   &   G R O U P   I N F O R M A T I O N
;*****************************************************************************

CODE_SEG_NAME		equ	<NWASMLIBSeg>
CODE_SEG_ATTR		equ	<public word 'CODE'>
DATA_SEG_NAME		equ	<NWASMLIBSeg>
DATA_SEG_ATTR		equ	<public word 'CODE'>

; If your module doesn't use group(s) comment out the appropriate
; lines below.  If the data group and code group are the same
; simply give them both the same names.

CODE_GROUP		equ	<CGroup>
DATA_GROUP		equ	<CGroup>

;*****************************************************************************
;                         M O D U L E   I N F O
;*****************************************************************************

CODE_NEAR		equ	0
CODE_FAR		equ	1
COM_MODULE		equ	0
EXE_MODULE		equ	1

; Set the CODE_DISTANCE equal to either CODE_NEAR or CODE_FAR.  This sets up
; whether or not calls between your module and NWASMLIB are near or far.
; This only affects code references, all data references are far.

CODE_DISTANCE		equ	CODE_NEAR

; Set the MODULE_TYPE equal to either COM_MODULE or EXE_MODULE depending
; on whether or not the module is a .COM or a .EXE file.

MODULE_TYPE		equ	COM_MODULE


;*****************************************************************************
;                       S E R V I C E   U S A G E 
;*****************************************************************************

; Set this equate to 0 if you do not wish to use NWArgc & NWArgv.

NW_ARG_ENABLED		equ	1

; Set this equate to 0 if you do not wish to use the NET.CFG parser services.

NW_CFG_PARSER_ENABLED	equ	1

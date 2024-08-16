/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/mlib.h	1.1"
/***************************************************************************
 * (c) Copyright 1983 - 1992 Novell, Inc.  All rights reserved.
 *  Program Name:	NetWare v3.11 NLM Master - NLM Library
 *      Filename:	MLIB.H
 * Date Modified:	September 15, 1992
 *       Version:	4.00 Rev A
 *
 **************************************************************************/

#define KEY_BS			8
#define KEY_CTRL_C			3
#define KEY_CTRL_Z			26
#define KEY_ESC			27
#define KEY_TAB			9
#define KEY_ENTER			'\n'
#define KEY_NOKEY			-1

#define KEY_F1			256
#define KEY_F2			257
#define KEY_F3			258
#define KEY_F4			259
#define KEY_F5			260
#define KEY_F6			261
#define KEY_F7			262
#define KEY_F8			263
#define KEY_F9			264
#define KEY_F10			265
#define KEY_F11			266
#define KEY_F12			267
#define KEY_SHIFT_F1		268
#define KEY_SHIFT_F2		269
#define KEY_SHIFT_F3		270
#define KEY_SHIFT_F4		271
#define KEY_SHIFT_F5		272
#define KEY_SHIFT_F6		273
#define KEY_SHIFT_F7		274
#define KEY_SHIFT_F8		275
#define KEY_SHIFT_F9		276
#define KEY_SHIFT_F10		277
#define KEY_SHIFT_F11		278
#define KEY_SHIFT_F12		279
#define KEY_CTRL_F1		280
#define KEY_CTRL_F2		281
#define KEY_CTRL_F3		282
#define KEY_CTRL_F4		283
#define KEY_CTRL_F5		284
#define KEY_CTRL_F6		285
#define KEY_CTRL_F7		286
#define KEY_CTRL_F8		287
#define KEY_CTRL_F9		288
#define KEY_CTRL_F10		289
#define KEY_CTRL_F11		290
#define KEY_CTRL_F12		291
#define KEY_ALT_F1			292
#define KEY_ALT_F2			293
#define KEY_ALT_F3			294
#define KEY_ALT_F4			295
#define KEY_ALT_F5			296
#define KEY_ALT_F6			297
#define KEY_ALT_F7			298
#define KEY_ALT_F8			299
#define KEY_ALT_F9			300
#define KEY_ALT_F10		301
#define KEY_ALT_F11		302
#define KEY_ALT_F12		303
#define KEY_ALT_1			304
#define KEY_ALT_2			305
#define KEY_ALT_3			306
#define KEY_ALT_4			307
#define KEY_ALT_5			308
#define KEY_ALT_6			309
#define KEY_ALT_7			310
#define KEY_ALT_8			311
#define KEY_ALT_9			312
#define KEY_ALT_0			313
#define KEY_ALT_HYPHEN		314
#define KEY_ALT_EQUALS		315

#define KEY_HOME			316
#define KEY_UP			317
#define KEY_PGUP			318
#define KEY_LEFT			319
#define KEY_RIGHT			320
#define KEY_END			321
#define KEY_DOWN			322
#define KEY_PGDN			323
#define KEY_INSERT			324
#define KEY_DELETE			325
#define KEY_CTRL_PRTSC		326
#define KEY_CTRL_LEFT		327
#define KEY_CTRL_RIGHT		328
#define KEY_CTRL_END		329
#define KEY_CTRL_PGDN		330
#define KEY_CTRL_HOME		331
#define KEY_CTRL_PGUP		332

#define KEY_ALT_A			333
#define KEY_ALT_B			334
#define KEY_ALT_C			335
#define KEY_ALT_D			336
#define KEY_ALT_E			337
#define KEY_ALT_F			338
#define KEY_ALT_G			339
#define KEY_ALT_H			340
#define KEY_ALT_I			341
#define KEY_ALT_J			342
#define KEY_ALT_K			343
#define KEY_ALT_L			344
#define KEY_ALT_M			345
#define KEY_ALT_N			346
#define KEY_ALT_O			347
#define KEY_ALT_P			348
#define KEY_ALT_Q			349
#define KEY_ALT_R			350
#define KEY_ALT_S			351
#define KEY_ALT_T			352
#define KEY_ALT_U			353
#define KEY_ALT_V			354
#define KEY_ALT_W			355
#define KEY_ALT_X			356
#define KEY_ALT_Y			357
#define KEY_ALT_Z			358


#define NUT				1
#define WAIT_NONE			0x00
#define WAIT_BEFORE_RETURN	0x01
#define WAIT_AFTER_KEY		0x02

int			AbortWindow(
				char *windowTitle);

char		*AttachClient(
				char *remoteClientName);

int			BindProtocol(
				char *commandLine);

int			CheckTitle(
				char *windowTitle);

int			ClientScreenContainsPhrase(
				char *remoteClientName,
				char *phrase,
				...);

int			ClientScreenContainsString(
				char *remoteClientName,
				char *string,
				...);

int			ClientType(
				char *remoteClientName,
				int waitForResponse,
				char *format,
				...);

int			ClientTypeKey(
				char *remoteClientName,
				int waitForResponse,
				int key);

int			CmdType(
				char *previousText,
				char *string,
				...);

int			CmdTypeKey(
				char *previousText,
				int key);

int			DefinedWindowContainsPhrase(
				int handle,
				char *phrase,
				...);

int			DefinedWindowContainsString(
				int handle,
				char *string,
				...);

int			DefineWindow(
				int x,
				int y,
				int width,
				int height);

int			DefineWindowBetweenPrompts(
				void);

int			DisconnectFromClient(
				char *remoteClientName);

int			EscapeFromWindow(
				char *windowTitle);

int			FieldContainsValue(
				char *windowTitle,
				char *field,
				char *value);

char		*GetClientScreenText(
				char *remoteClientName);

char		*GetDefinedWindowSubText(
				int handle,
				char *string,
				int length,
				char *beforeText,
				char *afterText);

char		*GetDefinedWindowText(
				int handle);

char		*GetMessageText(
				int messageNumber);

int			GetMLIBErrorNumber(
				void);

int			GetReturnCodeOption(
				void);

int			GetReverseVideoText(
				char *windowTitle,
				char *string);

int			GetScreenHook(
				void);

char		*GetScreenSubText(
				char *string,
				int length,
				char *beforeText,
				char *afterText);

char		*GetScreenText(
				void);

int			LoadNLM(
				char *filename);

int			LogClientError(
				char *remoteClientName,
				char *string,
				int logOriginalError,
				int logScreenMemory,
				int terminateNLMs,
				int quit,
				...);

int			LogError(
				char *string,
				int logOriginalError,
				int logScreenMemory,
				int terminateNLMs,
				int quit,
				...);

int			Mark(
				char *windowTitle,
				char *windowItem,
				...);

int			MasterInit(
				int argc,
				char *argv[],
				char *Header,
				char *Comments,
				char *programName,
				char *msgFileName);

int			MultipleSelect(
				char *windowTitle,
				char *windowItem,
				...);

int			PositionInWindow(
				char *windowTitle,
				char *windowItem);

int			PutCursorAtField(
				char *windowTitle,
				char *field);

void		RegisterCTestScriptNLM(
				void);

int			RegisterScreen(
				char *screenName);

int			RegisterScreenAndNLM(
				char *screenName,
				char *fileName);

int			ScreenContainsPhrase(
				char *phrase,
				...);

int			ScreenContainsString(
				char *string,
				...);

int			SelectFromWindow(
				char *windowTitle,
				char *windowItem);

int			SetEditScreenText(
				char *windowTitle,
				char *data,
				...);

int			SetFieldValue(
				char *windowTitle,
				char *field,
				char *value,
				char *finalValue);

void		SetReturnCodeOption(
				int returnCodeOption);

void		SetScreenHook(
				int screenHook);

int			TryToPutCursorAtField(
				char *field);

int			Type(
				char *format,
				int kbCheck,
				...);

int			TypeKey(
				int key,
				int kbCheck);

int			UnBindProtocol(
				char *commandLine);

int			UnDefineWindow(
				int handle);

int			UnLoadNLM(
				char *fileName);

int			WindowContainsField(
				char *windowTitle,
				char *field);

int			WindowContainsPhrase(
				char *windowTitle,
				char *phrase);

int			WindowContainsString(
				char *windowTitle,
				char *string);

int			WindowType(
				char *windowTitle,
				char *format,
				...);

int			WindowTypeKey(
				char *windowTitle,
				int key);

/* Return codes */

#define MLIBErrno GetMLIBErrorNumber()

#define ERR_INVALID_MESSAGE_FILE				0x01		/*  1 */
#define ERR_INVALID_MESSAGE_FILE_VERSION		0x02		/*  2 */
#define ERR_LOADNLM_FAILURE					0x03		/*  3 */
#define ERR_UNABLE_TO_ALLOCATE_MEMORY			0x04		/*  4 */
#define ERR_NO_NLMS_REGISTERED				0x05		/*  5 */
#define ERR_NO_NLMS_LOADED					0x06		/*  6 */
#define ERR_NLM_NOT_LOADED					0x06		/*  6 */
#define ERR_TEST_NLM_TERMINATED				0x07		/*  7 */
#define ERR_INVALID_SCREENNAME				0x08		/*  8 */
#define ERR_WRONG_WINDOW_TITLE				0x09		/*  9 */
#define ERR_WINDOW_ITEM_DOESNT_EXIST			0x0A		/* 10 */
#define ERR_NLM_NOT_UNLOADED					0x0B		/* 11 */
#define ERR_CLIENT_NOT_ATTACHED				0x0C		/* 12 */
#define ERR_CLIENT_NO_REPLY					0x0D		/* 13 */
#define ERR_REMOTE_CLIENT_PATTERN_NOT_FOUND		0x0E		/* 14 */
#define ERR_CLIENT_CONNECTION_FAILED			0x0F		/* 15 */
#define ERR_UNABLE_TO_CREATE_LOG_FILE			0x10		/* 16 */
#define ERR_POSITION_FIELD_DISPLAY_ONLY		0x11		/* 17 */
#define ERR_WINDOW_LABEL_DOES_NOT_EXIST		0x12		/* 18 */
#define ERR_POSITION_FIELD_FAILURE			0x13		/* 19 */
#define ERR_SETFIELD_FAILURE					0x14		/* 20 */
#define ERR_MARK_NOT_AVAILABLE				0x15		/* 21 */
#define ERR_INVALID_WINDOW_HANDLE				0x16		/* 22 */
#define ERR_PROMPT_NOT_MATCH					0x17		/* 23 */
#define ERR_LAN_PROTOCOL_NAME_MISSING			0x18		/* 24 */
#define ERR_LAN_PROTOCOL_MODULE_NOT_LOADED		0x19		/* 25 */
#define ERR_LAN_DRIVER_NAME_MISSING			0x1A		/* 26 */
#define ERR_LAN_DRIVER_NOT_LOADED				0x1B		/* 27 */
#define ERR_LAN_DRIVER_NOT_USING_CONFIG		0x1C		/* 28 */
#define ERR_MULTIPLE_BOARDS_USING_LAN_DRIVER	0x1D		/* 29 */
#define ERR_DMA_NUMBER_MISSING				0x1E		/* 30 */
#define ERR_PORT_ADDRESS_MISSING				0x1F		/* 31 */
#define ERR_MEMORY_ADDRESS_MISSING			0x20		/* 32 */
#define ERR_INTERRUPT_NUMBER_MISSING			0x21		/* 33 */
#define ERR_SLOT_NUMBER_MISSING				0x22		/* 34 */
#define ERR_SYNTAX_ERROR_IN_CONFIG_INFO		0x23		/* 35 */
#define ERR_LAN_PROTOCOL_ALREADY_BOUND			0x24		/* 36 */
#define ERR_OEM_LAN_DRIVER_MUST_BE_BOUND		0x25		/* 37 */
#define ERR_BIND_FAILURE					0x26		/* 38 */
#define ERR_LAN_PROTOCOL_NOT_BOUND			0x27		/* 39 */
#define ERR_UNBIND_FAILURE					0x28		/* 40 */
#define ERR_INVALID_PARAMETER				0x29		/* 41 */


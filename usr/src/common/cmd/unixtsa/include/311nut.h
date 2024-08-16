/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/311nut.h	1.1"
/****************************************************************************
 *
 * Program Name:  386 server nut interface
 *
 * Filename:      nut.h
 *
 * Date Created:  
 *
 * Version:       1.0
 *
 ****************************************************************************/

#include "sysmsg.mlh"

/* date/time fields */
#define DATE_WIDTH 22
#define TIME_WIDTH 22

/**** date field data structure ****/
typedef struct {
    WORD year;
    WORD month;
    WORD day;
} DATE;

/**** time field data structure ****/
typedef struct {
    WORD hour;
    WORD minute;
    WORD second;
} TIME;

/****************************************************************************/
/* constants */
#define SAVE                  1
#define NO_SAVE               0
#define NOHEADER              0
#define NOBORDER              0
#define NO_HELP_CONTEXT       0xffff
#define SINGLE                1
#define DOUBLE                2
#define CURSOR_OFF            0
#define CURSOR_ON             1
#define VIRTUAL               0
#define INFORM                1
#define WARNING               2
#define FATAL                 3
#define EDIT_TEXT_LINE_LENGTH 100

/* video constants */
#define V_UP         6
#define V_DOWN       7
#define LINE_OFFSET  160
#define EXPLODE_RATE 45
#define SCREEN_SPEED 0

/* video attributes */
#define VNORMAL     0
#define VINTENSE    1
#define VREVERSE    2
#define VBLINK      3
#define VIBLINK     4
#define VRBLINK     5

/* palettes */
#define NORMAL_PALETTE  1
#define INIT_PALETTE    2
#define HELP_PALETTE    3
#define ERROR_PALETTE   4
#define WARNING_PALETTE 5
#define OTHER_PALETTE   6

/* header types */
#define NO_HEADER         0
#define NORMAL_HEADER     2
#define LARGE_HEADER      3
#define VERY_LARGE_HEADER 4

/* keyboard constants */
#define KS_OFF   0
#define KS_ON    1
#define KS_INT   2

#define K_NORMAL 0

#define K_F1     1
#define K_F2     2
#define K_F3     3
#define K_F4     4
#define K_F5     5
#define K_F6     6
#define K_F7     7
#define K_F8     8
#define K_F9     9
#define K_F10    10

#define K_SF1    11
#define K_SF2    12
#define K_SF3    13
#define K_SF4    14
#define K_SF5    15
#define K_SF6    16
#define K_SF7    17
#define K_SF8    18
#define K_SF9    19
#define K_SF10   20

#define K_CF1    21
#define K_CF2    22
#define K_CF3    23
#define K_CF4    24
#define K_CF5    25
#define K_CF6    26
#define K_CF7    27
#define K_CF8    28
#define K_CF9    29
#define K_CF10   30

#define K_AF1    31
#define K_AF2    32
#define K_AF3    33
#define K_AF4    34
#define K_AF5    35
#define K_AF6    36
#define K_AF7    37
#define K_AF8    38
#define K_AF9    39
#define K_AF10   40

#define K_HELP   1
#define K_MODIFY 3
#define K_MARK   5
#define K_CANCEL 7
#define K_MODE   9
#define K_EXIT   40

#define K_ESCAPE 41        
#define K_BACK   42
#define K_INSERT 43
#define K_DELETE 44
#define K_SELECT 45
#define K_CYCLE  46
#define K_UP     47
#define K_DOWN   48
#define K_LEFT   49
#define K_RIGHT  50
#define K_SUP    51
#define K_SDOWN  52
#define K_SLEFT  53
#define K_SRIGHT 54
#define K_PUP    55
#define K_PDOWN  56
#define K_FRIGHT 57
#define K_FLEFT  58

/* available action keys for list */
#define M_ESCAPE  0x0001
#define M_INSERT  0x0002
#define M_DELETE  0x0004
#define M_MODIFY  0x0008
#define M_SELECT  0x0010
#define M_MDELETE 0x0020
#define M_CYCLE   0x0040
#define M_MMODIFY 0x0080
#define M_MSELECT 0x0100

/* return values for EditString */
#define E_ESCAPE  1
#define E_SELECT  2
#define E_EMPTY   4
#define E_CHANGE  8

/* type values for EditString */
#define EF_ANY      1L
#define EF_DECIMAL  2L
#define EF_HEX      4L
#define EF_NOSPACES 8L
#define EF_UPPER    0x10L
#define EF_DATE     0x20L
#define EF_TIME     0x40L
#define EF_FLOAT    0x80L
#define EF_SET      0x100L
#define EF_NOECHO   0x200L

/* form character macros */
extern BYTE *charTable;
#define F_H1    charTable[0]
#define F_H2    charTable[1]
#define F_V1    charTable[2]
#define F_V2    charTable[3]

#define F_UL1   charTable[4]
#define F_UR1   charTable[5]
#define F_LL1   charTable[6]
#define F_LR1   charTable[7]

#define F_UL2   charTable[8]
#define F_UR2   charTable[9]
#define F_LL2   charTable[10]
#define F_LR2   charTable[11]

#define F_UT1   charTable[12]
#define F_DT1   charTable[13]
#define F_LT1   charTable[14]
#define F_RT1   charTable[15]

#define F_UT12  charTable[16]
#define F_DT12  charTable[17]
#define F_LT12  charTable[18]
#define F_RT12  charTable[19]

#define F_UT21  charTable[20]
#define F_DT21  charTable[21]
#define F_LT21  charTable[22]
#define F_RT21  charTable[23]

#define F_UT2   charTable[24]
#define F_DT2   charTable[25]
#define F_LT2   charTable[26]
#define F_RT2   charTable[27]

#define F_UL12  charTable[28]
#define F_UR12  charTable[29]
#define F_LL12  charTable[30]
#define F_LR12  charTable[31]

#define F_UL21  charTable[31]
#define F_UR21  charTable[33]
#define F_LL21  charTable[34]
#define F_LR21  charTable[35]

#define F_X1    charTable[36]
#define F_X12   charTable[37]
#define F_X21   charTable[38]
#define F_X2    charTable[39]

#define F_UP    charTable[40]
#define F_DOWN  charTable[41]
#define F_LEFT  charTable[42]
#define F_RIGHT charTable[43]

#define F_BG    charTable[44]

/* form constants */
#define F_NOVERIFY  0x00
#define F_VERIFY    0x10
#define F_FORCE     0x20

/**** fieldFlags Type masks ****/

#define NORMAL_FIELD     0x00        /* normal editable field */
#define LOCKED_FIELD     0x01        /* non accessable */
#define SECURE_FIELD     0x02        /* non editable */
#define REQUIRED_FIELD   0x04        /* verify field on form exit */
#define HIDDEN_FIELD     0x09        /* hidden fields are also locked */
#define PROMPT_FIELD     0x11        /* prompt fields are also locked */
#define FORM_DESELECT    0x20        /* flag to cause form deselection
                                         * before action & verify routines
                                         * are called */
#define NO_FORM_DESELECT 0x00        /* In case old flag was used */
#define DEFAULT_FORMAT   0x00        /* normal field controlled justify */
#define RIGHT_FORMAT     0x40        /* right justification format */
#define LEFT_FORMAT      0x80        /* left justification format */
#define CENTER_FORMAT    0xC0        /* centering format */

#define IsKanji(code)    (((code >= 0x81) && (code <= 0x9f)) || ((code >= 0xe0) && (code <= 0xfc)))

/****************************************************************************/
/* process environment */

#define MAXPORTALS   15
#define MAXLISTS     11
#define SAVELISTS    6
#define MAXFUNCTIONS 41
#define MAXACTIONS   60
#define MAXHELP      30
#define NO_MESSAGE            0xffff
#define DYNAMIC_MESSAGE_ONE   0xfffe
#define DYNAMIC_MESSAGE_TWO   0xfffd
#define DYNAMIC_MESSAGE_THREE 0xfffc
#define SYSTEM_MESSAGE        0x8000

typedef struct
{
    BYTE frameLine;
    BYTE frameColumn;
    BYTE frameHeight;
    BYTE frameWidth;
    BYTE virtualHeight; /* width of virtual screen */
    BYTE virtualWidth;  /* height of virtual screen */
    BYTE cursorState;
    BYTE borderType;
    BYTE borderAttribute;
    BYTE saveFlag;
    BYTE directFlag;
    BYTE headerAttribute;
    BYTE *headerText;
    BYTE *virtualScreen;
    BYTE *saveScreen;
    BYTE portalLine;    /* top-most line of portal */
    BYTE portalColumn;  /* right-most column of portal */
    BYTE portalHeight;
    BYTE portalWidth;
    BYTE virtualLine;   /* position of portal over virtual portal */
    BYTE virtualColumn; /* position of portal over virtual portal */
    BYTE cursorLine;
    BYTE cursorColumn;
    BYTE firstUpdateFlag;
    struct ScreenStruct *screenID;
} PCB;

typedef struct LIST_STRUCT
{
    BYTE marked;
    struct LIST_STRUCT *prev;
    struct LIST_STRUCT *next;
    void *otherInfo;
    BYTE text[1];
} LIST;

typedef struct
{
    LIST *head;
    LIST *tail;
    int (*sortProc)();
    void (*freeProcedure)(void *memoryPointer);
} LISTPTR;

typedef struct
{
    BYTE *dynamicMessageOne;
    BYTE *dynamicMessageTwo;
    BYTE *dynamicMessageThree;
    BYTE **programMesgTable;
} MessageInfo;

typedef struct
{
    void (*interruptProc)(void *handle);
    BYTE key;
} INTERRUPT;

typedef struct
{
    short (*listAction)(int option, void *parameter);
    void *parameter;
} MENU_PARAMETER;

/* environment structure */
typedef struct
{
    PCB    *portal[MAXPORTALS];
    BYTE    currentPortal;
    BYTE    headerHeight;
    BYTE    waitFlag;
    LISTPTR listStack[MAXLISTS];
    LISTPTR saveStack[SAVELISTS];
    BYTE    nextAvailList;
    LIST   *head, *tail;
    int   (*defaultCompareFunction)(LIST *el1, LIST *el2);
    void  (*freeProcedure)(void *memoryPointer);
    void  (*interruptTable[MAXFUNCTIONS])();
    BYTE    functionKeyStatus[MAXACTIONS];
    MessageInfo messages;
    WORD    helpContextStack[MAXHELP];
    WORD    currentPreHelpMessage;
    int     freeHelpSlot;
    BYTE    redisplayFormFlag;
    BYTE    preHelpPortal;
    BYTE    waitPortal;
    BYTE    errorPortal;
    struct ResourceTagStructure *resourceTag;
    struct ScreenStruct *screenID;
    BYTE   *helpScreens;
    int     helpOffset;
    WORD    helpHelp;
}  NUTInfo;

typedef struct fielddef {
    LIST *element;               /* list element that owns the field */
    WORD fieldFlags;             /* Control flags
                                  * bit 0 is locked field
                                  * bit 1 is secure field
                                  * bit 2 is required entry field
                                  * bit 3 is hidden field
                                  * bit 4 is prompt field
                                  * bit 5 is form deselect flag
                                  * bit 6 & 7 control formatting
                                  * 00 = default format
                                  * 01 = right justify (bit 6 set)
                                  * 10 = left justify  (bit 7 set)
                                  * 11 = center
                                  */
                                        
    BYTE fieldLine;              /* Line where field is located */
    BYTE fieldColumn;            /* Column where field is located */
    BYTE fieldWidth;             /* Maximum width of field */
    BYTE fieldAttribute;         /* Display attribute for field */
    int fieldActivateKeys;       /* Keys that will activate the field */

    void (* fieldFormat)(struct fielddef *field, BYTE *text, int buffLen);
                                 /* Routine called when field selected */
    BYTE (* fieldControl)(struct fielddef *field, int selectKey,
            int *fieldChanged, NUTInfo *handle);
                                        /* Routine to verify Input */
    int (* fieldVerify)(struct fielddef *field, BYTE *data, NUTInfo *handle);
    void (* fieldRelease)(struct fielddef *);     /* Data & Xtra field release routine */

    BYTE *fieldData;             /* Pointer to data */
    BYTE *fieldXtra;             /* Additional control info */
    int fieldHelp;               /* help context for this field */
    struct fielddef *fieldAbove; /* Pointer to field above */
    struct fielddef *fieldBelow; /* Pointer to field below */
    struct fielddef *fieldLeft;  /* Pointer to field to left */
    struct fielddef *fieldRight; /* Pointer to field to right */
    struct fielddef *fieldPrev;  /* Pointer to previous field */
    struct fielddef *fieldNext;  /* Pointer to next field */
} FIELD;

/* macros */
#define Mark(element)     ((element)->marked |= 1)
#define Unmark(element)   ((element)->marked &= 0xfe)
#define IsMarked(element) ((element)->marked & 1)

/* global variables */
extern WORD screenHeight, screenWidth;

/****************************************************************************/
/* low level nut calls */

extern WORD InitDisplay(
    struct ScreenStruct *screenID);

extern void RestoreDisplay(
    struct ScreenStruct *screenID);

extern void ScreenSize(
    WORD *maxLines,
    WORD *maxColumes);

extern void EnableCursor(
    struct ScreenStruct *screenID);

extern void DisableCursor(
    struct ScreenStruct *screenID);

extern WORD ReadCursorPosition(
    WORD *line,
    WORD *column,
    struct ScreenStruct *screenID);

extern void PositionCursor(
    WORD line,
    WORD column,
    struct ScreenStruct *screenID);

extern void ShowLine(
    WORD line,
    WORD column,
    BYTE *text,
    BYTE length,
    struct ScreenStruct *screenID);

extern void ShowLineAttribute(
    WORD line,
    WORD column,
    BYTE *text,
    BYTE attribute,
    BYTE length,
    struct ScreenStruct *screenID);

extern void ScrollZone(
    WORD line,
    WORD column,
    BYTE height,
    BYTE width,
    BYTE attribute,
    BYTE count,
    BYTE direction,
    struct ScreenStruct *screenID);

extern void FillZone(
    WORD line,
    WORD column,
    BYTE height,
    BYTE width,
    BYTE fillChar,
    BYTE attribute,
    struct ScreenStruct *screenID);

extern void FillZoneAttribute(
    WORD line,
    WORD column,
    BYTE height,
    BYTE width,
    char attribute,
    struct ScreenStruct *screenID);

extern void SaveZone(
    WORD line,
    WORD column,
    BYTE height,
    BYTE width,
    BYTE *buffer,
    struct ScreenStruct *screenID);

extern void RestoreZone(
    WORD line,
    WORD column,
    BYTE height,
    BYTE width,
    BYTE *buffer,
    struct ScreenStruct *screenID);

/****************************************************************************/
/* portal stuff */

extern void ShowPortalLine(
    BYTE line,
    BYTE column,
    BYTE *text,
    BYTE length,
    PCB *portal);

extern void ShowPortalLineAttribute(
    BYTE line,
    BYTE column,
    BYTE *text,
    BYTE attribute,
    BYTE length,
    PCB *portal);

extern void ScrollPortalZone(
    BYTE line,
    BYTE column,
    BYTE height,
    BYTE width,
    BYTE attribute,
    BYTE count,
    BYTE direction,
    PCB *portal);

extern void FillPortalZone(
    BYTE line,
    BYTE column,
    BYTE height,
    BYTE width,
    BYTE fillCharacter,
    BYTE fillAttribute,
    PCB *portal);

extern void FillPortalZoneAttribute(
    BYTE line,
    BYTE column,
    BYTE height,
    BYTE width,
    char attribute,
    PCB *portal);

extern BYTE *GetMessage(
    WORD message,
    MessageInfo *messages);

extern BYTE *GetSystemMessage(
    WORD message);

extern void SetDynamicMessage(
    WORD message,
    BYTE *text,
    MessageInfo *messages);

extern void InitNut(
    WORD utility,
    WORD version,
    WORD headerType,
    BYTE **messageTable,
    BYTE *helpScreens,
    struct ScreenStruct *screenID,
    struct ResourceTagStructure *resourceTag,
    NUTInfo *handle);

extern void RestoreNut(
    NUTInfo *handle);

extern BYTE CreatePortal(
    BYTE line,
    BYTE column,
    BYTE frameHeight,
    BYTE frameWidth,
    BYTE virtualHeight,
    BYTE virtualWidth,
    BYTE saveFlag,
    BYTE *headerText,
    BYTE headerAttribute,
    BYTE borderType,
    BYTE borderAttribute,
    BYTE cursorFlag,
    BYTE directFlag,
    NUTInfo *nutInfo);

extern void DestroyPortal(
    BYTE portalNumber,
    NUTInfo *handle);
    
extern void DrawPortalBorder(
    PCB *portal);

extern void UpdatePortal(
    PCB *portal);

extern void PositionPortalCursor(
    BYTE line,
    BYTE column,
    PCB *portal);

extern void EnablePortalCursor(
    PCB *portal);

extern void DisablePortalCursor(
    PCB *portal);

extern void DrawBox(
    BYTE top,
    BYTE bottom,
    BYTE left,
    BYTE right,
    BYTE fillCharacter,
    BYTE attribute,
    struct ScreenStruct *screenID);

extern void SetPalette(
    BYTE newPalette,
    struct ScreenStruct *screenID);

extern void DeselectPortal(
    NUTInfo *handle);

extern void SelectPortal(
    BYTE portalNumber,
    NUTInfo *handle);

extern WORD ComputePortalPosition(
    BYTE centerLine,
    BYTE centerColumn,
    BYTE height,
    BYTE width,
    BYTE *line,
    BYTE *column,
    NUTInfo *handle);

extern void ClearPortal(
    PCB *portal);

/****************************************************************************/
/* list routines */

extern int CaselessCompare(
    BYTE *s1,
    BYTE *s2,
    int len);
    
extern int DefaultCompare(
    LIST *el1,
    LIST *el2);

extern void InitList(
    NUTInfo *handle,
    void (*freeRoutine)(void *memoryPointer));

extern void InitMenu(
    NUTInfo *handle);

extern void InitForm(
    NUTInfo *handle);

extern WORD PushList(
    NUTInfo *handle);

extern WORD PopList(
    NUTInfo *handle);

extern WORD SaveList(
    WORD listIndex,
    NUTInfo *handle);

extern WORD RestoreList(
    WORD listIndex,
    NUTInfo *handle);

/* #if !defined(DEBUG_CODE) and !defined(IN_HOUSE) */
extern void DestroyList(
    NUTInfo *handle);

extern void DestroyListPtr(
    LISTPTR *list);
/* #endif */

extern LIST *AppendToList(
    BYTE *text,
    void *otherInfo,
    NUTInfo *handle);

extern LIST *AppendToListPtr(
    LISTPTR *list,
    BYTE *text,
    void *otherInfo,
    NUTInfo *handle);

extern LIST *DeleteFromList(
    LIST *el,
    NUTInfo *handle);

extern LIST *InsertInList(
    BYTE *text,
    BYTE *otherInfo,
    LIST *atElement,
    NUTInfo *handle);

extern LIST *GetListHead(
    NUTInfo *handle);

extern LIST *GetListHeadPtr(
    LISTPTR *listPtr);

extern LIST *GetListTail(
    NUTInfo *handle);

extern void UnmarkList(
    NUTInfo *handle);

extern void SetList(
    LISTPTR *listPtr,
    NUTInfo *handle);

extern void GetList(
    LISTPTR *listPtr,
    NUTInfo *handle);

extern WORD IsAnyMarked(
    NUTInfo *handle);

extern WORD MultipleElementsMarked(
    LIST *element,
    NUTInfo *handle);

extern void PushMarks(
    NUTInfo *handle);

extern void PopMarks(
    NUTInfo *handle);

extern void SortList(
    NUTInfo *handle);

extern void QuickSortList(
    LIST *head,
    LIST *tail,
    NUTInfo *handle);

extern void InitListPtr(
    LISTPTR *listPtr,
    void (*freeRoutine)(void *memoryPointer));

/****************************************************************************/
/* keyboard routines */

extern void WaitForEscape(
    NUTInfo *handle);

extern int WaitForEscapeOrCancel(
    NUTInfo *handle);

extern void NUTGetKey(
    BYTE *type,
    BYTE *value,
    NUTInfo *handle);

extern WORD NUTUngetKey(
    BYTE type,
    BYTE value,
    NUTInfo *handle);

extern void EnableFunctionKey(
    BYTE key,
    NUTInfo *handle);

extern void DisableFunctionKey(
    BYTE key,
    NUTInfo *handle);

extern void EnableInterruptKey(
    BYTE key,
    void (*interruptProc)(void *handle),
    NUTInfo *handle);

extern void SaveFunctionKeyList(
    BYTE *keyList,
    NUTInfo *handle);

extern void EnableFunctionKeyList(
    BYTE *keyList,
    NUTInfo *handle);

extern void SaveInterruptList(
    INTERRUPT *interruptList,
    NUTInfo *handle);

extern void EnableInterruptList(
    INTERRUPT *interruptList,
    NUTInfo *handle);

extern void DisableAllInterruptKeys(
    NUTInfo *handle);

extern void DisableAllFunctionKeys(
    NUTInfo *handle);

extern void EnableAllFunctionKeys(
    NUTInfo *handle);

extern BYTE NUTKeyStatus(
    NUTInfo *handle);

/****************************************************************************/
/* middle level NUT functions */

extern void StartWait(
    BYTE centerLine,
    BYTE centerColumn,
    NUTInfo *handle);

extern void EndWait(
    NUTInfo *handle);

extern short CalcWidthAndHeight(
    BYTE messageLength,
    BYTE *text,
    BYTE maxDisplayWidth,
    BYTE maxDisplayHeight,
    int *newWidth,
    int *newHeight,
    int *splitWordFlag);

extern short DisplayTextInPortal(
    BYTE line,
    BYTE indentLevel,
    BYTE *text,
    BYTE attribute,
    PCB *portal);
        
extern short DisplayInformation(
    WORD header,
    BYTE pauseFlag,
    BYTE centerLine,
    BYTE centerColumn,
    BYTE palette,
    BYTE attribute,
    BYTE *displayText,
    NUTInfo *handle);

extern void Alert(
    BYTE centerLine,
    BYTE centerColumn,
    NUTInfo *handle,
    WORD message,
    ...);

extern void NUTTrace(
    NUTInfo *handle,
    BYTE *message,
    ...);

extern void DisplayErrorText(
    WORD message,
    BYTE severity,
    NUTInfo *handle,
    ...);

/****************************************************************************/
/* list related functions */

extern BYTE FormatMenuLine(
    LIST *element,
    BYTE skew,
    BYTE *textBuffer,
    BYTE width);

extern void DisplaySignals(
    LIST *element,
    BYTE line,
    BYTE height,
    NUTInfo *handle);

extern void DisplayList(
    BYTE line,
    LIST *element,
    BYTE skew,
    BYTE (*formatFun)(LIST *element, BYTE skew, BYTE *displayLine, BYTE width),
    BYTE markFlag,
    BYTE charIndex,
    NUTInfo *handle);

extern BYTE AlignListDisplay(
    LIST *oldElement,
    LIST *newElement,
    BYTE oldLine,
    NUTInfo *handle);

extern BYTE AlignChangedList(
    BYTE oldIndex,
    LIST *newElement,
    BYTE oldLine,
    NUTInfo *handle);

extern BYTE GetListIndex(
    LIST *element,
    NUTInfo *handle);

extern WORD SelectFromList(
    WORD validKeyFlags,
    BYTE *skew,
    LIST **element,
    BYTE (*formatFun)(LIST *element, BYTE skew, BYTE *text, BYTE width),
    int *curLine,
    NUTInfo *handle);

extern WORD List(
    WORD header,
    BYTE centerLine,
    BYTE centerColumn,
    BYTE height,
    BYTE width,
    WORD validKeyFlags,
    LIST **element,
    NUTInfo *handle,
    BYTE (*format)(LIST *element, BYTE skew, BYTE *displayLine, BYTE width),
    short (*action)(WORD, LIST **, int *, void *),
    void *actionParameter);

extern LIST *AppendToMenu(
    WORD message,
    int option,
    NUTInfo *handle);

extern int Menu(
    WORD header,
    BYTE centerLine,
    BYTE centerColumn,
    LIST *defaultElement,
    short (*action)(int option, void *parameter),
    NUTInfo *handle,
    void *actionParameter);

extern short NUTMenuAction(
    WORD action,
    LIST **element,
    int *line,
    void *listParameter);

extern int Confirm(
    WORD header,
    BYTE centerLine,
    BYTE centerColumn,
    int defaultChoice,
    short (*action)(int option, void *parameter),
    NUTInfo *handle,
    void *actionParameter);

extern int EditString(
    BYTE centerLine,
    BYTE centerColumn,
    BYTE editHeight,
    BYTE editWidth,
    WORD header,
    WORD prompt,
    BYTE *buf,
    BYTE maxLen,
    WORD type,
    NUTInfo *handle,
    int (*insertProc)(BYTE *buffer, int maxLen, void *parameters),
    int (*actionProc)(int action, BYTE *buffer, void *parameters),
    void *parameters);

extern int EditPortalString(
    BYTE *buf,
    BYTE maxLen,
    int type,
    int (*insertProc)(BYTE *buffer, int maxLen, void *parameters),
    PCB *framePortalInfo,
    PCB *p,
    NUTInfo *handle,
    void *parameters);

extern int InsertInPortalList(
    LIST **currentElement,
    int *currentLine,
    int (*InsertProcedure)(BYTE *text, void **otherInfo, void *parameters),
    void (*FreeProcedure)(void *otherInfo),
    NUTInfo *handle,
    void *parameters);

extern int ModifyInPortalList(
    LIST **currentElement,
    int *currentLine,
    int (*ModifyProcedure)(BYTE *text, void *parameters),
    NUTInfo *handle,
    void *parameters);

extern int DeleteFromPortalList(
    LIST **currentElement,
    int *currentLine,
    LIST *(*DeleteProcedure)(LIST *el, NUTInfo *handle, void *parameters),
    WORD deleteCurrentHeader,
    WORD deleteMarkedHeader,
    NUTInfo *handle,
    void *parameters);

/****************************************************************************/
/* help stuff */

extern int PushHelpContext(
    WORD helpContext,
    NUTInfo *handle);

extern int PopHelpContext(
    NUTInfo *handle);

extern void DisplayPreHelp(
    BYTE line,
    BYTE column,
    WORD message,
    NUTInfo *handle);

extern void RemovePreHelp(
    NUTInfo *handle);

extern void ProcessHelpKey(
    void *handle);

extern int GetHelpOffset(
    BYTE *helpScreens);

extern void DisplayHelpScreen(
    LONG offset,
    NUTInfo *handle);


/****************************************************************************/
/* form stuff */

extern void SetDefaultRelationships(
    NUTInfo *handle);

extern int EditPortalForm(
    WORD header,
    BYTE cline,
    BYTE ccol,
    BYTE formHeight,
    BYTE formWidth,
    WORD controlFlags,
    WORD formHelp,
    WORD confirmMessage,
    NUTInfo *handle);

extern void UpdateForm(
    FIELD **currentFormField,
    LIST **currentFormElement,
    int *lockedFormFlag,
    NUTInfo *handle);

extern int MoveToNextField(
    FIELD **field,
    int reverseIfDeadEnd);

extern int MoveToPreviousField(
    FIELD **field,
    int reverseIfDeadEnd);

extern int MoveToLeftField(
    FIELD **field,
    int reverseIfDeadEnd);

extern int MoveToRightField(
    FIELD **field,
    int reverseIfDeadEnd);

extern int MoveToAboveField(
    FIELD **field,
    int reverseIfDeadEnd);

extern int MoveToBelowField(
    FIELD **field,
    int reverseIfDeadEnd);

extern int EditForm(
    WORD headernum,
    BYTE line,
    BYTE col,
    BYTE portalHeight,
    BYTE portalWidth,
    BYTE virtualHeight,
    BYTE virtualWidth,
    int ESCverify,
    int forceverify,
    WORD confirmMessage,
    NUTInfo *handle);

extern FIELD *AppendToForm(
    BYTE fline,
    BYTE fcol,
    BYTE fwidth,
    BYTE fattr,
    void (* fFormat)(struct fielddef *field, BYTE *text, int buffLen),
    BYTE (* fControl)(struct fielddef *field, int selectKey,
            int *fieldChanged, NUTInfo *handle),
    int (* fVerify)(struct fielddef *field, BYTE *data, NUTInfo *handle),
    void (* fRelease)(struct fielddef *field),     /* Data & Xtra field release routine */
    BYTE *fData,
    BYTE *fXtra,
    WORD fflags,
    WORD fActivateKeys,
    WORD fhelp,
    NUTInfo *handle);

extern FIELD *AppendPromptField(
    BYTE line,
    BYTE column,
    WORD promptnum,
    NUTInfo *handle);

extern FIELD *AppendCommentField(
    BYTE line,
    BYTE column,
    BYTE *prompt,
    NUTInfo *handle);

extern void BoolFmt(
    FIELD *fieldPtr,
    BYTE *buffer,
    int bufLen);

extern void StringFmt(
    FIELD *fieldPtr,
    BYTE *buffer,
    int bufLen);

extern BYTE BoolEdit(
    FIELD *fp,
    int selectKey,
    int *cfp,
    NUTInfo *handle);

extern void IntegerFieldFmt(
    FIELD *fieldPtr,
    BYTE *buffer,
    int bufLen);

extern void HexFieldFmt(
    FIELD *fieldPtr,
    BYTE *buffer,
    int bufLen);

extern BYTE StringEdit(
    FIELD *fp,
    int selectKey,
    int *cfp,
    NUTInfo *handle);

extern BYTE IntegerFieldEdit(
    FIELD *fp,
    int selectKey,
    int *cfp,
    NUTInfo *handle);

extern BYTE HexFieldEdit(
    FIELD *fp,
    int selectKey,
    int *cfp,
    NUTInfo *handle);

extern BYTE IntegerFieldEdit(
    FIELD *fp,
    int selectKey,
    int *cfp,
    NUTInfo *handle);

extern int IntegerFieldVerify(
    FIELD *fp,
    BYTE *data,
    NUTInfo *handle);

extern int HexFieldVerify(
    FIELD *fp,
    BYTE *data,
    NUTInfo *handle);

extern int EditFormPortalString(
    BYTE *buf,
    BYTE maxLen,
    BYTE *charSet,
    FIELD *fp,
    PCB *p,
    NUTInfo *handle);

extern FIELD *AppendBoolField(
    BYTE line,
    BYTE column,
    WORD fflag,
    BYTE *data,
    WORD help,
    NUTInfo *handle);

extern FIELD *AppendStringField(
    BYTE line,
    BYTE column,
    BYTE width,
    WORD fflag,
    BYTE *data,
    BYTE *cset,
    WORD help,
    NUTInfo *handle);

extern FIELD *AppendIntegerField(
    BYTE line,
    BYTE column,
    WORD fflag,
    int *data,
    int minimum,
    int maximum,
    WORD help,
    NUTInfo *handle);

extern FIELD *AppendHexField(
    BYTE line,
    BYTE column,
    WORD fflag,
    int *data,
    int minimum,
    int maximum,
    WORD help,
    NUTInfo *handle);

extern FIELD *AppendHotSpotField(
    BYTE line,
    BYTE column,
    WORD fflag,
    BYTE *displayString,
    BYTE (* SpotAction)(FIELD *fp, int selectKey, int *changedField,
            NUTInfo *handle),
    NUTInfo *handle);

extern void NumberFieldFree(
    FIELD *fp);

/****************************************************************************/
/* misc */

extern BYTE NUTstrcat(
    BYTE *string,
    BYTE *newStuff);

extern void NUTmemmove(void *dest, void *source, int len);

#if !defined(_CTYPE_H_INCLUDED)
extern BYTE NUTtoupper(
    BYTE ch);

extern int NUTisalnum(
    BYTE ch);

extern int NUTisalpha(
    BYTE ch);

extern int NUTisdigit(
    BYTE ch);

extern int NUTisxdigit(
    BYTE ch);
#endif

extern int AsciiToInt(
    BYTE *data);

extern int AsciiHexToInt(
    BYTE *data);

extern WORD ConvertKanji(
    WORD JISCode);

extern void ConvertLine(
    BYTE *line,
    BYTE length);

/****************************************************************************/
/* edit text fuctions */

extern BYTE *FindNextLine(
    BYTE *currentLine);

extern BYTE *FindPreviousLine(
    BYTE *firstLine,
    BYTE *currentLine);

extern BYTE *FindLine(
    int lineNumber,
    BYTE *firstLine);

extern int GetLineLength(
    BYTE *line);

extern int MoveCursorAndPortal(
    int vLine,
    int vColumn,
    int editHeight,
    int editWidth,
    BYTE **displayStart,
    int  *startLine,
    BYTE *buffer,
    PCB *portal);

extern void CheckLineLength(
    BYTE *beginning);

extern int EditText(
    BYTE centerLine,
    BYTE centerColumn,
    int height,
    int width,
    WORD headerNumber,
    BYTE *textBuffer,
    int maxBufferLength,    /* length of document */
    WORD confirmMessage,
    int forceConfirm,
    NUTInfo *handle);

/****************************************************************************/
/* disk functions */

extern BYTE GetADisk(
    BYTE *volName,
    BYTE *prompt,
    NUTInfo *handle);

extern int NUTFindFile(
    BYTE *fileName,
    BYTE *volumeName,
    BYTE *diskDescription,
    BYTE *fileLocation,
    NUTInfo *handle);

/****************************************************************************/
/* date/time functions */

extern FIELD *AppendDateField(
    BYTE line,
    BYTE column,
    int fflag,
    DATE *date,
    WORD help,
    NUTInfo *handle);

extern FIELD *AppendTimeField(
    BYTE line,
    BYTE column,
    int fflag,
    TIME *time,
    WORD help,
    NUTInfo *handle);

extern BYTE DateEdit(
    FIELD *fp,
    int selectKey,
    int *ffp,
    NUTInfo *handle);

extern void DateFmt(
    FIELD *fieldptr,
    BYTE *buffer,
    int buffLen);

extern BYTE DateTimeEdit(
    BYTE *buffer,
    FIELD *fp,
    int selectKey,
    int *cfp,
    NUTInfo *handle);

extern int DateVerify(
    FIELD *fieldPtr,
    BYTE *buffer,
    NUTInfo *handle);

extern int MatchDate(
    BYTE *dateString,
    DATE *date);

extern int MatchTime(
    BYTE *timeString,
    TIME *time);

extern BYTE TimeEdit(
    FIELD *fp,
    int selectKey,
    int *ffp,
    NUTInfo *handle);

extern void TimeFmt(
    FIELD *fieldptr,
    BYTE *buffer,
    int buffLen);

extern int TimeVerify(
    FIELD *fieldPtr,
    BYTE *buffer,
    NUTInfo *handle);

extern void NUTGetDate(
    WORD *year,
    WORD *month,
    WORD *day,
    WORD *dayOfWeek);

extern void NUTGetTime(
    WORD *hour,
    WORD *minute,
    WORD *second,
    WORD *hundredth);

/****************************************************************************/
/****************************************************************************/


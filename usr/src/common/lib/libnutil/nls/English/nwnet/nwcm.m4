LCOM "@(#)cmd-nw:nls/English/nwcm.m4	1.2"
LCOM "$Id: nwcm.m4,v 1.6 1994/08/30 15:29:15 mark Exp $"
LCOM
LCOM Copyright 1992, 1993 Unpublished Work of Novell, Inc.
LCOM All Rights Reserved.
LCOM
LCOM This work is an unpublished work and contains confidential,
LCOM proprietary and trade secret information of Novell, Inc. Access
LCOM to this work is restricted to (I) Novell employees who have a
LCOM need to know to perform tasks within the scope of their
LCOM assignments and (II) entities other than Novell who have
LCOM entered into appropriate agreements.
LCOM 
LCOM No part of this work may be used, practiced, performed,
LCOM copied, distributed, revised, modified, translated, abridged,
LCOM condensed, expanded, collected, compiled, linked, recast,
LCOM transformed or adapted without the prior written consent
LCOM of Novell.  Any use or exploitation of this work without
LCOM authorization could subject the perpetrator to criminal and
LCOM civil liability.
LCOM
SET(MSG_NWCM_SET,3)
define(`Module_Name', `NWCM')
define(`Module_Version', `2.0')
LCOM
LCOM ************************************************************************
LCOM NETWARE CONFIGURATION MANAGEMENT LIBRARY ERROR CODES: 1 through 100
LCOM ************************************************************************
LCOM
INFORM_STR_TAG(`NWCM_NOT_FOUND', 1, `The specified configuration parameter is undefined.')
INFORM_STR_TAG(`NWCM_INVALID_TYPE', 2, `An invalid type was specified for the named configuration parameter.')
INFORM_STR_TAG(`NWCM_INVALID_DATA', 3, `The new data supplied for the configuration parameter was not valid.')
INFORM_STR_TAG(`NWCM_NOT_INITIALIZED', 4, `The NetWare configuration subsystem has not been initialized (an internal failure in the subsystem).')
INFORM_STR_TAG(`NWCM_INIT_FAILED', 5, `The NetWare configuration subsystem failed to initialize properly.')
INFORM_STR_TAG(`NWCM_SYNTAX_ERROR', 6, `A syntax error was detected in the NetWare configuration file.')
INFORM_STR_TAG(`NWCM_CONFIG_READ_ONLY', 7, `Insufficient permission to update the NetWare system configuration.')
INFORM_STR_TAG(`NWCM_SYSTEM_ERROR', 8, `An operating system error caused the configuration subsystem request to fail.')
INFORM_STR_TAG(`NWCM_CONFIG_NOT_LOCKED', 9, `The configuration subsystem was not locked (an internal failure in the subsystem).')
INFORM_STR_TAG(`NWCM_LOCK_FAILED', 10, `The NetWare configuration subsystem was unable to exclusively lock the configuration file.')
INFORM_STR_TAG(`NWCM_UNLOCK_FAILED', 11, `The NetWare configuration subsystem was unable to unlock the configuration file.')
INFORM_STR_TAG(`NWCM_NOT_IMPLEMENTED', 12, `The requested NetWare configuration operation is not implemented.')
LCOM
LCOM ************************************************************************
LCOM NETWARE CONFIGURATION MANAGEMENT UTILITY ERROR MESSAGES: 101 and up.
LCOM ************************************************************************
LCOM
INFORM_STR_TAG(NWCM_CMD_UNKNOWN_OPTION, 101, `Unknown option: %1$c.\n')
INFORM_STR_TAG(NWCM_CMD_ILLEGAL_HELP_OPTION, 102, `Help options may not be combined with any other options, and must be called one at a time.\n')
INFORM_STR_TAG(NWCM_CMD_FOLDER_OUT_OF_RANGE, 103, `Folder number %1$d is out of range.  Highest folder number is %2$d.\n')
INFORM_STR_TAG(NWCM_CMD_LOOKUP_FAILED, 104, `Unable to find parameter \"%1$s\".\n')
INFORM_STR_TAG(NWCM_CMD_SETTODEFAULT_FAILED, 105, `Unable to reset parameter \"%1$s\" to the default value.\n')
INFORM_STR_TAG(NWCM_CMD_MISSING_VALUE_ON_SET, 106, `Could not find an \"=\" to denote the value for parameter value assignment.\n')
INFORM_STR_TAG(NWCM_CMD_SETPARAM_FAILED, 107, `Unable to set parameter \"%1$s\" to \"%2$s\".\n')
INFORM_STR_TAG(NWCM_CMD_UNKNOWN_ARGUMENT, 108, `Do not know to handle argument \"%1$s\"\n')
INFORM_STR_TAG(NWCM_CMD_OPTION_REQ_PARAM, 109, `Option %1$c requires a parameter.\n')
INFORM_STR_TAG(NWCM_CMD_MISSING_VALUE_ON_VALIDATE, 110, `Could not find an equal sign to denote the value for parameter value validation test.\n')
INFORM_STR_TAG(NWCM_CMD_VALIDATEPARAM_FAILED, 111, `Value \"%2$s\" invalid for parameter \"%1$s\".\n')
INFORM_STR(NWCM_CMD_USAGE_1, 112, `\nUsage: %1$s [ -[qCcF] ] [ -[srvhd] param ] [ -[RVHDLf] folder ] ...\n')
INFORM_STR(NWCM_CMD_USAGE_2, 113, `\nOptions:\n\n    -q             When setting or resetting parameter values, do not\n                   print out the new value.\n')
INFORM_STR(NWCM_CMD_USAGE_3, 114, `\n    -s param=value Set the parameter to the indicated value.\n')
INFORM_STR(NWCM_CMD_USAGE_4, 115, `\n    -L folder      List all parameters in folder without displaying\n                   their current values.\n')
INFORM_STR(NWCM_CMD_USAGE_5, 116, `\n    -v param       View the value of a parameter.\n')
INFORM_STR(NWCM_CMD_USAGE_6, 117, `\n    -V folder      View the values of all parameters in a folder.\n')
INFORM_STR(NWCM_CMD_USAGE_7, 118, `\n    -r param       Reset the value of a parameter to the default\n                   value.\n')
INFORM_STR(NWCM_CMD_USAGE_8, 119, `\n    -R folder      Reset the values for all parameters in a folder to\n                   their default values.\n')
INFORM_STR(NWCM_CMD_USAGE_9, 120, `\n    -d param       Display the description string for a parameter from\n                   the message catalog.\n')
INFORM_STR(NWCM_CMD_USAGE_10, 121, `\n    -D folder      Display the description strings for all parameters\n                   in a folder from the message catalog.\n')
INFORM_STR(NWCM_CMD_USAGE_11, 122, `\n    -h param       Display the help string for a parameter from the\n                   message catalog.\n')
INFORM_STR(NWCM_CMD_USAGE_12, 123, `\n    -H folder      Display the help strings for all parameters in a\n                   folder from the message catalog.\n')
INFORM_STR(NWCM_CMD_USAGE_13, 124, `\n    -f folder      Display the description string for a folder from the\n                   message catalog.\n')
INFORM_STR(NWCM_CMD_USAGE_14, 125, `\n    -c             Display the path name of the configuration file.\n')
INFORM_STR(NWCM_CMD_USAGE_15, 126, `\n    -C             Display the path name of the configuration\n                   directory.\n')
INFORM_STR(NWCM_CMD_USAGE_16, 127, `\n    -F             Display the number of folders\n')
INFORM_STR(NWCM_CMD_USAGE_17, 128, `\n    -t param=value Test to see if value valid for parameter.\n')
INFORM_STR(NWCM_CMD_USAGE_18, 129, `\n    -x             Remove bad or non-existent parameters from the\n                   configuration file.\n')
INFORM_STR_TAG(NWCM_XCMD_UNKNOWN_FILE_MENU_OPTION, 131, `Internal Error: Unknown option (%1$d) passed to File menu callback.\n')
INFORM_STR_TAG(NWCM_XCMD_UNKNOWN_EDIT_MENU_OPTION, 132, `Internal Error: Unknown option (%1$d) passed to Edit menu callback.\n')
INFORM_STR_TAG(NWCM_XCMD_UNKNOWN_VIEW_MENU_OPTION, 133, `Internal Error: Unknown option (%1$d) passed to View menu callback.\n')
INFORM_STR_TAG(NWCM_XCMD_UNKNOWN_HELP_MENU_OPTION, 134, `Internal Error: Unknown option (%1$d) passed to Help menu callback.\n')
INFORM_STR(NWCM_XCMD_ENABLED, 135, `Enabled')
INFORM_STR(NWCM_XCMD_DISABLED, 136, `Disabled')
INFORM_STR_TAG(NWCM_XCMD_GARBAGE_DIALOG_FMT, 137, `Found unexpected characters (\"%1$s\") at end of input (\"%2$s\").  Using %3$s as value for %4$s.')
INFORM_STR_TAG(NWCM_XCMD_SETFAIL_DIALOG_FMT, 138, `The specified value (\"%1$s\") is not valid for %2$s.')
INFORM_STR(NWCM_XCMD_UPDATE_DIALOG_FMT, 140, `The value for the %1$s parameter has not been applied.  Apply it now?')
INFORM_STR_TAG(NWCM_XCMD_SETPARAM_UNSUPPORTED_TYPE, 141, `Internal Error: Unknown parameter type in local parameter list.\n')

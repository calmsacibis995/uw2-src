$ SCCSID(@(#)dos.msg	1.5 LCC) Merge 3.2.0 Branch Modified 18:17:50 11/9/94
$ SCCSID(@(#)dos.msg	1.74 LCC) Modified 21:46:23 11/2/94
$ English message file for Merge UNIX utility programs:
$	dosexec (dos)
$	dosopt
$	xdosopt
$	crt
$	tkbd
$	xcrt
$	display
$	romsnap
$	newunix
$	pssnap
$	swkey
$	x_msg
$	UI library
$	xmrgconfig
$	mrgconfig
$	mrgadmin
$	admconvert
$
$quote "
$domain LCC.PCI.UNIX.JANUS

UNKNOWN "unbekannte Meldung:\n"
ERROR	"FEHLER: %1 %2 %3 %4 %5 %6\n"
WARNING	"WARNUNG:\n"
$	-	dosexec
AOPT_BAD "AOPT_BAD: %1: FEHLER: Falsche Verwendung der Option 'a' '%2=%3'\n"
$	-	dosexec
$	-	dosopt
AOPT_TOOMANY "AOPT_TOOMANY: %1: FEHLER: Zu viele GerÑte zugewiesen.\n\
Fortsetzung nicht mîglich.\n"
$	-	crt
$	-	tkbd
BAD_ARGS "BAD_ARGS: %1: FEHLER: An das Programm wurden falsche Parameter Åbergeben.\n"
$	-	dosexec
BAD_IMG "BAD_IMG: %1: FEHLER: Die Bilddatei '%2' ist nicht der erwartete Typ.\n"
$	-	crt
$	-	tkbd
BAD_LCS_SET_TBL "BAD_LCS_SET_TBL: %1: FEHLER: Falsche Codiertabelle.\n\
\   Verwendung von Tabelle %2 oder %3 nicht mîglich. Fehlernummer=%4."
$	-	dosopt
BAD_OPT_USAGE "BAD_OPT_USAGE: %1: FEHLER: Falsche Verwendung der Option '%2'.\n"
$	-	dosexec
BAD_SWD "BAD_SWD: %1: FEHLER: öbertragungsfehler UNIX-DOS.\n"
$	-	dosexec
BOPT_ERR "BOPT_ERR: %1: FEHLER: 'DOS +b' ist nicht zulÑssig.\n"
$	-	dosexec
CD_X "CD_X: %1: FEHLER: Anfangs-'cd' in Laufwerk %2 fehlgeschlagen.\n\
Verzeichnis kann nicht zu %3%4 wechseln.\n"
$	-aix	pssnap
$	-	romsnap
CHOWN_ERR "CHOWN_ERR: %1: FEHLER:  Chown bei Datei '%2' nicht mîglich, Fehlernummer = %3.\n"
$	-	dosexec
CL_TOOLONG "CL_TOOLONG: %1: FEHLER: Befehlszeile zu lang.\n"
$	-	dosexec
CODEPAGE_NS "CODEPAGE_NS: %1: FEHLER: öbersetzung von DOS CODEPAGE '%2' wird nicht unterstÅtzt.\n\
\    (CODEPAGE-Datei '%3/%4' nicht gefunden.  err=%5)\n"
$	-	dosexec
CODESET_NS "CODESET_NS: %1: FEHLER: öbersetzung des UNIX-Codiersatzes '%2' nicht unterstÅtzt.\n\
\    (Codierdatei '%3/%4' nicht gefunden.  err=%5)\n"
$	-	dosexec
CODESET_USE "\n\
Stattdessen Verwendung von Codiersatz '%2'.\n"
$	-	dosexec
CONFIG_NF "CONFIG_NF: %1: FEHLER: Verwendung der Datei config.sys %2\n\
nicht mîglich.  ôffnen zum Lesen nicht mîglich.\n"
$	-	newunix
CONS_ONLY "CONS_ONLY: %1: FEHLER: Nur auf der Konsole durchfÅhrbar.\n"
$	-	romsnap
$	-	dosopt
CREATE_ERR "CREATE_ERR: %1: FEHLER: Erstellen von Datei '%2' nicht mîglich, Fehlernummer = %3.\n"
$	-	dosopt
CRT_DISPLAY "CRT_DISPLAY: %1: FEHLER: Verwendung von Anzeigetyp '%2' mit 'crt' nicht mîglich.\n"
$	-	dosexec
CWD_TOOLONG "CWD_TOOLONG: %1: FEHLER: Aktuelles Arbeitsverzeichnis ist zu lang %2.\
\nMax=%3.\n"
$	-	dosexec
DA_FAIL "DA_FAIL: Direktanschlu· von '%2' nicht mîglich.  Fehler bei GerÑt '%3'.\n"
$	-	dosexec
DEV_NA "DEV_NA: %1: FEHLER: GewÅnschtes GerÑt %2 nicht verfÅgbar.\n"
$	-	dosexec
DEV_TOOMANY "DEV_TOMANY: %1: FEHLER: Zu viele GerÑte angeschlossen.\n"
$	-	dosopt
DFLTS "* Standard von %2.\n"
$	-	dosopt
NO_OPTIONS "Keine Optionen in %1.\n"
$	-	dosexec
DINFO_BYTES "Byte"
DINFO_CGA "CGA"
DINFO_COM1 "COM1"
DINFO_COM2 "COM2"
DINFO_DIRECT "Direkt"
DINFO_DOS_DIR "DOS (direkt)"
DINFO_DOSPART "DOS Partitionen:"
DINFO_EGA "EGA"
DINFO_RAM "RAM"
DINFO_EMS "EMS"
DINFO_FDRIVES "Diskettenlaufwerke:"
DINFO_HERC "HERC"
DINFO_LPT "LPT"
DINFO_MONO "MONO"
DINFO_MOUSE "Maus"
DINFO_NONE "keine"
DINFO_OTHER "ANDERE"
DINFO_PMSG1 "DOS GerÑteinformation"
DINFO_PMSG2 "<Leertaste> drÅcken, um fortzufahren"
DINFO_TTY "TTY"
DINFO_UD_SHARED "UNIX/DOS gem. benutzt:"
DINFO_UNIX_DEF "UNIX (Standard)"
DINFO_UNKNOWN "unbekannt"
DINFO_USIZE "unbekannte Grî·e"
DINFO_VGA "VGA"
DINFO_VIDEO "Bildschirm"
DINFO_2BUTTON "2-Tasten-Maus"
$	-	display
DISPLAY_USAGE	"Befehlsformat:  display [gerÑte_name]\n\
\    Wird kein GerÑtename angegeben, wird die Standardausgabe gewÑhlt.\n"
$	-	dosexec
DMA_NA "DMA_NA: %1: FEHLER: Kann dma-Kanal %2 nicht zuordnen\n"
$	-	dosexec
DOSDEV_ERR "DOSDEV_ERR: %1: FEHLER: Zeile %2 der GerÑtebeschreibungsdatei\n\
'%3' ist\n\
falsch:\n\
%4\n"
$	-	dosexec
DOSPART_TWOSAME "\
%1: Warnung: Sie versuchen dieselbe DOS-Partition als\n\
zwei Laufwerksbuchstaben, %2 und %3, mit Zugriff SCHREIBEN anzuschlie·en. Das ist nicht zulÑssig. \n\
Fahren Sie mit dem Zugriff NUR LESEN fÅr diese Laufwerke fort.\n"
$	-	dosexec
DPOPT_ERR "DPOPT_ERR: %1: FEHLER: autoexec.bat kann nicht automatisch\
\  auf Laufwerk %2 starten.\n\
(Optionen +p und +d%2 nicht gleichzeitig zulÑssig)\n"
$	-	dosexec
DRIVE_LETTER "DRIVE_LETTER: %1: FEHLER: UngÅltiger Laufwerksbuchstabe %2\n"
$	-	dosexec
EM_FAIL "EM_FAIL: %1: FEHLER: Emulator wurde nicht initialisiert.\n"
$	-	dosexec
EM_VER "EM_VER: %1: FEHLER: Inkorrekte Version des Emulators installiert.\n"
$	-	dosexec
ENV_NOTSET "ENV_NOTSET: %1: FEHLER: Erforderliche Umgebungsvariable '%2'\
\  ist nicht eingestellt.\n"
$	-	dosexec
ENV_TOOMUCH "ENV_TOOMUCH: %1: FEHLER: Umgebung zu lang %2. max=%3.\n"
$	-	crt
ERR_SEMAPHORE "ERR_SEMAPHORE: %1: FEHLER: Zugriff auf Semaphor.\n"
$	-	dosexec
EXEC_CMDCOM "EXEC_CMDCOM: %1: FEHLER: AusfÅhrung von command.com fehlgeschlagen.\n"
$	-	dosexec
EXEC_ERR "EXEC_ERR: %1: FEHLER: %2 unsachgemÑ· installiert.\n"
$	-	dosexec
EXEC_FAILED "EXEC_FAILED:  %1:  FEHLER:  AusfÅhrung von %2 fehlgeschlagen, errno = %3.\n"
$	-	dosexec
EXIT_SIGNAL "EXIT_SIGNAL: %1: FEHLER: Signal %2 aufgefangen.\n"
$	-	dosexec
FATAL "FATAL: Schwerwiegender Fehler in %1:\n%2\n"
$	-	dosopt
FILE_OPTS_ERR "FILE_OPTS_ERR: %1: FEHLER: Dateioperationsfehler.\n"
$	-	dosopt
FN "\nDatei: %2\n"
$	-	dosopt
FN_TOOLONG "FN_TOOLONG: %1: FEHLER: Dateiname zu lang: %2. max=%3.\n"
$	-	dosexec
FORK_FAIL "FORK_FAIL: %1: FEHLER: Interne Verzweigung fehlgeschlagen.  Fehlernr.=%2\n\
(Zuviele Prozesse gestartet oder Speicher erschîpft.)\n"
$
FORK_EXEC_FAIL "FORK_EXEC_FAIL: %1: ERROR: Interne Verzweigung fehlgeschlagen  Fehlernr.=%2\n\
(Zuviele Prozesse gestartet oder Speicher erschîpft.)\n\
%3 kann nicht ausgefÅhrt werden\n"
$	-	dosexec
IA_FAIL "IA_FAIL: '%2' ist nicht verfÅgbar.\n\
GerÑt '%3' ist in Betrieb oder hat falsche Berechtigung.\n"
$	-	dosexec
$	-	dosopt
ILL_DRV "ILL_DRV: %1: FEHLER: UngÅltiges Laufwerk '%2'.\n"
$	-	dosexec
$	-	dosopt
ILL_MX "ILL_MX: %1: FEHLER: UngÅltiger Speicher-Selektor '%2'.\n"
$	-	dosexec
$	-	dosopt
ILL_OPT "ILL_OPT: %1: FEHLER: UngÅltige Option '%2'.\nMit +h erhalten Sie Optionsliste.\n"
$	-	dosexec
IMPROPER_INSTL "IMPROPER_INSTL: %1: FEHLER: UnsachgemÑ· installiert.\n\
\  Berechtigungen oder Privilegien sind falsch.\n"
$	-	dosexec
INITDEV_FAIL "INITDEV_FAIL: %1: FEHLER: Initialisierung von '%2=%3' nicht mîglich.\n"
$	-	dosexec
INIT_FAIL "INIT_FAIL: %1: FEHLER: Initialisierung von GerÑt '%2' nicht mîglich.\n"
$	-	dosexec
INTERNAL_ERR "INTERNAL_ERR: %1: FEHLER: Interner Fehler. %2 fehlgeschlagen. %3 %4 %5 %6\n"
$	-	dosexec
MEMORY_ERR "MEMORY_ERR: %1: FEHLER: Kein Speicherplatz mehr.\n"
$	-	dosexec
INVALID_DEV "INVALID_DEV: %1: FEHLER: %2 ist kein gÅltiges GerÑt. (%3)\n"
$	-	dosexec
INVALID_TMM "INVALID_TMM: %1: FEHLER: %2 ist kein gÅltiges GerÑt.\n\
(UngÅltiger zugeordneter E/A-Speicherbereich %3-%4)\n"
$	-	dosexec
INVALID_MEMMAP "INVALID_MEMMAP: %1: FEHLER: UngÅltiger direkter Anschlu·\n\
Abgebildeter E/A-Speicherbereich %2-%3\n"
$	-	dosexec
BAD_PRINTER_CMD "BAD_PRINTER_CMD: UngÅltiger Druckerbefehl fÅr %s.\n"
$	-	crt
$	-	xcrt
IOCTL_FAIL "IOCTL_FAIL: %1: FEHLER: Ioctl %2 bei GerÑt %3 fehlgeschlagen. Errno=%4.\n"
$	-	dosexec
KILL_FAIL "KILL_FAIL: %1: WARNUNG: Probleme beim Stoppen des Bildschirmprozesses.\n"
$	-	dosexec
LOADDEV "LOADDEV: %1: FEHLER: Fehler bei loaddev.\n"
$	-	dosexec
LOAD_FAIL "LOAD_FAIL: %1: FEHLER: Laden der Bilddatei '%2' fehlgeschlagen.\nerrno = %3.\n"
$	-	dosexec
MAX_USER "Sart einer weiteren DOS/Windows-Sitzung nicht mîglich, da die maximal\n\
zulÑssige Anzahl DOS/Windows-Benutzer (%2) das System aktuell benutzen.\n"
$	-	dosexec
MEM_NA "MEM_NA: %1: FEHLER: Zuordnung von Speicher %2 nicht mîglich\n"
$	-	dosexec
$	-	dosopt
MEM_OOR "MEM_OOR: %1: FEHLER: Der angeforderte Speicher liegt au·erhalb des Bereichs. Max=%2 Min=%3.\n"
$	-	dosexec
MERGE_NA "MERGE_NA: %1: FEHLER: Merge ist fÅr Ihr System nicht konfiguriert.\n\
\  Start von DOS nicht mîglich.\n"
$	-	dosexec tkbd ...
MERGE_DEV_NA "MERGE_DEV_NA: %1: FEHLER: Zugriff auf Systemaufrufe \"Merge\" nicht mîglich.\n"
$	-	newunix
NEWUNIX_USAGE "\nVerwendung:  %2 [-n vt_Zahl] [-p] [[-e] Befehl]\n"
$	-	dosexec
NO_DOS "NO_DOS: %1: FEHLER: DOS ist nicht installiert.\n\
\  Installieren Sie DOS mit 'dosinstall', ehe Sie Merge verwenden.\n"
$	-	dosexec
NO_PATH "NO_PATH: %1: FEHLER: Der UNIX PFAD ist nicht festgelegt.\n"
$	-	dosexec
NO_LICENCE "NO_LICENCE: %1: Es ist keine Lizenz verfÅgbar, um DOS/Windows jetzt auszufÅhren.\n"
$	-	dosexec
NO_USERS "NO_USERS: %1: Merge-Dienste von DOS/Windows wurden abgeschaltet.\n\
Jetzt ist keine DOS/Windows-Verwendung gestattet.\n"
$	-	dosexec
$	-	crt
$	-	tkbd
$	-	dosexec
NOT_DEV "NOT_DEV: %1: FEHLER: %2 (%3) ist kein GerÑt.\n"
$	-	dosexec
$	-	dosopt
NOT_DOS_PROG "NOT_DOS_PROG: %1: FEHLER: '%2' ist kein DOS Programm.\n"
$	-	dosopt
NOT_RW_ERR "NOT_RW_ERR: %1: FEHLER: ôffnen von Datei '%2' nicht mîglich.\n\
\  Datei nicht vorhanden oder darf von Ihnen nicht gelesen/geschrieben werden.\n"
$	-	dosopt
NOT_RW_WARN "NOT_RW_WARN: %1: WARNUNG ôffnen von Datei '%2' nicht mîglich.\n\
\  Datei nicht vorhanden oder darf von Ihnen nicht gelesen/geschrieben werden.\n"
$	-	dosexec
$	-	dosopt
$	-	display
$	-aix	pssnap
$	-	romsnap
NOT_R_ERR "NOT_R_ERR: %1: FEHLER: ôffnen von Datei '%2' nicht mîglich.\n\
\  Datei nicht vorhanden bzw. darf von Ihnen nicht gelesen werden.\n"
$	-	dosopt
NOT_R_WARN "NOT_R_WARN: %1: WARNUNG ôffnen von Datei '%2' nicht mîglich.\n\
\  Datei nicht vorhanden bzw. darf von Ihnen nicht gelesen werden.\n"
$	-	dosopt
NOT_W_ERR "NOT_W_ERR: %1: FEHLER: ôffnen von Datei '%2' nicht mîglich.\n\
\  Datei nicht vorhanden bzw. darf von Ihnen nicht geschrieben werden.\n"
$	-	dosexec
NO_BG_CMD "NO_BG_CMD: %1: FEHLER: AusfÅhrung eines bildschirmorientierten DOS-Programms\n\
\         im Hintergrund nicht mîglich.  Statt 'DOS ... &' 'DOS ...' verwenden\n"
$	-	dosexec
NO_BG_DOS "NO_BG_DOS: %1: FEHLER: DOS Sitzung kann nicht umgeleitet\n\
\  oder im Hintergrund gestartet werden.\n"
$	-	dosopt
NO_FILENAME "NO_FILENAME: %1: FEHLER: Dateiname erforderlich.\n"
$	-	crt
NO_GRAPHICS "\
\r\n\
\          +============================+ \r\n\
\          |                            | \r\n\
\          |          KEINE             | \r\n\
\          |         GRAFIKEN           | \r\n\
\          |           EIN              | \r\n\
\          |      ASCII-TERMINAL        | \r\n\
\          |                            | \r\n\
\          |    Grafikmodus verlassen   | \r\n\
\          |                            | \r\n\
\          +============================+ \r\n\
\r\n\
\n"
$	-	dosexec
NO_INIT_DATA "NO_INIT_DATA: %1: FEHLER: Keine Init-Daten.\n"
$	-	dosexec
NO_MORE "NO_MORE: %1: FEHLER: Kein Speicherplatz mehr verfÅgbar. Errno=%2\n"
$	-	dosexec
$	-	newunix
NO_RESOURCES "NO_RESOURCES: %1: FEHLER: Keine Ressourcen mehr verfÅgbar.\n"
$	-	newunix
NO_VT "NO_VT: %1: FEHLER: Kein vt verfÅgbar.\n"
$	-	dosexec
NO_VMPROC "NO_VMPROC: %1: FEHLER: Kein vm-Proze·.\n"
$	-	crt
NO_WIDE "\
\r\n\
\          +===============================+ \r\n\
\          |                               | \r\n\
\          |       NUR 80 SPALTEN          | \r\n\
\          |             AUF               | \r\n\
\          |        ASCII-TERMINAL         | \r\n\
\          |                               | \r\n\
\          |  132-Spaltenmodus verlassen   | \r\n\
\          |                               | \r\n\
\          +===============================+ \r\n\
\r\n\
\n"
$	-	dosexec
ON_ERR "ON_ERR: %1: FEHLER: Start des bildschirmorientierten DOS mit ON nicht mîglich.\n"
$	-	dosexec
PATH_TOOMUCH "PATH_TOOMUCH: %1: FEHLER: UmgebungsPFAD ist zu lang %2. max=%3.\n"
$	-	dosexec
PROC_EXIT "PROC_EXIT: %1: FEHLER: Notwendiger Prozess %2 unerwartet beendet.\n\
 Beenden.\n"
$	-	dosexec
$	-	dosopt
$	-aix	pssnap
$	-	romsnap
$	-	tkbd
READ_ERR "READ_ERR: %1: FEHLER: Lesen von Datei '%2'nicht mîglich.\n"
$	-	dosexec
PORT_NA "PORT_NA: %1: FEHLER: Zuordnung von Port(s) %2 nicht mîglich\n"
$	- dosexec (SVR4 only)
REBOOT_FOR_PARTITIONS "\
%1: Warnung:\n\
Zugriff auf DOS-Partitionen erst nach Neustart des Systems mîglich.\n"
$	-	dosopt
RMV_OPTS_ERR1 "RMV_OPTS_ERR1: %1: FEHLER: Entfernen von Option %2 nicht mîglich.\n\
\  Option war nicht installiert."
$	-	dosopt
RMV_OPTS_ERR2 "RMV_OPTS_ERR2: %1: FEHLER: Entfernen von Option %2 nicht mîglich.\n\
\  UngÅltige Option.\n"
$	-	romsnap
ROMSNAP_BAD_ADDR "ROMSNAP_BAD_ADDR: %1: FEHLER:  Falsche ROM-Startadresse %2.\n"
$	-	romsnap
ROMSNAP_USAGE "\nBefehlsformat:  romsnap  ROM-Startadresse  ROM-Bilddatei [-k].\n"
$	-	dosopt
$	-aix	pssnap
$	-	romsnap
SEEK_ERR "SEEK_ERR: %1: FEHLER: Problem mit Datei %2. (Suche fehlgeschlagen).\n"
$	-	dosexec
SEND_SIG_ERR "SEND_SIG_ERR: %1  Senden von Signal %2 an %3 nicht mîglich, errno = %4.\n"
$	-	dosexec
SERIOUS	"SERIOUS: Schwerer Fehler in %1:\n%2\n"
$	-	dosexec
SHAREINIT "SHAREINIT: %1: FEHLER: Initialisierung von gemeinsamem Speicherplatzsegment nicht mîglich.\n\
Fehler = %2 (%3)\n"
$	-	dosexec
SHELL_NF "SHELL_NF: %1: FEHLER: Verwendung von SHELL=%2 nicht mîglich. Datei nicht gefunden.\n"
$	-	crt
SHMA_FAILED "SHMA_FAILED: %1: FEHLER: Verwendung von gemeinsamem Speicher nicht mîglich. errno=%2\n"
$	-	dosexec
SHMC_FAILED "SHMC_FAILED: %1: FEHLER: Anlegen von gemeinsamem Speicher nicht mîglich.\n\
Errno=%2 a=%3 s=%4\n"
$	-aix	dosexec
SITE_NI "SITE_NI: %1: FEHLER: Merge ist auf Anlage %2 nicht installiert.\n"
$	-	dosexec
$	-	dosopt
SOPT_USAGE "SOPT_USAGE: %1: FEHLER: +s ZeitÅberschreitung au·erhalb des Wertebereichs.\n\
Wenn Sie keine ZeitÅberschreitung angeben wollen, 0 verwenden.\n\
Wenn Sie eine ZeitÅberschreitung angeben wollen, %2 bis %3 angeben.\n"
$	-	dosexec
SWITCHDOS_FAIL "SWITCHDOS_FAIL: %1: FEHLER: öbertragung UNIX-DOS fehlgeschlagen.\n\
errno=%2.\n"
$	-	dosexec
$	-	crt
SWKEY_FAIL "SWKEY_FAIL: FEHLER: Kann Switchkey nicht von einem Terminal aus starten.\n"
$	-	swkey
SWKEY_CUR "\nAktuelle Bildschirmumschaltsequenz von Merge 386 und Xsight: %2%3%4F<n>.\n"
$	-	swkey
SWKEY_USAGE "\n%1: FEHLER: Falsche Option\n\
Befehlsformat: switchkey [-acs]\n"
$	-	swkey
SWKEY_NEW "\nNeue Bildschirmumschaltsequenz Merge 386 und Xsight: %2%3%4F<n>.\n"
$	-	swkey
TERM_CAP "TERM_CAP: %1: FEHLER: Ihr Terminal hat nicht die erforderlichen\n\
\   Eigenschaften, um ein fernes, bildschirmorientiertes DOS zu unterstÅtzen.\n"
$	-	dosexec
TERM_ERR "TERM_ERR: %1: FEHLER: Kein terminfo fÅr TERM '%2'.\n"
$	-	dosexec
TOKENPAIR_NF "TOKENPAIR_NF: %1: FEHLER: Geeignete Tokentypen\n\
fÅr\n'%2=%3' nicht gefunden.\n"
$	-	dosexec
TOKEN_BAD "TOKEN_BAD: %1: FEHLER: Falsches Token %2 (%3).\n"
$	-	dosexec
TOKEN_NF "TOKEN_NF: %1: FEHLER: Token '%2' des erforderlichen Typs nicht gefunden.\n"
$	-	dosexec
BAD_DRIVE "BAD_DRIVE: %1: FEHLER: falscher Laufwerksbuchstabe %2.\n"
$	-	dosexec
LPT_BAD "LPT_BAD: %1: FEHLER: LPT2 und LPT3 kînnen nicht auf einen Token-Typ mit direktem Anschlu· eingestellt werden."
$	-	dosopt
UI_PROB "UI_PROB: %1: FEHLER: Installation von Datei '%2' nicht rÅckgÑngig zu machen.\n\
\     Datei ist derzeit nicht installiert.\n"
$	-	dosexec
UMB_CONFLICT "UMB_CONFLICT: %1: FEHLER: Konflikt bei der Zuweisung von Blîcken\n\
\     im oberen Speicherbereich (UMBs) %2-%3\n"
$	-	dosexec
UPOPT_ERR "UPOPT_ERR: %1: FEHLER: Gleichzeitige Verwendung der Optionen +p und +ut\
\  unzulÑssig.\n"
$	-	dosexec
VECT_NA "VECT_NA: %1: FEHLER: Zuordnung von Interrupt-Vektor %2 nicht mîglich\n"
$	-	crt
VMATTACH_FAILED "VMATTACH_FAILED: %1: FEHLER: vm86 (virtueller Speicher 86) konnte nicht zugeschrieben werden.\n\
Errno=%2\n"
$	-	dosexec
VMSVR_FAIL "VMSVR_FAIL: %1: VM Serverinitialisierung fehlgeschlagen. errno=%2.\n"
$	-	crt
$	-	dosexec
VM_DIED "VM_DIED: %1: FEHLER: Der VM86-Proze· wurde abgebrochen.\n"
$	-	dosexec		janus_main.c
WAIT_RET_ERR "WAIT_RET_ERR: %1:  FEHLER:  Systemaufruf \"Wait\" kommt zu frÅh zurÅck\n\
errno = %2.\n"
$	-	dosopt
$	-aix	pssnap
$	-	romsnap
WRITE_ERR "WRITE_ERR: %1: FEHLER: Schreiben von Datei '%2' nicht mîglich.\n"
$	-	dosopt
ZOPT_ERR "ZOPT_ERR: %1: FEHLER: Verwendung von Option 'z' oder 'Z' fÅr\n\
eine Standardwertedatei %2 nicht mîglich.\n"
$	-	dosopt
ZOPT_EXTRA "ZOPT_EXTRA: %1: FEHLER: Nach 'z' kann nur eine Option angegeben werden.\n"
$	-	dosopt
ZOPT_MISSING "ZOPT_MISSING: %1: FEHLER: Fehlende Option nach 'z'.\n"
$	-	dosopt
ZOPT_TOOMANY "ZOPT_TOOMANY: %1: FEHLER: Zu viele +z-Objekte angegeben. Fortfahren nicht mîglich.\n"
$	-	dosexec
HELP_EXEC "\n\
\   Verwendung: %1 [Flaggen] Dateiname Argum....\n\
\   Verwendung: %1 [Flaggen]\n\
\   Das erste Formular fÅhrt den DOS-Dateinamen aus, das zweite Formular\n\
\   fÅhrt 'command.com' von DOS aus.  Die Flaggen Åberschreiben\n\
\   Standardvorgaben und installierte Optionen und geben Anweisungen zur\n\
\   Aufhebung normaler ausfÅhrbarer DOS-Operationen.\n\
\   ('+' zeigt Optionsauswahl, '-' zeigt RÅckgÑngigmachung der Optionsauswahl an.)\n\
\     +- a[x] GerÑt zuweisen 'x'.\n\
\     +- b    Sich gut verhaltendes Programm. (stdio von/zu UNIX umleiten)\n\
\     +  c    Befehl: Der DOS-Befehlsdateiname wird ungeÑndert an DOS \n\
\             weitergegeben.\n\
\     +  dX   Momentanes Initialisierungslaufwerk festlegen. X= a bis z.\n\
\     +- e[f] GerÑtekonfigurationsdatei 'f' verwenden.\n\
\     +  h    Hilfe-Informationen angezeigt (dieser Bildschirm). DOS wird\n\
\             nicht ausgefÅhrt.\n\
\     +- l[f] Bilddatei 'f' laden ('f' kann ebenso Verzeichnisname sein).\n\
\     +  mn   Speichergrî·e. Zahl 'n' (dezimal) in Megabyte.\n\
\     +- p[f] command.com 'Permanent' machen. Die auszufÅhrende autoexec.bat ist 'f'.\n\
\     +- s[n] DOS-Druckerausgabe an UNIX Drucker spulen. ZeitÅberschreitung ist 'n' Sekunden.\n\
\     +- t    DOS-Befehlszeilenargumente Åbersetzen.\n"

$ 	-	dosopt
HELP_INST "\n\
Befehlsformat: %1 [Flaggen] Dateinamen\n\
\   Optionen fÅr Dateinamen installieren. Installationsflaggen haben anfÑngl. '+' oder '-'.\n\
\   Die anfÑngl. '+' oder '-' geben AuswÑhlen oder Entfernen der Option an.\n\
\   Wenn keine Optionen oder Anweisungen vorhanden sind, werden die aktuellen Optionen gedruckt.\n\
\     +- a[x] GerÑt 'x' zuweisen.\n\
\     +- b    Funktionierendes Programm. (E/A-Standard von/nach UNIX umleiten)\n\
\     +- dX   Aktuelles Ausgangslaufwerk einrichten. X= a bis z.\n\
\     +- e[f] GerÑtekonfigurationsdatei 'f' verwenden.\n\
\     +  h    Anzeige von Hilfeinformationen (dieser Bildschirm).\n\
\     +- l[f] DOS-Bilddatei 'f' laden ('f' kann auch ein Verzeichnisname sein).\n\
\     +  mn   Speichergrî·e. Zahl 'n' (dezimal) in Megabyte.\n\
\     +- p[f] command.com auf 'Permanent' einstellen. 'f' steht fÅr die auszufÅhrende Datei autoexec.bat.\n\
\     +- s[n] DOS-Druckerausgabe zu UNIX-Drucker spulen. ZeitÅberschreitung gleich 'n' Sekunden.\n\
\     +- t    DOS-Befehlszeilenargumente Åbersetzen.\n\
\     +- v    Verbos. BestÑtigungsmeldung anzeigen.\n\
\     +  y    COM-Dateien kînnen von UNIX ausgefÅhrt werden.  Wird nicht benîtigt, \n\
\             wenn andere Optionen eingestellt sind.\n\
\     +  zX   Option X entfernen oder zurÅcksetzen.\n\
\     +  Z    Alle Installationsdaten aus Datei entfernen..\n"
$
$ Note: The messages for the ifor_pm_* calls are for SCO only dosexec.
$
$ Error messages for error returns from 'ifor_pm_init_sco()'
$
$ --- For return value:  IFOR_PM_REINIT
PM_INIT_REINIT "\
%1: WARNUNG-  Versuch zur Reinitialisierung des Policy Managers.  (IFOR_PM_REINIT)\n"
$
$ --- For return value:  IFOR_PM_BAD_PARAM
PM_INIT_BAD_PARAM "\
%1: FEHLER-  UngÅltiger Parameter fÅr ifor_pm_init_sco.  (IFOR_PM_BAD_PARAM)\n"
$
$ --- For return value:  IFOR_PM_FATAL
PM_INIT_FATAL "\
%1: FEHLER-  Initialisierung des Policy Managers fehlgeschlagen.  (IFOR_PM_FATAL)\n"
$
$ Error messages for error returns from 'ifor_pm_request_sco()'
$
$ --- For return value:  IFOR_PM_BAD_PARAM
PM_REQ_BAD_PARAM "\
%1: FEHLER-  UngÅltiger Parameter fÅr ifor_pm_request_sco.  (IFOR_PM_BAD_PARAM)\n"
$
$ --- For return value:  IFOR_PM_NO_INIT
PM_REQ_NO_INIT "\
%1: FEHLER-  FÅr die Initialisierung des Policy Managers ist eine Lizenz erforderlich.  \n\
(IFOR_PM_NO_INIT)\n"
$
$ --- For return value:  IFOR_PM_FATAL
PM_REQ_FATAL "\
%1: FEHLER-  Lizenzanfrage vom Policy Manager fehlgeschlagen.  (IFOR_PM_FATAL)\n"
$
$ Error messages for error returns from 'ifor_pm_release()'
$
$ --- For return value:  IFOR_PM_BAD_PARAM
PM_RELEASE_BAD_PARAM "\
%1: WARNUNG-  UngÅltiger Parameter fÅr ifor_pm_release.  (IFOR_PM_BAD_PARAM)\n"
$
$ --- For return value:  IFOR_PM_FATAL
PM_RELEASE_FATAL "\
%1: WARNUNG-  Lizenzfreigabe fehlgeschlagen.  (IFOR_PM_FATAL)\n"
$
$	- 	xdosopt
XDOSOPT_TITLE "DOS Optionen"
XDOSOPT_START_BUTTON "Start"
XDOSOPT_SAVE_BUTTON "Anwenden"
XDOSOPT_DEFAULT_BUTTON "Standardwerte"
XDOSOPT_HELP_BUTTON "Hilfe"
XDOSOPT_CANCEL_BUTTON "Abbrechen"
XDOSOPT_VIDEO_TITLE "Bildschirm"
XDOSOPT_VGA_LABEL "VGA"
XDOSOPT_CGA_LABEL "CGA"
XDOSOPT_MDA_LABEL "MDA"
XDOSOPT_HERC_LABEL "Hercules"
XDOSOPT_COM_TITLE "COM-AnschlÅsse"
XDOSOPT_COM1_LABEL "COM1"
XDOSOPT_COM2_LABEL "COM2"
XDOSOPT_EMS_TITLE "EMS"
XDOSOPT_DRIVES_TITLE "Laufwerke"
XDOSOPT_DEFAULT_DRIVE "Keine"
XDOSOPT_LPT_TITLE "LPT-AnschlÅsse"
XDOSOPT_LPT_NAME "LPT1"
XDOSOPT_SPOOL_LP_NAME   "UNIX (Spooled)"
XDOSOPT_DIRECT_LP0_NAME "DOS (Direkt):lp0"
XDOSOPT_DIRECT_LP1_NAME "DOS (Direkt):lp1"
XDOSOPT_DIRECT_LP2_NAME "DOS (Direkt):lp2"
XDOSOPT_STATUS_DONE_MSG "Abgeschlossen"
XDOSOPT_QUIT_MSG "Sie haben die énderungen nicht angewendet.\nMîchten Sie wirklich beenden?"
XDOSOPT_YES_BUTTON "Ja"
XDOSOPT_NO_BUTTON "Nein"
XDOSOPT_OK_BUTTON "OK"
XDOSOPT_SAVE_ERR_MSG "Sie sind nicht berechtigt, fÅr diese\nDatei énderungen zu speichern."
XDOSOPT_MEM_TITLE "Speicher"
XDOSOPT_XMEM_NAME "Standard"
XDOSOPT_EMEM_NAME "EMS"
XDOSOPT_CONFIG_READ_MSG "Lesen der DOS- oder Windows-Konfiguration nicht mîglich."
XDOSOPT_MEMORY_MSG	"Kein Speicherplatz mehr."
XDOSOPT_INTERNAL_ERROR	"Interner Fehler."
$	- 	x_msg
XMSG_DOSERRTITLE "DOS Fehler"
XMSG_OKTITLE "OK"
$	- 	xcrt
XCRT_DOS_TITLE	"DOS"
XCRT_WINDOWS_TITLE "Fenster"
XCRT_FILE	"Datei"
XCRT_FILE_M	"D"
XCRT_ZOOM	"Fenster vergrî·ern"
XCRT_ZOOM_M	"F"
XCRT_REFRESH	"Auffrischen"
XCRT_REFRESH_M	"A"
XCRT_EXIT	"Beenden"
XCRT_EXIT_M	"B"
XCRT_OPTIONS	"Optionen"
XCRT_OPTIONS_M	"O"
XCRT_FOCUS	"Mausfokus auf DOS"
XCRT_FOCUS_M	"M"
XCRT_FONTS	"DOS-Schriftart"
XCRT_FONTS_M	"D"
XCRT_KEYS	"Schnelltasten"
XCRT_KEYS_M	"S"
XCRT_TUNE	"Abstimmen..."
XCRT_TUNE_M	"A"
XCRT_AUTO	"Automatisch"
XCRT_AUTO_M	"t"
XCRT_SMALL	"Klein"
XCRT_SMALL_M	"K"
XCRT_MEDIUM	"DatentrÑger"
XCRT_MEDIUM_M	"D"
XCRT_TO_DOS	"An DOS/Windows"
XCRT_TO_DOS_M	"W"
XCRT_TO_X	"An X Desktop"
XCRT_TO_X_M	"X"
XCRT_OK		"OK"
XCRT_CANCEL	"Abbrechen"
XCRT_HELP	"Hilfe"
XCRT_HELP_M	"H"
XCRT_ON_WINDOW	"Fenster"
XCRT_ON_WINDOW_M "s"
XCRT_ON_KEYS	"Tasten"
XCRT_ON_KEYS_M	"T"
XCRT_INDEX	"Index"
XCRT_INDEX_M	"I"
XCRT_ON_VERSION "Version"
XCRT_ON_VERSION_M "V"
XCRT_TUNE_TITLE		"Optionen"
XCRT_TUNE_COLORMAP	"Farbenkarte"
XCRT_TUNE_AUTOZOOM	"Automatischer Zoom"
XCRT_TUNE_DEFAULTS	"Standardeinstellungen ab Werk"
XCRT_CLIPBOARD	"Graphischer Zwischenspeicher X wird aktualisiert ..."
XCRT_CLIPBOARD_DONE	"Aktualisierung des graphischen Zwischenspeichers ... abgeschlossen."
XCRT_VERSION		"Version"
XCRT_VERSION_TEXT	"DOS Merge\n%1\nCopyright %2\nLocus Computing Corporation"
XCRT_QUIT_MSG "DOS beenden?"
XCRT_NOHELP_MSG "Keine Hilfe verfÅgbar."
XCRT_YES_BUTTON_TEXT "Ja"
XCRT_NO_BUTTON_TEXT "Nein"
XCRT_OK_BUTTON_TEXT "OK"
XCRT_NODEV_ERR "Fehler: GerÑt %1 nicht verfÅgbar."
XCRT_INTR_ERR "Fehler: MERGE-interner Fehler."
XCRT_VT_ERR "Fehler: Erhalt einer neuen Bildschirmanzeige nicht mîglich."
XCRT_SERVER_ERR "Fehler: Server gibt Anzeige nicht frei."
XCRT_NOMSE_ERR "DOS Sitzung ist nicht fÅr Maus eingerichtet."
XCRT_TMPL1 "%1 wird auf Bildschirm vt%2 gezoomt."
XCRT_TMPL2 "%1 drÅcken, um das DOS MenÅ aufzurufen."
XCRT_MSG1 "EGA/VGA-Grafiken sind nicht in einem Fenster verfÅgbar."
XCRT_MSG2 "DOS Fenster verlassen und DOS neu starten."
XCRT_MSG3 "WÑhlen Sie im FenstermenÅ \"Zomm\", um Graphiken auszufÅhren."
XCRT_MSG4 "Zoom kann nicht fortgesetzt werden."
XCRT_MSG5 "Fokus ist nicht betriebsbereit."
XCRT_VERSION_ERROR "Ihr Bildschrimtreiber fÅr DOS Merge Windows/X mu· aktualisiert\n\
werden.  Schlagen Sie nach in den Hinweisen zur Version nach,\n\
wenden Sie sich an Ihren Systemverwalter oder gehen Sie\n\
folgenderma·en vor: \n\
1) Starten Sie DOS\n\
2) Geben Sie ein\n\
   WINXCOPY <Laufwerk>:<Windows\\System Verz.>\n\
   Beispiel: WINXCOPY D:\\WINDOWS\\SYSTEM"
XCRT_PICTURE_ERROR "Anzeige des Bilds nicht mîglich. Versuchen\nSie eine Bildschirmauflîsung von mindestens 800x600\nund mindestens 256 Farben."
$	- 	xcrt - Old GUI Messages
XCRT_MENU_TITLE "DOS MenÅ"
XCRT_UNFOCUS_LABEL "Fokussierung aufheben"
XCRT_X_COLORS_LABEL "X-Farben"
XCRT_DOS_COLORS_LABEL "DOS Farben"
XCRT_FREEZE_LABEL "Autofreeze eingeschaltet"
XCRT_UNFREEZE_LABEL "Autofreeze ausgeschaltet"
XCRT_UNZOOM_LABEL "Vergrî·erung aufheben"
XCRT_NORMAL_KEYS_LABEL "DOS Tasten"
XCRT_WINDOW_KEYS_LABEL "Desktop-Tasten"
XCRT_OLD_MSG3 "Im DOS MenÅ auf Zoom klicken, um Graphiken zu starten."
$	- 	xmrgconfig
GUI_OK			"OK"
GUI_CANCEL		"Abbrechen"
GUI_HELP		"Hilfe"
GUI_YES			"Ja"
GUI_NO			"Nein"
GUI_DELETE		"Lîschen"
GUI_MODIFY		"éndern"
GUI_TITLE 		"DOS Merge"
GUI_AUTOMATIC_LABEL	"Automatisch"
GUI_CONFIG_TITLE	"DOS und Windows-Konfiguration"
GUI_NONE_LABEL		"Keine"
GUI_HOME_LABEL		"Home"
GUI_DEVICES_BUTTON_LABEL "GerÑte"
GUI_OPTIONS_BUTTON_LABEL "Optionen"
GUI_SAVE_LABEL		"Speichern"
GUI_SAVE_AS_LABEL	"Speichern als..."
GUI_START_LABEL		"Start"
GUI_DRIVES_LABEL	"Laufwerke"
GUI_CONFIGURE_LABEL	"Konfigurieren"
GUI_DOS_DRIVE_LABEL	"DOS Laufwerk..."
GUI_UNIX_FILESYS_LABEL	"UNIX Dateisystem..."
GUI_COM_PORTS_LABEL	"COM-AnschlÅsse"
GUI_COM1_LABEL		"COM1"
GUI_COM2_LABEL		"COM2"
GUI_LPT_PORTS_LABEL	"LPT-AnschlÅsse"
GUI_LPT1_LABEL		"LPT1"
GUI_LPT2_LABEL		"LPT2"
GUI_LPT3_LABEL		"LPT3"
GUI_TIMEOUT_LABEL	"ZeitÅberschreitung"
GUI_DEVICES_LABEL	"GerÑte"
GUI_DEVICES_STATUS_MSG	"Es besteht ein Konflikt zwischen\n%1 und\n%2."
GUI_MULTI_CONFLICT_MSG  "Konflikt zwischen mehreren\nGerÑten entdeckt."
GUI_MEMORY_LABEL	"Speicher"
GUI_STANDARD_LABEL	"Standard"
GUI_EMS_LABEL		"EMS"
GUI_AUTOEXEC_LABEL	"AUTOEXEC.BAT"
GUI_RUN_SYS_LABEL	"Systemweit ausfÅhren"
GUI_PERSONAL_LABEL	"Nur persînliche ausfÅhren"
GUI_OTHER_LABEL		"Andere"
GUI_BROWSE_LABEL	"BlÑttern..."
GUI_EDIT_LABEL		"Dateien bearbeiten"
GUI_AUTOEXEC_MNEMONIC	"A"
GUI_SYS_AUTO_LABEL	"Systemweite AUTOEXEC.BAT..."
GUI_PERSONAL_AUTO_LABEL "Persînliche AUTOEXEC.BAT..."
GUI_OTHER_AUTO_LABEL	"Andere AUTOEXEC.BAT..."
GUI_CONFIG_LABEL	"CONFIG.SYS"
GUI_CONFIG_MNEMONIC	"C"
GUI_SYS_CONFIG_LABEL	"Systemweite CONFIG.SYS..."
GUI_PERSONAL_CONFIG_LABEL "Persînliche CONFIG.SYS..."
GUI_OTHER_CONFIG_LABEL	"Andere CONFIG.SYS..."
GUI_WINDOWS_SIZE_LABEL	"Windows-Grî·e"
GUI_CUSTOM_LABEL	"Individuell:"
GUI_RESIZE_LABEL	"Manuelle Grî·enanpassung..."
GUI_WIDTH_LABEL		"Breite:"
GUI_HEIGHT_LABEL	"Hîhe:"
GUI_DOS_SIZE_LABEL	"DOS-Grî·e"
GUI_DOS_FONT_LABEL	"DOS-Schriftart"
GUI_SCALE_DOS_LABEL	"DOS-Graphiken skalieren"
GUI_DISPLAY_TYPE_LABEL	"Typ anzeigen"
GUI_SMALL_LABEL		"Klein"
GUI_MEDIUM_LABEL	"DatentrÑger"
GUI_LARGE_LABEL		"Gro·"
GUI_X1_LABEL		"x1"
GUI_X2_LABEL		"x2"
GUI_MDA_LABEL		"MDA"
GUI_HERCULES_LABEL	"Hercules"
GUI_CGA_LABEL		"CGA"
GUI_VGA_LABEL           "VGA"
GUI_OPTIONS_LABEL	"Optionen"
GUI_AUTOZOOM_LABEL	"Automatischer Zoom"
GUI_COLORMAP_LABEL	"Farbenkarte"
GUI_ACCEL_KEYS_LABEL	"Schnelltasten auf X"
GUI_FACTORY_LABEL	"Standardeinstellungen ab Werk"
GUI_COMMAND_LABEL	"Befehl"
GUI_DOS_DRIVE_TITLE	"DOS-Laufwerk"
GUI_READ_ONLY_LABEL	"Nur Lesen"
GUI_EXCLUSIVE_LABEL	"Ausschlie·lich"
GUI_FILES_LABEL	 	"Dateien"
GUI_FILTER_LABEL	"Filter"
GUI_DIRECTORIES_LABEL	"Verzeichnisse"
GUI_SELECTION_LABEL	"Auswahl"
GUI_RESIZE_TITLE	"Windows Fenstergrî·e"
GUI_FILE_LABEL		"Datei"
GUI_RESIZE_MESSAGE	"Verwenden Sie Ihre Maus oder Tastatur, um die Grî·e dieses\nFenster auf Ihre bevorzugte Microsoft Windows Fenstergrî·e\neinzustellen."
GUI_STATUS_MSG		"Statuszeile"
GUI_CONFIRM_MSG		"Sie haben die Konfiguration verÑndert.\nSind Sie sicher, da· Sie beenden wollen?"
GUI_FILE_DOESNT_EXIST_MSG "Datei existiert nicht."
GUI_DIR_DOESNT_EXIST	"Verzeichnis existiert nicht."
GUI_FILE_IS_DIR_MSG	"AusgewÑhlte Datei ist ein Verzeichnis, kann nicht fÅr Bearbeitung geîffnet werden."
GUI_PERMISSION_DENIED	"Zugriff verweigert."
GUI_NOT_DIR		"Angegebene Auswahl ist kein Verzeichnis."
GUI_EDIT_ERROR		"Fehler bei der Bearbeitung der Datei"
GUI_OPEN_CONFIG_ERROR	"Fehler beim ôffnen der Konfigurationsdatei."
GUI_CONFIG_DOESNT_EXIST "Konfiguration \"%1\" existiert nicht."
GUI_INTERNAL_ERROR	"Interner Fehler"
GUI_MEMORY_ERROR	"Kein Speicherplatz mehr."
GUI_WRITE_CONFIG_ERROR	"Konfigurationsdatei kann nicht gespeichert werden:\n%1"
GUI_DELETE_CONFIG_ERROR	"Konfigurationsdatei kann nicht gelîscht werden."
GUI_ALREADY_EXISTS	"Konfiguration \"%1\" existiert bereits."
GUI_CONFLICT_MSG	"Folgende GerÑte stehen untereinander in Hardware-Konflikt:\n%1.\nUm diese Konflikte zu lîsen, machen Sie die Auswahl eines oder\nmehrerer dieser Objekte im Bereich \"GerÑte\" rÅckgÑngig.\nWenn Sie diese GerÑte miteinander verwenden wollen, mÅssen\nSie eines oder mehrere neu konfigurieren, um\nHardware-Konflikte zu vermeiden."
GUI_DRIVE_WARNING	"Warnung: \"%1\" existiert nicht."
GUI_CANT_START		"DOS- oder Windows-Sitzung kann nicht gestartet werden."
GUI_NO_DRIVES_ERROR	"Kein DOS Laufwerke verfÅgbar."
GUI_CANT_READ_FILE	"Datei \"%1\" nicht lesbar."
GUI_CANT_CREATE_FILE	"Datei \"%1\" nicht lesbar."
GUI_VIEW_FILE_MSG	"Die Datei \"%1\" ist nicht beschreibbar, mîchten Sie sie einsehen?"
GUI_CREATE_FILE_MSG	"Die Datei \"%1\" existiert nicht, mîchten Sie sie erstellen?"
$	-	mrgadmin messages
ADM_USAGE_CMD "Befehlsformat:\tmrgadmin class add Token:Datei:vollstÑndiger Name\n\t\tmrgadmin class delete Token\n\t\tmrgadmin class list [Token]\n\t\tmrgadmin class update Token:Datei:vollstÑndiger Name\n\t\tmrgadmin class printdef Token\n"
ADM_CLASS_MSG "Fehler: zugelassene Klassen sind:\n"
ADM_NO_CLASS_MSG "Kann keine Informationen Åber Klasse \"%1\" erhalten.\n"
ADM_NO_CLASS_ENTRIES_MSG "Keine EintrÑge fÅr Klasse \"%1\" gefunden.\n"
ADM_BAD_TOKEN_MSG "Kann keine Informationen Åber Token \"%1\" erhalten.\n"
ADM_PERMISSION_MSG "Sie mÅssen root sein, um die Merge Verwaltung zu Ñndern.\n"
ADM_CANT_DELETE_TOKEN_MSG "Token \"%1\" kann nicht gelîscht werden\n"
ADM_BAD_TOKEN_DEF_MSG "Die Token-Definition mu· folgende Form haben: \"Token:Datei:Name\".\n"
ADM_CANT_READ_MSG	"Lesen von \"%1\" nicht mîglich: %2.\n"
$	-	mrgadmin commands
ADM_ADD_STR		"add"
ADM_DELETE_STR		"delete"
ADM_LIST_STR		"list"
ADM_UPDATE_STR		"update"
ADM_PRINTDEF_STR	"printdef"
$	-	admin library messages
ADM_NO_ERROR "Interner Fehler - kein Fehler."
ADM_INTERNAL_ERROR "Interner Fehler in Verwaltungsbibliothek."
ADM_PARSE_VARIABLE "Unerkannter Variablenname."
ADM_PARSE_NO_NUMBER "Es mu· eine Nummer angegeben werden."
ADM_PARSE_NUMBER "UnzulÑssiger numerischer Wert."
ADM_PARSE_ILLEGAL_VARIABLE "Variable fÅr angegebenen Anschlu·typ nicht zugelassen."
ADM_PARSE_MAX_NUMBERS "Anzahl der maximal zugelassenen Elemente Åberschritten."
ADM_MEMORY "Kein Speicherplatz mehr."
ADM_PARSE_MAX_NUMBER "Maximaler numerischer Wert Åberschritten."
ADM_PARSE_RANGE "Fehlerhafte Bereichsangabe."
ADM_BAD_ATTACH "Fehlerhafte Anschlu·typ-Angabe."
ADM_BAD_DEV_TYPE "Fehlerhafte GerÑtetyp-Angabe."
ADM_BAD_FAILURE_ACTION "Fehlerhafte failureaction-Angabe"
ADM_BAD_USER_ACCESS "Fehlerhafte Benutzerzugriff-Angabe."
ADM_BAD_VPI_PPI_OPTION "Falsche Option VPI/PPI."
ADM_BAD_DRIVE_OPTION "Falsche Laufwerksoption."
ADM_MISSING_VPI_DEV "Erforderliche vpidevice-Angabe fehlt."
ADM_MISSING_PPI_DEV "Erforderliche ppidevice-Angabe fehlt."
ADM_MISSING_DRIVE_NAME "Erforderliche Laufwerksnamenangabe fehlt."
ADM_MISSING_PRINTER_CMD "Erforderliche Druckerbefehlsangabe fehlt."
ADM_IRQ_MISMATCH "irq und physicalirq stimmen nicht Åberein."
ADM_DMA_MISMATCH "dma und physicaldma stimmen nicht Åberein."
ADM_IOPORTS_MISMATCH "ioports und physicalioports stimmen nicht Åberein."
ADM_MEM_MISMATCH "memmappedio und physicalmemmappedio stimmen nicht Åberein."
ADM_SYSERR "Systemfehler."
ADM_CLASS_NOT_FOUND "Klasse nicht gefunden"
ADM_TOKEN_NOT_FOUND "Token nicht gefunden."
ADM_LOCK "Token-Datei kann nicht gesperrt werden."
ADM_TOKEN_EXISTS "Token existiert bereits."
ADM_BAD_TOKEN "Token-Namen sind auf die Zeichen A-Z, a-z 0-9 und \"-\" begrenzt."
ADM_NO_ATTACH "Erforderliche Anschlu·typ-Angabe fehlt."
ADM_PARSE_OPTION "Unerkannte Optionswerte."
ADM_BAD_UMB "UnzulÑssige UMB-Angabe. Sie mu· innerhalb des Bereichs 0xA0000-0xFFFF liegen."
$       -       admconvert-Meldungen
ADMCVT_CANT_OPEN "\"%1\" kann nicht geîffnet werden.\n"
ADMCVT_CANT_CREATE "Erstellen von \"%1\" nicht mîglich\n"
ADMCVT_BAD_FORMAT "Formatfehler in /etc/dosdev.\n"
ADMCVT_MEMORY "kein Speicherplatz mehr\n"
ADMCVT_CANT_ADD "Token \"%1\" kann nicht hinzugefÅgt werden: %2\n"
ADMCVT_LP_CONVERT "/usr/lib/merge/lp kann nicht umgewandelt werden.\n"
ADMCVT_DOS_DRIVE_NAME "UrsprÅngliches DOS %1:"
ADMCVT_COM1_NAME "DOS Direkter IRQ 4"
ADMCVT_COM2_NAME "DOS Direkert IRQ 3"
ADMCVT_PORT1_NAME "DOS Direkte Schnittstelle 1"
ADMCVT_PORT2_NAME "DOS Direkte Schnittstelle 2"
$	-	Configuration library errors
CFG_NO_ERROR		"Interner Fehler - kein Fehler."
CFG_PARSE_INTERNAL_ERROR "Interner Fehler in parse."
CFG_PARSE_VARIABLE	"Unerkannter Variablenname."
CFG_PARSE_ACCEL		"Unerkannter Schnelltastenwert. Verwenden Sie \"dos\" oder \"x\"."
CFG_PARSE_BOOLEAN	"Unerkannter boole'scher Wert. Verwenden Sie \"true\" oder \"false\"."
CFG_PARSE_DISPLAY	"Unerkannter Anzeigetypwert. Verwenden Sie \"auto\", \"vga\", \"cga\", \"mono\", oder \"hercules\"."
CFG_PARSE_DRIVE_DEF	"Unerkannte Laufwerkdefinition: Verwenden Sie ein Laufwerkstoken oder \"none\"."
CFG_PARSE_DRIVE_OPTION	"Unerkannte Laufwerksoption. Verwenden Sie \"readonly\" oder \"exclusive\"."
CFG_PARSE_DRIVE_LETTER	"UnzulÑssiger Laufwerksbuchstabe. Nur C bis J sind zugelassen."
CFG_PARSE_NUMBER	"UnzulÑssiger numerischer Wert."
CFG_PARSE_SCALE_VALUE	"Unerkannter scaledosgraphics-Wert. Verwenden Sie \"auto\", \"1\"," oder \"2\"."
CFG_PARSE_NO_NUMBER	"Es mu· eine Nummer angegeben werden."
CFG_PARSE_OPTION	"Unerkannte Optionswerte."
CFG_PARSE_FONT		"Unerkannter Schriftartwert. Verwenden Sie \"auto\", \"small\", oder \"medium\"."
CFG_UNZOOM_KEY		"Es mu· ein X-SchlÅsselname angegeben werden."
OPTCVT_BAD_USER		"Kann keine Informationen fÅr aktuellen Benutzer bekommen."
OPTCVT_BAD_USER2	"Kann keine Informationen fÅr Benutzer %1 bekommen.\n"
OPTCVT_NOT_DIR		"Das Konfigurationsverzeichnis existiert bereits."
OPTCVT_MKDIR_FAILED	"Konfigurationsverzeichnis kann nicht erstellt werden."
OPTCVT_WRITE_FAILED	"Konfigurationsdateien konnten nicht geschrieben werden."
OPTCVT_UNLINK		"Optconvert konnte die alten Konfigurationsdateien nicht entfernen."
OPTCVT_ADM_ERROR	"Verwaltungsfehler wÑhrend der Konvertierung."
OPTCVT_BAD_OPTIONS	"Befehlsformat: optconvert [-c Benutzer]\n"
OPTCVT_NO_CONVERT	"Konfiguration existiert bereits fÅr Benutzer %1.\n"
OPTCVT_BAD_LOG		"Protokoll %1 kann nicht geschrieben werden.\n"
OPTCVT_BAD_CONVERT	"+a%1 kann nicht umgewandelt werden.\n"
CFG_BAD_DIR		"Konfigurationsverzeichnis kann nicht geîffnet werden."
CFG_MEMORY		"Kein Speicherplatz mehr."
CFG_SYSERR		"Systemfehler."
CFG_BAD_NAME		"UnzulÑssiger Konfigurationsname."
CFG_NOT_DEL	"Sie kînnen die Konfigurationen \"dos\" oder \"win\" nicht lîschen."
CFG_HEADER1	"Diese Datei wurde mit Merge-Konfigurationsdienstprogrammen erstellt."
CFG_HEADER2	"Verwenden Sie die Konfigurationsdienstprogramme, um énderungen an dieser Datei vorzunehmen."
$	-	mrgconfig
CFG_BAD_CMD	"Interner Fehler - unerkannter Befehl.\n"
CFG_YES_RESP	"y"
CFG_NO_RESP	"n"
CFG_CONFIRM	"Wollen Sie \"%1\" wirklich lîschen?"
CFG_RESPONSE	"Bitte antworten Sie \"%1\" oder \"%2\"\n"
CFG_CANT_DELETE "Lîschen von \"%1\" nicht mîglich."
CFG_DELETED	"Konfiguration \"%1\" gelîscht.\n"
CFG_NOT_DELETED	"Konfiguration \"%1\" nicht gelîscht.\n"
CFG_ADD_USAGE_CMD "Befehlsformat: addmrgconfig alte_Konfig. [copyto] neue_Konfig.\n"
CFG_DEL_USAGE_CMD "Befehlsformat: delmrgconfig config.\n"
CFG_LIST_USAGE_CMD "Befehlsformat: listmrgconfig\n"
CFG_DEL_OPT_ERROR "Sie kînnen nur customdev-Optionen lîschen.\n"
CFG_SET_OPT_ERROR "Sie kînnen nur eine Option angeben.\n"
CFG_USAGE_CMD	"Befehlsformat: mrgconfig config list Option[,Option...].\n               mrgconfig config set Option\n               mrgconfig config delete Option\n"
CFG_COPYTO_STR	"copyto"
CFG_LIST_STR	"list"
CFG_SET_STR	"set"
CFG_DELETE_STR	"delete"
CFG_ALREADY_EXISTS "Konfiguration \"%1\" existiert bereits.\n"
CFG_BAD_OPTION	"Option - \"%1\" nicht erkannt.\n"
CFG_WRITE_ERROR	"Interner Fehler - write fehlgeschlagen.\n"
CFG_DEL_VALUE_ERROR "Sie mÅssen einen customdev-Wert angeben.\n"
CFG_DEL_NOT_FOUND "Individuelles GerÑt \"%1\" existiert nicht in Konfiguration \"%2\".\n"
CFG_READ_ERROR  "Interner Fehler - Konfiguration kann nicht gelesen werden.\n"
CFG_LIST_ERROR	"Liste der Konfigurationen nicht erhalten.\n"
CFG_NOLIST_MSG	"Es sind keine Konfigurationen aufzufÅhren.\n"
$ SCCSID(@(#)install.msg	1.23 LCC) Modified 16:45:28 9/7/94
$ Messages for the install and remove scripts.
$domain LCC.MERGE.UNIX.INSTALL
$quote "

GEN_ERR_1 "Fehler verhindert den Abschlu· der Installation.\n"
GEN_ERR_2 "%1 auf Ausgabefehler untersuchen.\n"
GEN_ERR_3 "Schlie·en und Beenden...\n"
GEN_ERR_4 "Fehler beim Entfernen.\n"
GEN_ERR_5 "Weiter ...\n"
GEN_ERR_6 "Kernel wird nicht wiederaufgebaut.\n"

BUILD_FAIL "Kernel-Aufbau fehlgeschlagen.\n"
LU_FAIL   "link_unix fehlgeschlagen\n"
IDB_FAIL  "idbuild fehlgeschlagen\n"

REPL_ERR  "%1 (aktuell installiert) kann nicht ersetzt werden.\n\
%2 vor der Neuinstallation entfernen.\n"

I_WARN_1   "Im laufenden Kernel ist %1 bereits installiert.\n"
I_ERR_1   "%1 kann in diesem System nicht installiert werden.\n"
I_ERR_2   "Das Unix Base Package mu· eine UnterstÅtzung fÅr %1\n\
enthalten, sonst ist zunÑchst eine\n\
Aktualisierung von UNIX Base Package zu installieren.\n"

KERNEL_HAS_MERGE "\n\
Beim laufenden UNIX Kernecho ist %1 installiert. %2 ist vor der\n\
Neuinstallation ganz zu entfernen, sonst kînnen Installationsfehler\n\
auftreten und Kern wird mîglicherweise nicht sachgemÑ· verkettet.\n"

NO_STREAMS "\n\
Streams-Treiber nicht installiert.  Der Streams-Treiber mu· installiert\n\
sein, damit %1 installiert werden kann.\n"

CANT_INST  "Installation von %1 nicht mîglich.\n"
CANNOT_REMOVE "Entfernen von %1 nicht mîglich.\n"
NOSPACE    "\       Platz zum Wiederaufbau des Kernel nicht ausreichend.\n"

INSTALL_1 "%1 Dateien werden installiert.  Bitte warten Sie...\n"
INSTALL_2 "%1 Dateien werden konfiguriert.  Bitte warten Sie...\n"

REMOVE_3 "%1 Dateien werden entfernt.  Bitte warten Sie...\n"
REMOVE_4 "Die Installation der Verteilerdateien %1 wurde rÅckgÑngig gemacht.\n"

IDIN_FAIL "idinstall von %1 fehlgeschlagen.\n" \
ALREADY_INSTALLED "Warnung: Treiber %1 bereits installiert.\n"
IDCHK_FAIL  "idcheck -p von %1 zurÅckgegeben %2\n"

LINK_FAIL_1 "VerknÅpfung einer Datei fehlgeschlagen.\n"
LINK_FAIL_2 "VerknÅpfung fehlgeschlagen: %1 %2\n"
CPIO_CP_FAIL "Kopieren von Dateien mit cpio fehlgeschlagen.\n"
IDTUNE_FAIL "idtune fehlgeschlagen\n"

$ The following five are used in install and remove when the user is
$ prompted if a re-link of the kernel is wanted.  YESES and NOES are
$ each a list of the acceptable single character responses for yes and no.
YESES "jJ"
NOES "nN"
LINK_NOW_Q "Mîchten Sie jetzt einen neuen Kern aufbauen? (j/n) "
Y_OR_NO "Antworten Sie mit \"j\" oder \"n\": "
REBUILD_WARN "Sie mÅssen den Kern neu aufbauen, damit %1 richtig funktioniert.\n"
REBUILD_NEED "Sie mÅssen den Kern neu aufbauen, damit das System richtig funktioniert.\n"

$ This section (to the line with END-PKGADD) is PKGADD/SVR4 specific.

INST_LOG_HDR "\
# Diese Protokolldatei wurde mit dem Installationsproze· %1 erstellt.\n\
# Wenn wÑhrend dem von %2 gesteuertenTeil der Installation ein Fehler\n\
# auftritt, wird er hier angezeigt. Die Datei wird wÑhrend der Installation\n\
# erstellt und kann vom Benutzer gelîscht werden. Diese Datei\n\
# kann ebenfalls nicht-Fehler-Ausgabe von anderen (Standard-) UNIX\n\
# Programmen enthalten.\n"

REM_LOG_HDR "\
# Diese Protokolldatei wurde mit dem Entfernproze· %1 erstellt.\n\
#  Wenn wÑhrend des von %2 gesteuerten Teil des Entfernens ein\n\
# Fehler auftritt, wird er hier angezeigt.  Die Datei wird wÑhrend des\n\
# Entfernens erstellt und kann vom Benutzer gelîscht werden. Diese Datei\n\
# kann ebenfalls nicht-Fehler-Ausgabe von anderen (Standard-) UNIX\n\
# Programmen enthalten.\n"

INST_NO_VERS "\
FEHLER: Versionsdatei nicht gefunden\n\
       Merge kann nicht installiert werden\n"

PO_ERR_MSG1 "Druckereintrag (%1) kann nicht erstellt werden.\n"

INST_USER_STARTUP "Aktualisierung der UNIX-Benutzerstartdateien ...\n"
INST_PRINTER "Installation der DOS Druckerschnittstelle ...\n"
INST_CONFIG_SYS "Allgemeine config.sys wird aktualisiert ...\n"
INST_AUTOEXEC "Allgemeine autoexec.bat wird aktualisiert ...\n"
INST_DRV_MSG "Installation von %1 Treibern ...\n"
INST_MOD_MSG "\tModul: %1\n"
INST_MERGE "MERGE-Treiber wird aktiviert ...\n"
INST_DOSX "Der DOSX-Treiber wird aktiviert ...\n"
INST_SEG_WRAP "SEG WRAP-Treiber wird aktiviert ...\n"
INST_DONE "Installation von %1 ist abgeschlossen.\n"
INST_CANCEL "Installation vom Benutzer abgebrochen.\n"
INST_DRV_SH "Fehler bei der Installation der Merge-Treiber.\n"
INST_MRG_SH "Probleme bei der Installation der Merge-Dateien.\n"
INST_X_SH "Probleme bei der Installation der Merge X-Dateien.\n"

REM_USER_STARTUP "Aktualisierung der UNIX-Benutzerstartdateien ...\n"
REM_DOS_FILES "DOS Dateien werden entfernt (aus dosinstall) ...\n"
REM_PRINTER "DOS Druckerschnittstelle wird entfernt ...\n"
REM_PRINTER_NOT "DOS Druckerschnittstelle wird nicht entfernt.\n"
MISSING_PROG "Programm %1 fehlt.\n"
REM_CONFIG_SYS "Allgemeine config.sys wird aktualisiert ...\n"
REM_AUTOEXEC "Allgemeine autoexec.bat wird aktualisiert ...\n"
REM_DRIVERS "%1 Treiber werden entfernt ...\n"
REM_MERGE "MERGE-Treiber wird deaktiviert ...\n"
REM_DOSX "DOSX-Treiber wird deaktiviert ...\n"
REM_SEG_WRAP "SEG WRAP-Treiber wird deaktiviert ...\n"
REM_DONE "%1 wurde vollstÑndig entfernt.\n"
REM_CANCEL "Entfernen vom Benutzer abgebrochen.\n"

REM_PROB_MODLIST "%1/modlist nicht gefunden, es wurden keine Module entfernt.\n"
REM_PROB_MODULE "%1 kann nicht entfernt werden.\n"

# request script
REQ_PREV_INSTALL "\n\
$MERGENAME ist bereits installiert.\n\
Es mu· erst entfernt werden, bevor es neu installiert werden kann.\n\n"

$ END-PKGADD
$ SCCSID(@(#)scripts.msg	1.2 LCC) Merge 3.2.0 Branch Modified 12:38:24 10/14/94
$ SCCSID(@(#)scripts.msg	1.40 LCC) Modified 23:55:19 10/10/94
$ English message file for the Merge scripts.
$quote "

$domain LCC.MERGE.UNIX.SET_NLS
$ Messages for the "set_nls" script.
WARN_PREFIX "WARNUNG: "
ERR_PREFIX "FEHLER: "

$domain LCC.MERGE.UNIX.MKVFLP
$ Messages for the "mkvfloppy" script.
USAGE "BEFEHLSFORMAT: %1 vollstÑndiger_Dateipfadname [-s]\n"

$domain LCC.MERGE.UNIX.DOSBOOT
$ Messages for the "dosboot" script.
ERR1 "FEHLER: Option \"+l\" kann nicht mit %1 verwendet werden.\n"

$domain LCC.MERGE.UNIX.INITDPART
$ Messages for the "initdospart" script.

INSERT "HD-Diskette in Laufwerk A: einlegen und <EINGABETASTE> drÅcken: "
FORMAT "DOS Plattenpartition formatieren.\n"
ERR_1  "FEHLER: Es gibt keine DOS Plattenpartiton.\n"
ERR_2  "FEHLER:  Nur Root darf die DOS Partition formatieren.\n"
ERR_3  "FEHLER:  Die Diskette kann nicht formatiert werden.\n\
\n\
öberprÅfen Sie, ob sie nicht schreibgeschÅtzt ist.\n"

ERR_4  "FEHLER:  Die Diskette scheint fehlerhaft zu sein.\n"

TEXT_1 "Die DOS-Partition ist vor dem Gebrauch zu\n\
\n\
formatieren. Die DOS Plattenpartition mu· nicht formatiert werden,\n\
wenn die Formatierung bereits durchgefÅhrt und die Partition\n\
seither weder vergrî·ert noch verschoben wurde.\n\
\n\
Warnung: Die Formatierung der DOS Partition wird alle darin\n\
\           befindlichen Daten lîschen.\n\
Warnung: Um die DOS Partition zu formatieren, mÅssen Sie das System\n\
\           herunterfahren und von einer Diskette neu starten.\n\
\n\
Sie benîtigen dafÅr eine HD-Diskette. Stellen Sie sicher,\n\
da· sie nicht schreibgeschÅtzt ist. Diese Diskette wird formatiert\n\
und in eine Startdiskette umgewandelt werden.\n"

$ NOTE: the CONTINUE, YESES, NOES and Y_OR_N string are closely related.
$ The CONTINUE and Y_OR_N are prompts for a yes or no answer.
$ They don't have newline chars, so the cursor will stay at the end
$ of the printed line.
$ The YESSES and NOES strings define the recognized single character
$ yes or no answers.
$ The YESES  string is a list of single characters that mean "yes".
$ The NOES  string is a list of single characters that mean "no".
CONTINUE "Mîchten Sie weitermachen (j/n)? "
Y_OR_N "Antworten Sie mit \"j\" oder \"n\": "
YESES "jJ"
NOES  "nN"

RETRY "Verwenden Sie eine unbeschÑdigte HD-Diskette.\n\
\n\
Mîchten Sie es noch einmal versuchen (j/n)? "

REBOOT "\n\
Um die DOS Partition zu formatieren, mu· das System von der\n\
Diskette geladen werden, die sich gerade im Laufwerk befindet.\n\
\n\
Die Diskette NICHT herausnehmen\n\
\n\
<EINGABETASTE> drÅcken, um das System vor dem Neustart herunterzufahren. "

EXIT   "Die DOS Partition wird nicht formatiert.\n"
EXIT_2 "Die DOS Partition mu· vor dem Gebrauch formatiert werden.\n"


$domain LCC.MERGE.UNIX.DOSINSTALL
$ Messages for the dosinstall and dosremove scripts.
$    (sourcefiles: unix/janus/dosinst.sh, unix/janus/dosremove.sh)
EXITING "Wird beendet...\n"
ERR_0 "DOS kann nicht installiert werden.\n"
ERR_1 "Nur Root oder Superuser kînnen DOS installieren.\n"
ERR_2 "DOS ist bereits installiert. Das aktuell installierte DOS\n\
mu· vor der Neuinstallation entfernt werden.\n"
CANNOT_CONTINUE "Fortfahren nicht mîglich.\n"
$ The BAD_FLOP_NAME error can happen when the user used the "DRIVE_DEV"
$ variable to specify which floppy device to use, and the device does
$ not exist.  If the user did not set DRIVE_DEV, then the internal
$ logic that determines the device filename did not work.  To work around
$ this problem, the use should set DRIVE_DEV to the device name to use.
BAD_FLOP_NAME "UngÅltiger GerÑtename des Diskettenlaufwerks '%1'\n"

RM_MSG "DOS wird entfernt\n"
ERR_RM "Schwierigkeiten beim Entfernen von DOS.\n\
Installation von DOS kînnte fehlschlagen.\n"
RE_INST "DOS wird neuinstalliert\n"
INSTALLING_DOS "DOS wird installiert\n"
INSTALLING_DOSVER "%1 wird installiert\n"
DOS_INST_DONE "DOS Installation abgeschlossen.\n"

DSK_DONE "Sie kînnen die Diskette jetzt herausnehmen.\n"

DOSINST_MKIMG_FAILED "Nicht alle DOS Bilddateien wurden erstellt.\n\
\     Die Installation von DOS ist teilweise fehlgeschlagen.\n"

DOSINST_MKIMG_MONO_FAILED "Erstellen von Mono-DOS Bilddateien nicht mîglich.\n\
\     Installation von DOS wird abgebrochen.\n"

NO_SPACE    "     Nicht genÅgend Platz.\n"
NO_SPACE_IN "     Nicht genÅgend Platz in %1\n"
NEED_SPACE  "     Benîtigt: %1 Blîcke, Vorhanden: %2.\n"

DOSINST_CANT "%1 kann nicht installiert werden\n"

DOSINST_DOSBRAND_MENU1 "WÑhlen Sie aus, welches DOS Sie installieren wollen.\n"

DOSINST_DOSBRAND_MENU2 "\
\      0: Keines der oben genannten. DOS Installation abbrechen.\n\
\n\
Geben Sie die Optionsnummer ein und drÅcken dann die Eingabetaste: "

DOSINST_01 "Geben Sie 0 oder 1 ein\n"
DOSINST_012 "0, 1 oder 2 eingeben\n"
DOSINST_0123 "Geben Sie 0, 1, 2 oder 3 ein\n"
DOSINST_01234 "Geben Sie 0, 1, 2, 3 oder 4 ein\n"
DOSINST_012345 "Geben Sie 0, 1, 2, 3, 4 oder 5 ein\n"
DOSINST_0123456 "Geben Sie 0, 1, 2, 3, 4, 5 oder 6 ein\n"
DOSINST_01234567 "Geben Sie 0, 1, 2, 3, 4, 5, 6 oder 7 ein\n"

DOSINST_SCO_DISKSIZE "\
Bitte geben Sie die Grî·e Ihrer DOS Disketten ein.\n\
\n\
\      1: 3,5 Zoll LD-Disketten.\n\
\      2: 3,5 Zoll HD-Disketten.\n\
\      3: 3,5 Zoll Disketten unbekannte Dichte.\n\
\      4: 5,25 Zoll LD-Disketten.\n\
\      5: 5,25 Zoll HD-Disketten.\n\
\      6: 5,25 Zoll Disketten unbekannter Dichte.\n\
\      0: Keine der oben genannten. DOS Installation abbrechen.\n\
\n\
Geben Sie die Optionsnummer ein und drÅcken die Eingabetaste: "

DOSINST_FLOP_P "\
Von welchem Diskettenlaufwerk wird die Installation durchgefÅhrt?\n\
\n\
\      1: Erstes Laufwerk.  (Laufwerk '%1')\n\
\      2: Zweites Laufwerk. (Laufwerk '%2')\n\
\      0: Stop.  DOS Installation abbrechen.\n\
\n\
Geben Sie die Nummer ein und drÅcken dann die Eingabetaste: "

DOSINST_MSG1 "Diskette %1 von %2 wird installiert.\n"

DOSINST_FROM "Installation von Laufwerk %1.\n"

WRONG_DISK_1 "\
\      Die Diskette scheint nicht die richtige zu sein.\n\
\      Bitte prÅfen Sie, ob die richtige Diskette im angegebenen Laufwerk ist.\n"

WRONG_DISK_N "\
\      Diese Diskette wurde bereits als Diskette %1 gelesen.\n\
\      Bitte prÅfen Sie, ob die richtige Diskette im angegebenen Laufwerk ist.\n"

DOSINST_DW_P "\
Was mîchten Sie jetzt tun:\n\
\      0: Die Installation von DOS abbrechen.\n\
\      1: Erneut versuchen.\n\
Geben Sie 0 oder 1 ein, und drÅcken Sie die Eingabetaste: "

DOSINST_DW_R "Geben Sie 0 oder 1 ein\n"

DOSINST_NOTINST "DOS wird nicht installiert.\n"
DOSINST_AUTO_NA "\
Automatische Installation von DOS nicht mîglich.\n\
FÅhen Sie das Programm 'dosinstall' aus, um DOS von Ihrem DOS\n\
Diskettenlaufwerk aus zu installieren.\n"

CHECK_DISK  "Diskette wird ÅberprÅft."
INSERT_S "DOS Diskette %1 in Laufwerk einlegen und EINGABETASTE drÅcken. "
INSERT_D "DOS Diskette %1 in Laufwerk %2 einlegen und EINGABETASTE drÅcken. "
INSERT_NEXT "Diskette %1 herausnehmen, Diskette %2 einlegen und EINGABETASTE drÅcken. "
INST_CANCEL "Installation vom Benutzer abgebrochen.\n"
INST_SYS "DOS Systemdateien werden installiert.\n"
READING_MESSAGE "Diskette wird gelesen."
CANNOT_CREATE "Fehler: %1 konnte nicht erstellt werden\n"
NO_FILE "Fehler: %1 fehlt\n"
MISSING_DIR "Fehler: Verzeichnis %1 fehlt"
CREATING_DIR "Warnung: Fehlendes Verzeichnis %1\n\
Einrichtung des Verzeichnisses.\n"

$ Note: Q_REMOVE, Q_CONTINUE, and Q_ANYWAY don't end in a newline.
$ Also, the answer to them must be either the string in ANS_Y or ANS_N or ANS_Q.
Q_REMOVE "Mîchten Sie das aktuell installierte DOS jetzt entfernen (j/n/b)? "
Q_CONTINUE "Mîchten Sie die DOS Installation fortsetzen (j/n/b)? "
Q_ANYWAY "Mîchten Sie DOS trotzdem entfernen (j/n/b)? "
Q_PROMPT "Antworten Sie mit j, n oder b\n"
ANS_Y "y"
ANS_N "n"
$ Note: ANS_Q is what users can type at all prompts to "quit" or abort
$ the installation, (also entering 0 does the same thing as ANS_Q).
ANS_Q "q"
DOSINST_0Q_MSG "\
Wenn Sie nach der Eingabeaufforderung 0 oder b eingeben, wird die DOS Installation abgebrochen.\n"
CANCEL_MSG "Die Neuinstallation von DOS wird abgebrochen.\n"
IMPROPER "DOS ist nicht richtig installiert.\n"
RM_ABORT "Entfernen von DOS wird abgebrochen. Es wurden keine Dateien entfernt.\n"
RM_PROB "Fehler: Probleme beim Entfernen von DOS.\n"
RM_PROB_1 "Fehlende Dateiliste. DOS kann nicht vollstÑndig entfernt werden.\n"
RM_ALMOST "DOS wurde entfernt.\n"
RM_DONE "DOS wurde entfernt.\n"
BAD_DISK "\
\     Probleme beim Lesen der Diskette.\n\
\     öberprÅfen Sie, ob die Diskette richtig ins Laufwerk eingelegt ist.\n"
CLEAN_UP "\
Zum Abschlu· der DOS Installation mehr Plattenspeicher bereitstellen und neu starten.\n"
VDISK_ERR "Fehler auf virtueller Platte. DOS kann nicht installiert werden.\n"
MISSING_FILE "Fehler: %1 nicht gefunden\n"
INTERNAL_ERR "Interner dosinstall-Fehler %1\n"
EXPANDING "Die DOS Dateien werden erweitert.\n"
CONFIGURING "Die DOS Dateien werden konfiguriert.\n"
DRIVEA_NAME "A"
DRIVEB_NAME "B"
DOSINST_NDISKS_Q "Anzahl der %1-Disketten in Ihrem Satz: "
DOSINST_0_9 "Geben Sie eine Zahl zwischen 0 und 9 ein\n"
DOSINST_0_ABORT "Wenn Sie 0 eingeben, wird die Installation von DOS abgebrochen.\n"
FROM_BUILTIN "(von eingebauten Dateien)"
FROM_FLOP "(von Disketten)"
MIN_SYSTEM "Minimumsystem"
DOSINST_PLS_ENTER "Bitte geben Sie %1 ein\n"
MISSING_SET "'set'-Dateien fehlen\n"
CREATE_BOOT_FAIL "\
Warnung: neue boot.dsk-Datei wurde nicht erstellt.\n\
\         Existierende boot.dsk-Datei wird verwendet.\n"
BOOT_TOOSMALL "\
Warnung: Kann keine virtuelle Platte mit ausreichendem Speicherplatz\n\
\         erstellen um alle DOS NLS-Funktionen zu initialisieren,\n\
\         deaktiviere also automatische DOS NLS-Funktionen.\n"

$domain LCC.MERGE.UNIX.CHECKMERGE
$ Messages for the "checkmerge" script.
USAGE "BEFEHLSFORMAT: %1\n"
MSG_1 "Vergleich der Datei-PrÅfsummen fÅr\n"
MSG_2 "mit den Werten in %1...\n"
MSG_3 "wurde seit der letzten Installation von Merge geÑndert.\n"
MSG_4 "PrÅfsumme in %1 kann nicht ermittelt werden.\n\
Fahre fort...\n"
MSG_5 "DateiprÅfung ausgefÅhrt.\n"

$domain LCC.MERGE.UNIX.LPINSTALL
$ Messages for the lpinstall script.
MAIN_MENU "\n\
\n\
\n\
\         Merge Drucker-Installationsprogram\n\
\         ----------------------------------\n\
\n\
\         1)  Drucker installieren\n\
\         2)  Drucker entfernen\n\
\         3)  Drucker auflisten\n\
\n\
\         %1)  Beenden\n\
\n\
\         ----------------------------------\n\
\         Geben Sie die Option ein: "
$ Note: QUIT_CHAR must be a single character that mean quit.  It is used
$ as the %1 in the MAIN_MENU message and is compared with what the user
$ typed as the response to the "enter option" question.
QUIT_CHAR "q"
INSTALL "\n\
\         INSTALLIEREN\n
\         ---------------------\n"
PRINTER "        Druckername [%1]: "
MODEL   "        Druckermodell [%1]: "
DEVICE  "        GerÑt [%1]: "
$ Note: YES_CHAR must be a single character that means yes.  It is used
$ as the %1 in the OK_INST and  %2 in OK_REMOVE messages and is compared
$ with what the user typed as the response to those messages.
YES_CHAR "y"
OK_INST  "        Installieren OK? [%1]: "
OK_REMOVE "        %1 entfernen OK? [%2]: "
CONTINUE "        DrÅcken Sie die [EINGABETASTE], um fortzufahren"
REMOVE "\n\
\         ENTFERNEN\n\
\         ---------------------\n\
\         Druckername: "
LIST "\n\
Anzahl der installierten Drucker: %1\n\
-----------------------------------------------\n"
MISSING_PROG	"Programm %1 fehlt.\n"
NO_LP		"Mîglicherweise ist der LP-Druckdienst nicht installiert.\n"
CANNOT_CONTINUE	"Fortfahren nicht mîglich.\n"

$domain LCC.MERGE.UNIX.MKIMG
$ Messages for the "mkimg" script.

CANNOT_CONTINUE	"Fortfahren nicht mîglich.\n"
CANNOT_COPY	"Warnung: Datei %1 kann nicht kopiert werden.\n"
CANNOT_MAKE	"Abbild von %1 kann nicht erzeugt werden.\n"
CANNOT_MKDIR	"Verzeichnis kann nicht angelegt werden: %1\n"
CANNOT_READ	"Datei '%1' kann nicht gelesen werden\n"
CANNOT_WRITE	"%1 konnte nicht geschrieben werden\n"
CONFIG_MSG	"DOS Bilddateien werden konfiguriert.\n"
COPY_PROB	"Probleme beim Kopieren von Dateien auf die virtuelle Platte %1\n"
CREATE_NOROOM	"Nicht genÅgend Platz fÅr die Erstellung von Datei %1\n"
DOS_NI		"DOS ist nicht installiert\n"
FORMAT_PROB	"Probleme beim Formatieren der virtuellen Platte %1\n"
IMG_MADE	"Abbild %1 hergestellt.\n"
INCOMPATABLE	"(Inkompatible %1-Karte?)\n"
MAKING		"Abbild %1 wird hergestellt.\n"
MISSING		"Datei %1 fehlt\n"
MUST_SU		"Nur Root oder Superuser kînnen Abbild %1 herstellen.\n"
NATIVE_MUST_SU	"Nur Root oder Superuser kînnen systemeigenes Abbild %1 herstellen.\n"
NATIVE_TOKEN	"systemeigen"
NOT_MADE	"%1 Abbild wurde nicht hergestellt.\n"
NO_DIRECTORY	"Verzeichnis %1 existiert nicht.\n"
NO_IMG_MADE	"Es wurden keine Bilddateien angelegt.\n"
NO_PERMIS_TMP	"Keine Berechtigung zur Erstellung der temporÑren Datei %1\n"
NO_PERMIS_WRITE	"Keine Berechtigung zum Schreiben von %1\n"
NO_ROOM		"(Kein Platz mehr?)\n"
NO_ROOM_TMP	"Nicht ausreichend Platz zur Erstellung der temporÑren Datei %1\n"
OPT_NA		"Option %1 nicht erlaubt.\n"
REMOVE_FAIL	"Datei %1 konnte nicht entfernt werden\n"
SOME_NOT_MADE	"Verschiedene Bilddateien wurden nicht erstellt.\n"
USAGE_LINE	"BEFEHLSFORMAT: %1 [cga] [mda] [ d Verzeichnis ] [devicelow] [+aXXX]\n"
USING_INSTEAD	"Stattdessen Verwendung von %1.\n"
USING_ONCARD	"%1 ROM wird auf der Grafikkarte verwendet.\n"
WRONG_DISP	"Auf Bildschirmtyp %2$2 kann kein Abbild %1$1 erstellt werden.\n"

$domain LCC.MERGE.UNIX.S55MERGE
$ Messages for "S55merge" script.
NOT_DOSINST	"Warnung: DOS wird nicht automatisch installiert.\n"
NOT_MKIMG	"Warnung: DOS Bilddateien werden nicht automatisch erstellt.\n"
DOING_MKIMG	"DOS Bilddateien werden automatisch im Hintergrund erstellt.\n"

$domain LCC.MERGE.UNIX.PART_SET
$ Messages for the "part_set" script.
BAD_DOSDEV	"Defekte Datei '%1'.\n"
MISSING_LINE	"Zeile '%1' fehlt.\n"

$domain LCC.MERGE.UNIX.MERGEFONTMAKE
$ Messages for the "mergefontmake" script (part of X-Clients package).
MFM_USAGE	"\
BEFEHLSFORMAT: mergefontmake  0|1|2|3|4  [s|ML]\n\
Merge-Schriftarten neu kompilieren.\n\
\n\
Schriftarten kînnen auf vier verschiedene Arten kompiliert\n\
werden, und die Schriftart funktioniert nur, wenn sie\n\
in der von Server X erwarteten Weise kompiliert wurde.\n\
Der erste Parameter bestimmt, auf welche der vier\n\
verschiedenen Weisen die Schriftarten kompiliert werden:\n\1, 2, 3 oder 4.\n\
Wird der Parameter '0' angegeben, so wird der Standard verwendet,\n\
welcher auch immer das sein mag.\n\
\n\
'mergefontmake' versucht herauszufinden, welche\n\
Schriftartkompilierung Sie verwenden, denn verschiedene\n\
X-Versionen verwenden verschiedene Optionen fÅr die\n\
Kompilierung von Schriftarten. (Einige Versionen verwenden\n\
die Option '-s' statt '-M' und '-L'). Um den verwendeten\n\
Typ zu erzwingen, geben Sie 's' oder 'ML' als zweiten\n\
Parameter an.\n"
MFM_MISSING	"Fehler: %1 fehlt\n"
MFM_BAD_SECONDP	"Fehler: Als zweiter Parameter mu· 's', 'ML' oder nichts angegeben werden.\n"
MFM_NO_XBIN	"Fehler: X bin-Verzeichnis nicht gefunden.\n"
MFM_NO_FONTDIR	"Fehler: Das Schriftartverzeichnis kann nicht bestimmt werden.\n"
MFM_I_NOPERMIS	"Fehler: Keine Berechtigung zur Installation von Schriftarten in %1\n"
MFM_01234	"Fehler: Sie mÅssen 0, 1, 2, 3 oder 4 verwenden.\n"
MFM_U_NOPERMIS	"Fehler: Keine Berechtigung zur Aktualisierung von %1\n"
MFM_FC_ERR	"Fehler: %1 gab %2 zurÅck\n"
MFM_COMP_ERR	"Fehler bei der Kompilierung von %1\n"

$domain LCC.MERGE.UNIX.WINSETUP
$ Messages for the "winsetup" script.  This scripts runs when the
$ "Win_Setup" icon is used, or the user types "winsetup".
$ There are a maximum of 24 lines of message that can be printed out,
$ altough all don't have to be used.
$ Also, a single line message can be put into "SPCL_MSG", which is printed
$ out before the batch scripts are run.
$ All these messages are printed on the DOS screen, so these messages
$ must be in the appropriate DOS code page.
LINE01 "⁄ƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒø"
LINE02 "≥         Verwenden Sie diese DOS Sitzung, um Windows 3.1 zu installieren     ≥"
LINE03 "√ƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒ¥"
LINE04 "≥                                                                             ≥"
LINE05 "≥ - Um eine persînliche Kopie von Windows zu installieren, legen Sie          ≥"
LINE06 "≥    Ihres Windows-Pakets ein und geben bei Installation von Laufwerk A:      ≥"
LINE07 "≥                                                                             ≥"
LINE08 "≥                a:\\setup                                                     ≥"
LINE09 "≥                                                                             ≥"
LINE10 "≥   (oder b:\\setup, wenn von Laufwerk B: installiert wird)                    ≥"
LINE11 "≥                                                                             ≥"
LINE12 "≥   Schlagen Sie in Ihrem Handbuch nach einer schrittweisen Beschreibung der  ≥"
LINE13 "≥   Installationsprozedur nach. Das Handbuch beschreibt ebenfalls alternative ≥"
LINE14 "≥   Installationsszenarien.                                                   ≥"
LINE15 "≥ - Um Windows neu zu konfigurieren, wechseln Sie in das Laufwerk und         ≥"
LINE16 "≥   Verz., in dem Windows installiert wurde, und geben dann folgendes ein.    ≥"
LINE17 "≥                                                                             ≥"
LINE18 "≥                setup                                                        ≥"
LINE19 "≥                                                                             ≥"
LINE20 "¿ƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒŸ"
$quote "
$domain LCC.PCI.UNIX.LSET_ESTR
$ The following messages are from lset_estr.c
LSET_ESTR0 "kein Fehler"
LSET_ESTR1 "Systemfehler"
LSET_ESTR2 "Parameterfehler"
LSET_ESTR3 "Tabelle ist gesperrt/entsperrt"
LSET_ESTR4 "Sperren/Entsperren der Tabelle nicht mîglich"
LSET_ESTR5 "IPC-Identifikator nicht erhalten"
LSET_ESTR6 "Sperrsatz erstellen bzw.darauf zugeifen nicht mîglich"
LSET_ESTR7 "kann gemeinsames Speicherplatzsegment nicht anschlie·en"
LSET_ESTR8 "kein Speicher-/Tabellenplatz"
LSET_ESTR9 "angegebener Steckplatz ist belegt"
LSET_ESTR10 "angegebener Steckplatz ist nicht belegt"
LSET_ESTR11 "Sperrsatz existiert bereits"
LSET_ESTR12 "Zugriff verweigert"
LSET_ESTR13 "unbekannter LSERR-Fehler (%d)"

$ @(#)messages	4.5 9/6/94 14:20:37

$quote "
$domain LCC.PCI.UNIX.RLOCKSHM

$ The following messages are from rlockshm.c
RLOCKSHM6 "Config ist nur mit neuartigem gemeinsam benutztem Speicher gÅltig.\n"
RLOCKSHM7 "%1 von gemeinsam genutztem Speicher nicht mîglich, "
RLOCKSHM8 "(Systemfehler: %1).\n"
RLOCKSHM9 "%1.\n"
RLOCKSHM10 "\nStandardkonfigurations-Informationen:\n\n"
RLOCKSHM11 "\tSegmentanschlu·-Adresse\t\t(Programm ausgewÑhlt)\n"
RLOCKSHM12 "\tSegmentanschlu·-Adresse\t\t%1\n"
RLOCKSHM14 "\tSchlÅssel fÅr gemeinsam benutzten Speicher\t\t%1\n"
RLOCKSHM15 "\tSperrsatzschlÅssel\t\t\t%1\n"
RLOCKSHM17 "\tTabelleneintrÑge fÅr geîffnete Dateien\t\t%1\n"
RLOCKSHM18 "\tTabelleneintrÑge fÅr Datei-Header\t%1\n"
RLOCKSHM19 "\tTabelleneintrÑge fÅr Hash-Dateien\t%1\n"
RLOCKSHM20 "\tTabelleneintrÑge fÅr Datensatzsperren\t%1\n"
RLOCKSHM22 "\tEinzelsatzsperren\t\t%1\n"
RLOCKSHM24 "Diese Daten kînnen mit Hilfe folgender konfigurierbarer"
RLOCKSHM25 "Parameternamen geÑndert werden:\n"
RLOCKSHM27 "\t%1 - %2\n"
RLOCKSHM28 "\nWeitere vorhandene Speicherinformationen:\n\n"
RLOCKSHM29 "\tTabelleneintrÑge fÅr geîffnete Dateien\t\t%1 @ %2 Byte\n"
RLOCKSHM30 "\tTabelleneintrÑge fÅr Datei-Header\t%1 @ %2 Byte\n"
RLOCKSHM31 "\tTabelleneintrÑge fÅr Hash-Datei\t%1 @ %2 Byte\n"
RLOCKSHM32 "\tTabelleneintrÑge fÅr Datensatzsperre\t%1 @ %2 Byte\n"
RLOCKSHM34 "\tGeîffnete Segmentbasis\t\t%1\n"
RLOCKSHM35 "\tDateisegmentbasis\t\t%1\n"
RLOCKSHM36 "\tSperrsegmentbasis\t\t%1\n"
RLOCKSHM38 "\tgeîffnete Dateitabelle\t\t\t%1\n"
RLOCKSHM39 "\tDatei-Header-Tabelle\t\t%1\n"
RLOCKSHM40 "\tHash-Dateitabelle\t\t%1\n"
RLOCKSHM41 "\tDatensatz-Sperrtabelle\t\t%1\n"
RLOCKSHM43 "\tFreiliste der Tabelle der geîffneten Dateien \t\t%1\n"
RLOCKSHM44 "\tFreiliste der Dateitabelle\t\t%1\n"
RLOCKSHM45 "\tFreiliste der Sperrsatztabelle\t\t%1\n"
RLOCKSHM46 "\tTabelleneintrÑge fÅr geîffnete Dateien\t\t%1 @ %2 Byte\n"
RLOCKSHM47 "\tTabelleneintrÑge fÅr Datei-Header\t%1 @ %2 Byte\n"
RLOCKSHM48 "\tTabelleneintrÑge fÅr Hash-Datei\t%1 @ %2 Byte\n"
RLOCKSHM49 "\tTabelleneintrÑge fÅr Datensatzsperre\t%1 @ %2 Byte\n"
RLOCKSHM50 "\tgesamte Segmentgrî·e\t\t%1 Byte\n"
RLOCKSHM52 "\tEinzelsatzsperren\t\t%1\n"
RLOCKSHM54 "\tAnschlu·basis\t\t\t%1"
RLOCKSHM55 " (Programm ausgewÑhlt)"
RLOCKSHM57 "\tgeîffnete Dateitabelle\t\t\t%1\n"
RLOCKSHM58 "\tDatei-Header-Tabelle\t\t%1\n"
RLOCKSHM59 "\tHash-Dateitabelle\t\t%1\n"
RLOCKSHM60 "\tDatensatz-Sperrtabelle\t\t%1\n"
RLOCKSHM62 "\tFreilistenindex der Tabelle der offenen Dateien\t%1\n"
RLOCKSHM63 "\tFreilistenindex der Dateitabelle\t%1\n"
RLOCKSHM64 "\tFreilistenindex der Sperrsatztabelle\t%1\n"
RLOCKSHM65 "\tbenîtigter Faktor fÅr die Anpassung\t%1\n"
RLOCKSHM66 "\nEintrÑge der Datei-Headertabelle:\n"
RLOCKSHM100 "Eintrag"
RLOCKSHM101 "Liste der offenen Dateien"
RLOCKSHM102 "Liste der Sperren"
RLOCKSHM103 "Hash-Verweis"
RLOCKSHM104 "Unique-ID"
RLOCKSHM105 "Index der offenen Dateien"
RLOCKSHM106 "Index der Sperren"
RLOCKSHM70 "\nTabelleneintrÑge fÅr Hash-Datei-Header:\n"
RLOCKSHM200 "Eintrag"
RLOCKSHM201 "Datei-Header"
RLOCKSHM75 "\nTabelleneintrÑge fÅr Datensatzsperre:\n"
RLOCKSHM111 "NÑchste_Sperre"
RLOCKSHM112 "Niederwertige_Sperre"
RLOCKSHM113 "Hochwertige_Sperre"
RLOCKSHM114 "Sitzungs-ID"
RLOCKSHM115 "DOS_PID"
RLOCKSHM116 "NÑchster_Index"
RLOCKSHM80 "\nTabelleneintrÑge der geîffneten Dateien:\n"
RLOCKSHM120 "NÑchste_geîffnete_Datei"
RLOCKSHM121 "Header"
RLOCKSHM122 "annehmen"
RLOCKSHM123 "verweigern"
RLOCKSHM130 "Dateikopf-Index"
RLOCKSHM131 "Datei-Beschreiber"
RLOCKSHM_OLDST	"WARNUNG: Altmodischer Speicher wird verwendet -- jetzt veraltet\n"
RLOCKSHM_USAGE	"\nBefehlsformat: %1 [-cdhmrAFHLOV] [Name=Daten] ...\n"
RLOCKSHM_DETAIL "\
\  -c  Gemeinsam genutztes Speichersegment erstellen.\n\
\  -d  (Nur) Standardkonfiguration anzeigen.\n\
\  -h  (Nur) diese Informationen anzeigen.\n\
\  -m  Veschiedene existierende Segmentinformationen anzeigen.\n\
\  -r  Gemeinsam genutztes Speichersegment entfernen.\n\
\  -A  Alle EintrÑge (auch nicht verwendete) anzeigen.\n\
\  -F  Datei-Header-Tabelle anzeigen.\n\
\  -H  Hashed-Datei-Header-Tabelle anzeigen.\n\
\  -L  Satzsperrentabelle anzeigen.\n\
\  -O  Tabelle der geîffneten Dateien anzeigen.\n\
\  -V  (Nur) Version/Copyright drucken.\n\
\  Name=Daten - Keine oder mehere Konfigurationseinstellungen.\n\n"

$ The following messages are from set_cfg.c
$quote "
SET_CFG1 "UnzulÑssige Konfigurations-Zeichenkette: '%1'\n"
SET_CFG2 "Kein Name fÅr '%1' geliefert.\n"
SET_CFG3 "Der Name in '%1' ist zu lang.\n"
SET_CFG4 "Keine Daten fÅr '%1' geliefert.\n"
SET_CFG5 "Unbekannter Name: '%1'\n"
SET_CFG6 "Die Daten fÅr '%1' dÅrfen nicht negativ sein.\n"
SET_CFG7 "Die Daten fÅr '%1' sind ungÅltig.\n"
SET_CFG8 "Daten fÅr '%1' mÅssen weniger als %2 sein.\n"

$ The following are handcrafted from set_cfg.c
SET_CFG100 "Basis"
SET_CFG101 "Segmentanschlu·-Adresse (0 = Programm ausgewÑhlt)"
SET_CFG102 "SchlÅssel"
SET_CFG103 "LSW fÅr gemeinsam genutzten Speicher und SperrsatzschlÅssel festlegen"
SET_CFG104 "offene_Tabelle"
SET_CFG105 "Max. Anzahl an TabelleneintrÑgen fÅr geîffnete Dateien"
SET_CFG106 "Dateitabelle"
SET_CFG107 "Max. Anzahl an TabelleneintrÑgen fÅr Datei-Header"
SET_CFG108 "Hash-Tabelle"
SET_CFG109 "Max. Anzahl an TabelleneintrÑgen fÅr Hash-Datei"
SET_CFG110 "Sperrtabelle"
SET_CFG111 "Max. Anzahl an TabelleneintrÑgen fÅr Datensatzsperre"
SET_CFG112 "Datensatzsperren"
SET_CFG113 "Max. Anzahl an Sperren fÅr jeden Datensatz"
$ SCCSID(@(#)messages	7.11	LCC)	/* Modified: 10:33:25 10/20/93 */

$quote "
$domain LCC.PCI.UNIX

$ In general, the names below start with some indication of the file in
$ which the string is actually used.  Usually, this is the base name of
$ the file, although it may be some "shorter" version of the name.

LICM_VIOLATION	"Lizenzvergehen, %1 (wird beendet) gegen %2\n"
LICM_TERMINATE	"Lizenzvergehen -- Server wird beendet\n"
LICM_BAD_KEY	"UngÅltige Lizenz (fehlerhafter LizenzschlÅssel)\n"
LICM_ALTERED	"UngÅltige Lizenz (verÑnderter Text)\n"
LICM_NO_ID	"UngÅltige Lizenz (keine Lizenz-ID)\n"
LICM_NO_MEMORY	"Unbrauchbare Lizenz (kein Speicher)\n"
LICM_EXPIRED	"Unbrauchbare Lizenz (abgelaufen)\n"
LICM_NO_SERIAL	"Keine Client-Seriennummer angegeben\n"
LICM_BAD_SERIAL	"UngÅltige Client-Seriennummer angegeben\n"

LICU_DUPLICATE	"Doppelte Client-Seriennummer angegeben, Verbindung nicht erlaubt."
LICU_INVALID	"UngÅltige Client-Seriennummer, Verbindung nicht erlaubt."
LICU_RESOURCE	"Host-Ressource ist nicht verfÅgbar, setzen Sie sich mit Ihrem Systemverwalter in Verbindung."

$ %1 is the maximum number of clients
LICU_LIMIT	"Dieser Server hat sein per Lizenz festgelegtes Client-Limit (%1) erreicht, keine Verbindung mîglich."

$ %1 is the log file name, %2 is the error string, %3 and %4 (where used) are
$ the user and group IDs, respectively
LOG_OPEN	"Protokolldatei '%1' kann nicht geîffnet werden, %2\n"
LOG_CHMODE	"Modus von '%1' kann nicht geÑndert werden, %2\n"
LOG_CHOWN	"EigentÅmer von '%1' kann nicht auf %3/%4 geÑndert werden, %2\n"
LOG_REOPEN	"Nach Abschneidung kann '%1' nicht wieder geîffnet werden, %2\n"

$ %1 is a host file descriptor
LOG_OPEND	"Protokoll von Beschreiber %1 kann nicht erneut geîffnet werden\n"

$ %1 is the program name, %2 is the process id
LOG_SERIOUS	"Schwerer Fehler in %1 (PID %2), versuche fortzufahren\n"
LOG_FATAL	"Schwerwiegender Fehler in %1 (PID %2), kann nicht fortfahren\n"

$ %1 is a number of bytes
MEM_NONE	"%1 Byte Speicherplatz kînnen nicht zugeordnet werden.\n"
MEM_RESIZE	"Speichergrî·e kann nicht auf %1 Byte verÑndert werden\n"

$ %1 is the host name
NETAD_NOHOST	"Kann Daten Åber Host '%1' nicht finden\n"

$ %1 is the error string
NETIF_DEVACC	"Kann nicht auf das NetzwerkgerÑt zugreifen, %1\n"
NETIF_CONFIG	"Kann die Netzwerkkonfiguration nicht bestimmen, %1\n"

NETIO_DESC_ARR	"Feld des Netzwerk-Beschreibers kann nicht zugeordnet werden\n"
NETIO_RETRY	"Kann ein Paket aus dem Netzwerk nicht empfangen.\n"
NETIO_CORRUPT	"Interne Netzwerktabelle wurde beschÑdigt\n"
NETIO_MUXERR	"Netzwerk-Multiplexing-Fehler\n"

$ %1 is the size of a network packet
NETIO_PACKET	"Die maximale Netzwerk-Paketgrî·e (%1) ist zu klein\n"

$ %1 is a network address
NETIO_IFERR	"Nicht behebbarer Fehler bei Schnittstelle %1\n"

DOSSVR_NO_CSVR	"Kann die Verbindungs-Serveradresse nicht finden\n"
DOSSVR_SETIF	"Kann die lokale Netzwerkschnittstelle nicht îffnen\n"
DOSSVR_CURDIR	"Kann das gegenwÑrtige Arbeitsverzeichnis nicht bestimmen\n"

$ %1 is the error string
DOSSVR_R_CPIPE	"Kann Konfigurations-Pipe nicht lesen, %1\n"
DOSSVR_G_NETA	"Kann Netzwerkattribute nicht erhalten, %1\n"
DOSSVR_S_NETA	"Kann Netzwerkattribute nicht festlegen, %1\n"
DOSSVR_G_TERMA	"Kann Zeilenattribute fÅr Terminal nicht bekommen, %1\n"
DOSSVR_S_TERMA	"Kann Zeilenattribute fÅr Terminal nicht festlegen, %1\n"
DOSSVR_C_CPIPE	"Kann Konfigurations-Pipe nicht erstellen, %1\n"
DOSSVR_NOFORK	"Kann neuen Proze· nicht erstellen, %1\n"

$ %1 is an RLOCK package error, %1 is a system error
DOSSVR_RLINIT	"Kann Satzsperrendaten nicht initialisieren, %1, %2\n"

$ %1 is a program name, %2 is an error string (if used)
DOSSVR_NOEXEC	"Kann '%1' nicht starten, %2\n"
DOSSVR_NOSHELL	"Kann Benutzer-Shell '%1' nicht starten\n"
DOSSVR_ACC_SVR	"Kann nicht auf den DOS Server '%1' zugreifen\n"
DOSSVR_RUN_SVR	"Kann DOS Server '%1' nicht in Betrieb nehmen, %2\n"

$ %1 is an luid, %2 is the error string
DOSSVR_LUID	"Kann luid nicht auf %1 einstellen, %2\n"

$ %1 is the written count, %2 is the expected count, %3 is the error string
DOSSVR_W_CPIPE	"Kann Konfigurations-Pipe (%1 Byte von %2) nicht schreiben, %3\n"

CONSVR_NOMEM	"Kann Speicher fÅr die Funktionszeichenkette nicht zuordnen\n"
CONSVR_NO_NET	"Kann die Netzwerkschnittstelle(n) nicht îffnen\n"

$ %1 is the luid that started the consvr process
CONSVR_LUID	"Die luid ist bereits auf %1 eingestellt\n"

$ %1 is file or program name, %2 is error string (where used)
CONSVR_RUN_SVR	"Kann DOS Server '%1' nicht in Betrieb nehmen, %2\n"
CONSVR_NO_FF	"Kann die Funktionsdatei '%1' nicht îffnen\n"
CONSVR_ERR_FF	"Fehler in Funktionsdatei '%1'\n"

$ %1, %2, %3 and %4 are the major, minor, sub-minor and special version ID
$ values, respectively
CONSVR_BANNER		"PC-Schnittstelle fÅr DOS, Version %1.%2.%3 %4\n"
CONSVR_BANNER_AADU	"DOS Server fÅr AIX, Version %1.%2\n"

$ %1 is error string
IPC_NO_MSGQ	"Kann Meldungswarteschlage nicht erstellen, %1\n"
IPC_NO_SEM	"Kann Semaphor nicht erstellen, %1\n"

MAPSVR_NO_NET	"Kann die Netzwerkschnittstelle(n) nicht îffnen\n"

DOSOUT_SEGMENT	"Kann nicht auf das gemeinsam genutzte RD-Speichersegment zugreifen\n"
DOSOUT_REXMIT	"Zu viele erneute öbertragungen\n"

$ %1 is an error string
DOSOUT_NO_SHM	"Kein gemeinsam genutzer Speicher, %1\n"
DOSOUT_S_NETA	"Kann Netzwerkattribute nicht festlegen, %1\n"
DOSOUT_PIPE_ACK	"Pipe-E/A-Fehler (ACK warten), %1\n"
DOSOUT_PIPE_CNT	"Pipe-E/A-Fehler (ACK Steuerung), %1\n"
DOSOUT_ERR6	"PTY Lesefehler 6: %1\n"
DOSOUT_ERR7	"PTY Lesefehler 7: %1\n"
DOSOUT_ERR8	"PTY Lesefehler 8: %1\n"
DOSOUT_ERR13	"PTY Lesefehler 13: %1\n"
DOSOUT_ERR14	"PTY Lesefehler 14: %1\n"
DOSOUT_ERR19	"PTY Lesefehler 19: %1\n"
DOSOUT_ERR9	"TTY Schreibfehler 9: %1\n"
DOSOUT_ERR10	"TTY Schreibfehler 10: %1\n"
DOSOUT_ERR15	"TTY Schreibfehler 15: %1\n"
DOSOUT_ERR16	"TTY Schreibfehler 16: %1\n"
DOSOUT_ERR17	"TTY Schreibfehler 17: %1\n"

$ %1 is an error string
SEMFUNC_NOSEM	"Kann keine RD-Semaphor erstellen, %1\n"
SEMFUNC_NOSHM	"Kann kein gemeinsam genutztes RD-Speichersegment erstellen, %1\n"

$ %1 is the number of objects in the caches, %2 is the size of each object
VFILE_NOMEM	"Kann fÅr %1 Objekte keinen Speicher zuordnen, jeweils %2 Byte\n"

$ %1 is the lcs error number
NLSTAB_HOST	"Kann nicht auf Host-LCS-Tabelle zugreifen, Fehler %1\n"
NLSTAB_CLIENT	"Kann nicht auf Client-LCS-Tabelle zugreifen, Fehler %1\n"
NLSTAB_SET	"Kann LCS-Tabellen nicht festlegen, Fehler %1\n"

DEBUG_CHILD	"untergeordneter Proze·"
DEBUG_OFF	"aus"
DEBUG_ON	"ein"

$ %1 is the program name
DEBUG_USAGE	"Befehlsformat: %1 <PID> [[op]KanÑle] [untergeordneter_Proze·] [on] [off]\n"

$ %1 is the program name, %2 is the faulty command line argument
DEBUG_ARG	"%1: UngÅltiges Argument: '%2'\n"

$ %1 is the program name, %2 is the channel file, %3 is an error string
DEBUG_NOFILE	"%1: Kann Kanaldatei '%2' nicht erstellen, %3\n"

$ %1 is the program name, %2 is a PID, %3 is an error string
DEBUG_INVAL	"%1: Proze· %2 kann nicht signalisiert werden, %3\n"
DEBUG_NO_SIG	"%1: Kann Proze· %2 nicht signalisieren, %3\n"

$ %1 is the program name, %2 is a channel number specified in the argument list
DEBUG_BADBIT	"%1: Kanal-Nr.%2 ist au·erhalb des Bereichs\n"
$ SCCSID("@(#)messages	7.2	LCC")	/* Modified: 1/19/93 20:08:07 */
$domain LCC.PCI.DOS.CONVERT
$quote "
$ NOTE: '\n' indicates that a new line will be printed
$ The following messages are from convert.c
CONVERT1 "Klein- und Gro·schreibung sind BEIDE angegeben\n"
CONVERT1A "Nicht kompatible Optionen angegeben\n"
CONVERT2 "In Konflikt stehende öbersetzungen angegeben\n"

$ %1 is the file name on which a read error occured
CONVERT3 "Fehler beim Lesen von %1\n"

$ %1 is the fiel name which cannot be opened
CONVERT4 "ôffnen von %1 nicht mîglich\n"

$ input file %1 and output file %2 are identical
CONVERT5 "Dateien %1 und %2 sind identisch\n"

$ an error was encountered writing file %1
CONVERT6 "Fehler beim Schreiben von %1\n"

CONVERT7 "Ausgabe-öbersetzungstabelle nicht angegeben\n"

CONVERT8 "Gro·-/Kleinschreibung ohne öbersetzung angegeben\n"

$ translation table %1 cannot be opened
CONVERT10 "ôffnen der öbersetzungstabelle %1 nicht mîglich\n"

CONVERT15 "öbersetzungstabelle ungÅltig!\n"
CONVERT17 "Eingabe der öbersetzungstabelle nicht gegeben\n"
CONVERT21 "Schreiben an die Ausgabe fehlgeschlagen!\n"
CONVERT30 "Dem öbersetzungspuffer kann kein Platz zugewiesen werden!\n"
CONVERT31 "Die öbersetzungstabellen sind nicht gesetzt!\n"

$ character %1 was untranslatable with the options used
CONVERT32 "\nUnÅbersetzbares Zeichen in Zeile # %1\n"

$ unknown error %1 occurred
CONVERT42 "Unbekannter öbersetzungsfehler:%1\n"

CONVERT45 "öbersetzungstabelle(n) nicht gefunden!\n"
CONVERT46 "Fehlerhafte öbersetzungstabelle(n)!\n"
CONVERT60 "Umgebungsvariable LAND nicht gesetzt, 1 verwenden\n"

$ code page %1 will be used as no other was specified
CONVERT61 "Umgebungsvariable CODEPAGE nicht gesetzt, %1 verwenden\n"

CONVERT77 "Ein- und AusgabeÅbersetzungstabellen nicht angegeben\n"
CONVERT80 "Warnung! 8-Bit-Zeichen\n"
CONVERT86 "Kein Speicherplatz zum Bearbeiten von CONVOPTs gefunden!\n"
CONVERT90 "Zuordnung weiteren Speicherplatzes nicht mîglich!\n"
CONVERT_B1 "Mit dem Standardzeichensatz kann \"Best Single\" nicht ausgefÅhrt werden!\n"
CONVERT_S1 "\nöbersetzungsinformation:\n"

$ %1 is the number of glyphs
CONVERT_S2 "\"Glyphs\" exakt Åbersetzt:\t\t%1\n"
CONVERT_S3 "\"Glyphs\" in mehrere Bytes Åbersetzt:\t%1\n"
CONVERT_S4 "\"Glyphs\" in Benutzerstandard Åbersetzt:\t%1\n"
CONVERT_S5 "\"Glyphs\" in \"Best Single Glyph\" Åbersetzt:\t%1\n"
CONVERT_S6 "\nGesamtanzahl bearbeiteter \"Glyphs\":\t%1\n"

$ %1 is the number of bytes, %2 is the number of lines in the text file
CONVERT_S7 "\n%1 Byte auf %2 Zeilen bearbeitet\n"

$ %1 is the name of the program
CONVERT_M1_D "Befehlsformat: %1   [/Optionen] ... [Eingabe [Ausgabe]]\n"
CONVERT_M3_D "Optionen sind: /u      Datei Gro·schreibung\n\
\              /l      Datei Kleinschreibung\n\
\              /f      Gezwungen (alte Option)\n\
\              /b      BinÑr (alte Option)\n"
CONVERT_M4_D "\                /7      Gibt Warnung aus, wenn ein Zeichen das 8. Bit verwendet.\n\
\                /x      Direkt.  Nicht Åbersetzen\n\n\
\                /i tbl  Eingabetabelle tbl Åbersetzen\n\
\                /o tbl  Ausgabetabelle tbl Åbersetzen.\n\
\n"
CONVERT_M5_D "\                /c c    c als user_char des öbersetzungsfehlers verwenden.\n\
\                /m      Mehrfache ZeichenÅbersetzungen erlauben.\n\
\                /a      Abbruch beim öbersetzungsfehler.\n\
\                /s      Die beste einfache ZeichenÅbersetzung verwenden.\n\
\                /z      STRG-Z ordnungsgemÑ· handhaben.\n\
\n"
CONVERT_M6_D "\                /d      FÅr öbersetzung von Zeilenvorschub WagenrÅcklauf durchfÅhren.\n\
\                /p      FÅr öbersetzung von WagenrÅcklauf Zeilenvorschub durchfÅhren.\n\
\n"
CONVERT_M7_D "\                /q      Anzeige von Warnmeldungen unterdrÅcken.\n\
\                /v      Warnmeldungen und öbersetzungsstatistiken anzeigen.\n\
\                /h oder /? Diese Meldung drucken.\n"


CONVERT_M1_U "Befehlsformat: %1   [-Optionen] ... [Eingabe [Ausgabe]]\n"
CONVERT_M3_U "Optionen sind: -u      Datei Gro·schreibung\n\
\                -l      Datei Kleinschreibung\n\
\                -f      Gezwungen (alte Option)\n\
\                -b      BinÑr (alte Option)\n"
CONVERT_M4_U "\                -7      Gibt Warnung aus, wenn ein Zeichen das 8. Bit verwendet.\n\
\                -x      Direkt.  Nicht Åbersetzen\n\n\
\                -i tbl  Eingabetabelle tbl Åbersetzen\n\
\                -o tbl  Ausgabetabelle tbl Åbersetzen.\n\
\n"
CONVERT_M5_U "\                /c c    c als user_char des öbersetzungsfehlers verwenden.\n\
\                /m      Mehrfache ZeichenÅbersetzungen erlauben.\n\
\                /a      Abbruch beim öbersetzungsfehler.\n\
\                /s      Die beste einfache ZeichenÅbersetzung verwenden.\n\
\                /z      STRG-Z ordnungsgemÑ· handhaben.\n\
\n"
CONVERT_M6_U "\                -d      FÅr öbersetzung von Zeilenvorschub WagenrÅcklauf durchfÅhren.\n\
\                -p      FÅr öbersetzung von WagenrÅcklauf Zeilenvorschub durchfÅhren.\n\
\n"
CONVERT_M7_U "\                -q      Anzeige von Warnmeldungen unterdrÅcken.\n\
\                -v      Warnmeldungen und öbersetzungsstatistiken anzeigen.\n\
\                -h oder -? Diese Meldung drucken.\n\
"

$ %1 is the filename, %2 is the options
CONVERT_HELP00_D "Befehlsformat: %1 [/%2] [Eingabedatei [Ausgabedatei]]\n"
CONVERT_HELP00_U "Befehlsformat: %1 [-%2] [Eingabedatei [Ausgabedatei]]\n"
CONVERT_HELP01_D "               %1 -h oder -? FÅr ausfÅhrliche Hilfe\n"
CONVERT_HELP01_U "               %1 -h oder -? FÅr ausfÅhrliche Hilfe\n"
CONVERT_HELP1_D "Befehlsformat: %1 /%2 [Eingabedatei [Ausgabedatei]]\n"
CONVERT_HELP1_U "Befehlsformat: %1 -%2 [Eingabedatei [Ausgabedatei]]\n"

$ The following messages are from getopt.c
$ Do NOT change of the order of %1 and %2!
GETOPT1 "Unbekannte Option %1%2\n"
GETOPT2 "Fehlendes Argument bei Option %1%2\n"

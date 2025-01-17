$ SCSIDM @(#)messages	1.4    LCC     ;  Modified: 18:56:55 2/14/92
$domain LCC.MERGE.DOS.MERGE
$quote "
$ The following messages are from merge.c
$ 
MERGE2 "%1: %2\n"
MERGE3 "%1: %2\n"
MERGE4 "p->Name: %1\n"
MERGE5 "p->Adresse: %1\n"
MERGE6 "p->bAbstand: %1\n"
MERGE7 "p->bGr��e: %1\n"
MERGE8 "p->Wahrheit: %1\n"
MERGE9 "p->Typ: %1\n"
MERGE10 "MERGE: %1"
MERGE11 "MERGE: Syntax:\n"
MERGE12 "    Merge-Satz <Token-Name> <Wert>\n"
MERGE13 "    merge display [<Token-Name>]\n"
MERGE14 "    Merge-R�ckkehr <Token-Name>\n"
MERGE15 "p->Kommentar: %1\n"
MERGE16	"\nG�ltige/r Token-Name/n ist/sind:\n"
$ These are error messages from merge.c
BAD_NAME "Token-Name nicht gefunden.\n"
TOOBIG "Wert zu hoch.\n"
BAD_VALUE "Der Wert mu� numerisch sein.\n"
BAD_FILE "Datei kann nicht ge�ffnet werden.\n"
USE_ONOFF "Ein|Aus verwenden.\n"
USE_LOCREM "Fern|Lokal verwenden\n"
BAD_SET "Falsche Werte in der Datei merge.set\n"
SET_MSG "Set"
ON_MSG "ein"
OFF_MSG "aus"
REMOTE_MSG "Fern"
LOCAL_MSG "Lokal"
DISPLAY_MSG "Anzeige"
RETURN_MSG "R�ckkehr"
$	SCCSID(@(#)messages	9.2	LCC)	; Modified: 15:55:17 3/28/94
$domain LCC.PCI.DOS.ON

$quote "
$ The following messages are from on.c
$ NOTE: on.c is also the source for jobs and kill

$ this is the legend bar for the jobs command
ON1 "JOB   HOST        MODUS   ENDE STATUS    BEFEHL\n"

$ this is the display format for the jobs command
$ if you change the order here be sure to change the
$ legend bar, ON1.
$ The values here are:
$	%1 job number
$	%2 host drive
$	%3 host name
$	%4 job state (running, sleeping, killed, etc.)
$	%5 exit status as text
$	%6 command
$ NOTE: it is unlikely that this format will need to be changed
ON2 "[%1]  %2 %3  %4 %5  %6\n"

$ %1 is the name of the current command running
ON3 "%1\n"

$ this is for detached processes. %1 is the job index.
ON4 "[%1] freigegeben\n"

$ this message is used if a job cannot be run, %1 is the program
$ name (on, jobs, kill) and %2 is the command give.
ON5 "%1: %2: Zugriff verweigert oder Datei nicht gefunden\n"

$ this is the first part of an error message, %1 is the program name
ON6 "%1: "

$ %1 is the program name
ON7 "%1: Kein ferner Proze� in der Auftragstabelle"


ON8 "Zul�ssige Signale:\n\t"

$ this is the format for the output of each allowable signal,
$ %1 is the signal name
ON9 "%1, "

ON12 "%1: Abbrechen (Kill) von %2 nicht m�glich\n"

$ these two messages define the user's options
$ abort check defines which characters to match
$ for each of the three options both in upper
$ and lower case
ON13 "a - Abbrechen, w - Weitermachen, f - Freigeben: "
$ NOTE: ABORT_CHECK must have both upper and lower for each option,
$ immediately following each other or the program will behave
$ unexpectedly
ABORT_CHECK	 "AaCcDd"

ON16 "BRIDGE.DRV nicht installiert oder inkompatible Version"
ON18 "Inkompatible Version von BRIDGE.DRV"

$ %1 is either the drive letter, the hostname or a '-'
ON19 "<%1> ist kein angeschlossener Host"

$ drive %1 is not a virtual drive 
ON20 "<%1:> ist kein virtuelles Laufwerk"

$ program %1 received a bad numeric argument %2
ON21 "%1: Falsches numerisches Argument (%2)\n"

$ %1 is the name of the jobs program
ON22 "Befehlsformat: %1 [%%job oder -]\n"

$ %1 is the name of the jobs program
ON23 "%1: Auftrag nicht gefunden\n"

ON24 "oder ein numerisches Argument.\n"

$ %1 is the name of the kill program
ON25 "%1: Auftrags- oder Proze�nummer angeben.\n"

$ %1 is the name of the jobs command, %2 is the job number or id given
ON26 "%1: Auftrag %2 nicht gefunden\n"

$ %1 is the program name, %2 is an error output, one of the ERROR
ON27 "%1\n"

$ %1 is the number of the unknown error
ON28 "Unbekannter Fehler bei uexec (%1)\n"

$ %1 is the name of the on program
ON29 "Befehslformat: %1 [-Signal] %Auftragsnummer\n"

$ %1 is the name of the program
ON30 "%1: Auftragstabelle voll\n"

ON31 "Inkompatible DOS Version."

$ %1 is the name of the program
ON32 "Befehlsformat: %1 <Host-Befehl>\n"
ON33 "Befehlsformat: %1 <Host> <Befehl>\n"

$ %1 is the name of the program %2 is the lcs_errno encountered in the
$ failed translation
ON34 "%1: �bersetzungsfehler %2.\n"

ON35 "LMF-Datei Datei \"ABORT_CHECK\" falsch angegeben\n"
ON36 "Tempor�re Datei %1 kann nicht angelegt werden\n"

$ shared usage message
ONUSAGE "       %1 /h oder /? druckt diese Meldung\n"

$ internal variables that need to be patched for on.c
$ NOTE: these names were placed here by hand!

$ these are the possible named signals
SIGNAME0	"abbrechen"
SIGNAME1	"Term"
SIGNAME2	"Usr1"
SIGNAME3	"Usr2"

$ these are the possible jobstates
JOBSTATES0	"Unbekannt"
JOBSTATES1	"L�uft"
JOBSTATES2	"Abgeschlossen"
	
$ these are the possible exit statuses
EXTYPES0    	"beenden"
EXTYPES1	"Signal"
EXTYPES2	"coredump"
EXTYPES3	"Fehler3"
EXTYPES4	"unbekannt"

$ these are the failure error messages
ERRVEC0		"Fehler im Netzwerkdienst."
ERRVEC1		"Nicht an einen Host angeschlossen."
ERRVEC2		"Hostausf�hrung fehlgeschlagen."
ERRVEC3		"Ung�ltiges Format."
ERRVEC4		"Fehler bei DOS Speicherzuweisung."
$ SCCSID(@(#)messages	7.7 changed 1/28/92 19:37:36)
$domain LCC.PCI.DOS.LOGOUT
$quote "

$ The following messages are from logout.c
LOGOUT1 "Alle virtuellen Treiber wurden abgemeldet.\n"

$ This is the virtual drive identifier of the host
LOGOUT2 "%1: "

$ This is the name of the host
LOGOUT3 " (%1)"

$ This indicates that the host to be logged out was not logged in
LOGOUT4 " nicht angemeldet\n"

$ The host has been logged out
LOGOUT5 " abgemeldet\n"

$ This is the usage message for the logout program
LOGOUT6 "\
Befehlsformat:\n\
      %1            - alle Hosts\n\
  oder  %2 <Hostname> - genannter Host\n\
  oder  %3 <Laufwerk>:   - Laufwerk zugewiesener Host\n\
  or  %4 /h oder /?   - diese Meldung drucken\n\
"

$ The logout program has run out of memory
LOGOUT7 "kein Speicherplatz mehr"

$domain LCC.PCI.DOS.PCICONF
$ The following messages are from pciconf.c

$ This message is used to print an error message from pciconf
PCICONF1 "PCICONF: %1"

$ This is the usage message 
PCICONF2 " PCICONF: Syntax:\n\
    pciconf set Token-Name [on|off][remote|local]\n\
    pciconf display [Token-Name]\n\
    pciconf return Token-Namen\n\
    pciconf /h oder /?  druckt diese Meldung\n"

$ These messages are the commands that may be given as arguments to pciconf
PCICONF3 "Set"
PCICONF4 "Anzeige"
PCICONF5 "R�ckkehr"

$ These messages are the names of values the options can have
VAL_ON     "ein"
VAL_OFF    "aus"
VAL_LOCAL  "Lokal"
VAL_REMOTE "Fern"

$ These messages are the token-names
$ (they will be truncated to 14 characters)
O_NETBIOS "Netbios"
O_NETDR   "vLaufwerk"
O_5F02    "5f02"
O_LPTBIN  "lpt_binary"

$ This messages precedes the list of possible token names 
PCICONF6 "\nM�gliche Token-Namen sind:\n"

$ This message formats the possible tokens
PCICONF7 "\t%1\n"

$ This tells the user that he has selected an invalid option (on/off are valid)
PCICONF8 "Ein|Aus verwenden.\n"

$ This tells the user that he has selected an invalid option 
$ (remote/local are valid)
PCICONF9 "Fern|Lokal verwenden\n"
PCICONF10 "Speicher oder Programmdatei defekt\n"
PCICONF12 "Token-Name unbekannt.\n"

$ these are debug messages only
PCICONF15 "Debug ein\n"
PCICONF16 "EINSTELLEN\n"
PCICONF17 "<NAME> ANZEIGEN\n"
PCICONF18 "ZUR�CKGEBEN\n"
PCICONF19 "Fehler: Ung�ltige Aktion: %1\n"

$ These are used to list the values of local/remote drive and nbs,nd on/off 
PCICONF22 "%1: %2\n"
PCICONF23 "ein"
PCICONF24 "aus"
PCICONF25 "Fern"
PCICONF26 "Lokal"

$domain LCC.PCI.DOS.PCIINIT
$ The following messages are from pciinit.c
PCIINIT1 "Richtiges BRIDGE-Ger�t nicht gefunden\n"

$ The following shows an initialization error and the associated errno
$ PCIINIT2 is a discontinued message
PCIINIT2 "Fehler bei der Initialisierung von $ %1\n"

PCIINIT_BAD_DOS     "Fehler: Keine Initialisierung, DOS nicht erkannt\n"
PCIINIT_BAD_BRIDGE  "Fehler: Keine Initialisierung, BRIDGE.DRV FEHLERHAFT\n"
PCIINIT_DRIVER_VER  "Warnung: Falsche Netzwerktreiber-Version\n"
PCIINIT_DRIVER_INIT "Warnung: Initialisierung des Netzwerktreibers fehlgeschlagen\n"
PCIINIT_NO_NET \\x13
      "Fehler: Keine Initialisierung, keine Netzwerkschnittstelle oder RS232-Ports\n"

PCIINIT_FOR_NET   "Zur Verwendung �ber Netzwerkschnittstelle\n"
PCIINIT_FOR_RS232 "Zur Verwendung �ber RS232-Ports\n"
PCIINIT_FOR_BOTH  "Zur Verwendung �ber Netzwerkschnittstelle und RS232-Ports\n"

PCIINIT_DRIVES    "Die Laufwerke der PC-Schnittstelle sind %1 bis %2\n"
PCIINIT_NETWARE_DRIVES "Das erste NetWare_Laufwerk ist %1\n"

PCIINIT3 "Warnung: Maschinenname kann nicht gesetzt werden\n"

$ An improperly formed (one without a leading '-' or '/' ) was given.
PCIINIT4 "Falsches Argument <%1>\n"

$ An invalid option was given
PCIINIT5 "Unbekannte Option <%1>\n"

$ The broadcast address given is not in the proper format 
PCIINIT6 "Ung�ltige Rundspruchadresse <%1>\n"

$ there was no 'localhost' internet address found for the PC
PCIINIT7 "'%1' Eintrag nicht gefunden\n"

$ the localhost address is not in the proper format or invalid
PCIINIT8 "Ung�ltige lokale Adresse <%1>\n"

$  The address does not conform to the internet Class A, B or C values
PCIINIT9 "Falsche Adre�klasse \"%1\"\n"

$ Both the EXCELAN and the HOSTS environment variables must agree
PCIINIT10 "\"HOSTS\" und \"EXCELAN\\tcp\" m�ssen �bereinstimmen\n"

PCIINIT11 "�ffnen von %1 unm�glich\n"
PCIINIT12 "Bridge-Version mu� gr��er als 2.7.2 sein\n"

$ The next two messages refer to the HOSTS file
PCIINIT13 "�ffnen von Datei %1 nicht m�glich.\n"
PCIINIT14 "%1 ist defekt oder ung�ltig.\n"


PCIINIT15 "Fernes Laufwerk: %1\n"
PCIINIT16 "Release %1.%2.%3 (Serien-#%4.%5) initialisiert\n"

$ this prints the ip address
PCIINIT17 "%1"
PCIINIT18 "IP-Adresse = "
PCIINIT19 "\nRundspruchadresse = "

$ This is the program name
PCIINIT21 "pciinit: "

$ This is the product name
PCIINIT24 "PC-Schnittstelle "

$ This is printed if pciinit has already been run
PCIINIT25 "bereits initialisiert"

$ This warns the user to run PCIINIT without the /e option to initialize the
$ software.
PCIINIT26 "Warnung: PC-I wurde nicht initialisiert.\n\
PCIINIT ohne Option /e ausf�hren, um PC-I zu initialisieren.\n"

$ The subnet mask is not in the proper format 
PCIINIT27 "Ung�ltige Subnetmask<%1>\n"

$ The subnet mask is not in the proper format 
PCIINIT28 "Ung�ltige Subnetmask<%1.%2.%3.%4>\n"

$ Netware is already installed
PCIINIT30 "Initialisierung nach Initialisierung von Netware nicht m�glich\n"

$ usage message for pciinit
PCIINIT_U "\
Befehlsformat: pciinit [/i<Host>] [/b<Host>] [/s<Maske>] [<serielles-Verzeichnis>]\n\
       wobei <Host>  ein Name oder eine IP-Adresse ist\n\
       wobei <Maske> ein Name oder eine Unternetmask ist\n\
       pciinit /e          druckt die aktuellen Einstellungen der Netzwerkadresse\n\
       pciinit /h oder /?  druckt diese Meldung\n"

$ for bridge versions prior to release 3.2
PCIINIT_OLD_BRIDGE "Warnung: Alte Version von bridge.drv\n"

$ This is an error found in the bridge
ERROR_MSG "Fehler - "

$ This is used to indicate that a particular host did not respond
GEN_ERROR" hat nicht geantwortet. Wiederholen? (J oder N) : "


DISC_MSG "PC-Schnittstellensitzung beendet.  Anmeldung wiederholen."
ERR_MSG	"PARAMETER-FEHLER!\r\n$"
DRV_MSG	 " virtuelle Laufwerke und $"
JOBS_MSG " Auftragstabelleneintr�ge\r\n\$"

$ The copy protection violation message
VIOLATION "Kopierschutz der PC-Schnittstelle verletzt - SYSTEM DEAKTIVIERT"


$domain LCC.PCI.DOS.PRINTER
$ The following messages are from printer.c.
$
$ These tokens are the words that the user types to select
$ remote/local printer operation.
PCI_REMOTE_TOKEN "Fern"
PCI_LOCAL_TOKEN "Lokal"
MRG_REMOTE_TOKEN "Unix"
MRG_LOCAL_TOKEN "DOS"
AIX_REMOTE_TOKEN "Aix"

$ the DOS call to set the printer state failed
PRINTER1 "Drucker: Einstellung des Zustands nicht m�glich.\n"

$ the printer name given is not in the range allowed or is non-numeric
PRINTER2 "%1: Falscher Name f�r Druckdatenstrom.\n"

$ This message says that you can't reset the printer program
$ when you are just setting it
PRINTER3 "Beim Einstellen des Druckerprogramms ist %1 ung�ltig\n"


PRINTER4 "Konflikt zwischen den Optionen /P und /D\n"

$ The following 5 thru 8 are the usage message for the multistream
$ version of PC-Interface, and the Merge version.
$ The PCI version uses PRINTER7 instead of PRINTER7_M.
$ The Merge version uses PRINTER7_M instead of PRINTER7.
PRINTER5 "Befehlsformat:\nDrucker\n"
$ %1 is the 'local' token, which is "local" in PCI and "dos" in Merge.
PRINTER6 "printer [LPTn] %1\n"
PRINTER7 "printer [LPTn] {Host|Laufwerk|-} [Druckprogramm|/R] [/X[0|1]]\n        [/T[Zeit�berschreitung]]\n"
$ %1 is the 'remote' token, for Merge is "unix" (or "aix", on an AIX machine).
PRINTER7_M "printer [LPTn] %1 [Druckprogramm|/R] [/X[0|1]] [/T[Zeit�berschreitung]]\n"
PRINTER8 "printer [LPTn] [/P|/D] [/X[0|1]] [/T[Zeit�berschreitung]]\n"
PRINTER_HELP "Drucker /h oder /? druckt diese Meldung\n"

$ The following 9 and 10 are the usage message for the non-multistream
$ version of PC-Interface
PRINTER9 "Befehlsformat: Drucker {%1}\n"
PRINTER10 "               {%1} [/Tn]\n"

PRINTER11 "Einstellung der Druckerparamter (LPT%1) nicht m�glich\n"
PRINTER12 "Einstellung des fernen LPT%1 nicht m�glich.  Wahrscheinlich sind Sie nicht angemeldet.\n"
PRINTER13 "Ihre Bridge-Version unterst�tzt nur LPT1.\n"
PRINTER14 "Drucker: Einstellung des Zustands nicht m�glich.\n"
PRINTER15 "Drucker: Status nicht erhalten.\n"
PRINTER16 "Die Option %1 wird von Ihrer Bridge-Version nicht unterst�tzt\n"
PRINTER22 "LPT%1: "


$ This message prints out which drive is associated with which host
PRINTER23 " (%1: %2)"

$ This message prints out the drive of the host for systems that support
$ only one host
PRINTER24 " (%1:)"

$ the following messages, PRINTER25 through PRINTER33 are used
$ to create an informational display for each printer
$ the format of this message is
$ 	LPT#: [local|remote] , print on exit [on|off], 
$	[no timeout | timeout = # ], [default printer | <printprog>]
PRINTER25 ", Drucken beim Beenden "
PRINTER26 "ein"
PRINTER27 "aus"
PRINTER28 ", Zeit�berschreitung = %1"
PRINTER29 ", keine Zeit�berschreitung"
PRINTER30 ",\n      "
PRINTER31 ", "
PRINTER32 "\"%1\"\n"
PRINTER33 ", Standarddrucker\n"

$ this says whether the printer's state is local or remote
PRINTER34 "Aktueller Druckerstatus: %1"

PRINTER35 " mit  %1 zweiter Zeit�berschreitung."
PRINTER36 " ohne Zeit�berschreitung."

$ This message indicates that a particular drive is not connected
PRINTER37 "%1: ist nicht angeschlossen"

$ This message is self-explanatory.
PRINTER38 "kein Speicherplatz mehr\n"

$domain LCC.PCI.DOS.UDIR
$ The following messages are from udir.c

$ %1 is drive number
UDIR1 " Datentr�ger in Laufwerk %1 "
UDIR2 "hat kein Label\n"

$ %1 is the drive label 
UDIR3 "ist %1\n"

$ %1 is the path name
UDIR4 " Verzeichnis von %1\n"

$ This is the format of the udir output line
$ %1 is the UNIX side name, %2 is the mapped DOS file name,
$ %3 is the owner's name, %4 is the file attributes
UDIR5 "%1 %2%3%4"

$ This is the message that the file is a directory
UDIR7 "<DIR>    "

$ This completes the output line with date (%1) followed by time (%2)
UDIR9 "%1%2\n"

UDIR14 "\nDatei nicht gefunden\n"

$ %1 is the number of files found, %2 is for spacing, %3 is the number
$ of bytes found
UDIR15 "\t%1 Datei(en)%2%3 Byte frei\n"

$ the drive given is invalid
UDIR17 "Ung�ltige Laufwerksbezeichnung\n"

$ udir usage message
UDIR18 "\
Befehlsformat: udir [/a] [Laufwerkbuchstabe:][Pfad][Verzeichnis|Dateiname]\n\
               udir /h oder /?    - druckt diese Meldung\n"


$ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
$ ! 	NOTE - convert.c and getopt.c messages are		!
$ !	 stored on the UNIX side in util			!
$ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

$domain LCC.PCI.DOS.DOSWHAT

DOSWHAT2 "Befehlsformat: %1 [/f] [/w] Datei [Datei] ...\n\
               %1 /h oder /? druckt diese Meldung\n"

$ %1 is the file name
DOSWHAT3 "%1: %2 nicht gefunden\n"
DOSWHAT5 "%1: \n\tVerzeichnis\n"
DOSWHAT7 "%1:\n\tGer�t\n"
DOSWHAT8 "%1: �ffnen von %2 nicht m�glich\n"
DOSWHAT9 "%1: fstat bei %2 nicht m�glich\n"
DOSWHAT11 "%1:\n\tGer�t\n"

DOSWHAT14 "Dateityp nicht festgelegt: Versuchen Sie es mit dem UNIX-Dateibefehl.\n"

DOSTYPE0 " Bin�re Datei\n"
DOSTYPE1 " DOS Textdatei im ASCII-Format\n"
DOSTYPE2 " UNIX-Textdatei im ASCII-Format\n"
DOSTYPE3 " Verzeichnis\n"
DOSTYPE4 " Ger�t\n"

$domain LCC.PCI.DOS.PCIDEBUG
$quote "
$ The following messages are from pcidebug.c
$ %1 is the program name - pcidebug
PCIDEBUG1 "%1: Befehlsformat: %1 <Host|Laufwerk> <[=+-~]Kanalliste|ein|aus|schlie�en> [...]\n\t\t\t\t\tKanalliste = Kanalnummer1[,Kanalnummer2[...]]\n\
\t\t %1 /h oder /?  druckt diese Meldung\n"

$ %1 is the program name, %2 is the argument
PCIDEBUG2 "%1: Ung�ltiges Argument: \"%2\"\n"

$ %1 is the program name, %s is the bit value
PCIDEBUG3 "%1: Bit %2 au�erhalb des Wertebereichs\n"

$ %1 is the program name, %2 is the ioctl value
PCIDEBUG4 "%1: Fehler in ioctl %2\n"

$ the next three are tokens 
$ if they are changed, change them in PCIDEBUG1 to agree
PCIDEBUG10 "aus"
PCIDEBUG11 "ein"
PCIDEBUG12 "Schlie�en"


$ The following messages are from vdrive.c
$domain	LCC.PCI.DOS.NLSVD
$quote "

USEAGENODRIVE "Momentan sind keine virtuellen PCI-Laufwerke angemeldet.\n"
NODRIVE "Momentan sind keine virtuellen PCI-Laufwerke angemeldet.\n"
BADDRIVE "Ung�ltige Laufwerksangabe - %1\n"
USEAGEBADDRIVE "Ung�ltige Laufwerksangabe - %1\n"
USEAGENOTADRIVE "Laufwerk %1 ist kein virtuelles Laufwerk.\n"
FATAL "Schwerwiegender Fehler beim Aufruf des Hostnamens.\n"
CONNECT "%1: ist an %2 angeschlossen.\n"

$ SCCSID("@(#)messages	6.1	LCC")	/* Modified: 10/15/90 15:48:13 */
$domain LCC.PCI.DOS.CONVERT
$quote "
$ NOTE: '\n' indicates that a new line will be printed
$ The following messages are from convert.c
CONVERT1 "Klein- und Gro�schreibung sind BEIDE angegeben\n"
CONVERT1A "Nicht kompatible Optionen angegeben\n"
CONVERT2 "unix2dos und dos2unix sind BEIDE angegeben\n"

$ %1 is the file name on which a read error occured
CONVERT3 "Fehler beim Lesen von %1\n"

$ %1 is the fiel name which cannot be opened
CONVERT4 "�ffnen von %1 nicht m�glich\n"

$ input file %1 and output file %2 are identical
CONVERT5 "Dateien %1 und %2 sind identisch\n"

$ an error was encountered writing file %1
CONVERT6 "Fehler beim Schreiben von %1\n"

CONVERT7 "Ausgabe-�bersetzungstabelle nicht angegeben\n"

CONVERT8 "Gro�-/Kleinschreibung ohne �bersetzung angegeben\n"

$ translation table %1 cannot be opened
CONVERT10 "�ffnen der �bersetzungstabelle %1 nicht m�glich\n"

CONVERT15 "�bersetzungstabelle ung�ltig!\n"
CONVERT17 "Eingabe der �bersetzungstabelle nicht gegeben\n"
CONVERT21 "Schreiben an die Ausgabe fehlgeschlagen!\n"
CONVERT30 "Dem �bersetzungspuffer kann kein Platz zugewiesen werden!\n"
CONVERT31 "Die �bersetzungstabellen sind nicht gesetzt!\n"

$ character %1 was untranslatable with the options used
CONVERT32 "\nUn�bersetzbares Zeichen in Zeile # %1\n"

$ unknown error %1 occurred
CONVERT42 "Unbekannter �bersetzungsfehler:%1\n"

CONVERT45 "�bersetzungstabelle(n) nicht gefunden!\n"
CONVERT46 "Fehlerhafte �bersetzungstabelle(n)!\n"
CONVERT60 "Umgebungsvariable LAND nicht gesetzt, 1 verwenden\n"

$ code page %1 will be used as no other was specified
CONVERT61 "Umgebungsvariable CODEPAGE nicht gesetzt, %1 verwenden\n"

CONVERT77 "Ein- und Ausgabe�bersetzungstabellen nicht angegeben\n"
CONVERT80 "Warnung! 8-Bit-Zeichen\n"
CONVERT86 "Kein Speicherplatz zum Bearbeiten von CONVOPTs gefunden!\n"
CONVERT90 "Zuordnung weiteren Speicherplatzes nicht m�glich!\n"
CONVERT_B1 "Mit dem Standardzeichensatz kann \"Best Single\" nicht ausgef�hrt werden!\n"
CONVERT_S1 "\n�bersetzungsinformation:\n"

$ %1 is the number of glyphs
CONVERT_S2 "\"Glyphs\" exakt �bersetzt:\t\t%1\n"
CONVERT_S3 "\"Glyphs\" in mehrere Bytes �bersetzt:\t%1\n"
CONVERT_S4 "\"Glyphs\" in Benutzerstandard �bersetzt:\t%1\n"
CONVERT_S5 "\"Glyphs\" in \"Best Single Glyph\" �bersetzt:\t%1\n"
CONVERT_S6 "\nGesamtanzahl bearbeiteter \"Glyphs\":\t%1\n"

$ %1 is the number of bytes, %2 is the number of lines in the text file
CONVERT_S7 "\n%1 Byte auf %2 Zeilen bearbeitet\n"

$ %1 is the name of the program
CONVERT_M1_D "Befehlsformat: %1   [/Optionen] ... [Eingabe [Ausgabe]]\n"
CONVERT_M3_D "Optionen sind: /u      Datei Gro�schreibung\n\
\              /l      Datei Kleinschreibung\n\
\              /f      Gezwungen (alte Option)\n\
\              /b      Bin�r (alte Option)\n"
CONVERT_M4_D "\                /7      Gibt Warnung aus, wenn ein Zeichen das 8. Bit verwendet.\n\
\                /x      Direkt.  Nicht �bersetzen\n\n\
\                /i tbl  Eingabetabelle tbl �bersetzen\n\
\                /o tbl  Ausgabetabelle tbl �bersetzen.\n\
\n"
CONVERT_M5_D "\                /c c    c als Benutzerzeichen f�r fehlgeschlagene �bersetzung verwenden.\n\
\                /m      Mehrfachzeichen�bersetzungen verwenden.\n\
\                /a      Bei fehlgeschlagener �bersetzung abbrechen.\n\
\                /s      Beste Einzelzeichen�bersetzung verwenden.\n\
\                /z      C-Z richtig handhaben (dos2unix/unix2dos).\n\
\n"
CONVERT_M6_D "\                /d      F�r �bersetzung von Zeilenvorschub Wagenr�cklauf durchf�hren.\n\
\                /p      F�r �bersetzung von Wagenr�cklauf Zeilenvorschub durchf�hren.\n\
\n"
CONVERT_M7_D "\                /q      Anzeige von Warnmeldungen unterdr�cken.\n\
\                /v      Warnmeldungen und �bersetzungsstatistiken anzeigen.\n\
\                /h oder /? Diese Meldung drucken.\n\
"


CONVERT_M1_U "Befehlsformat: %1   [-Optionen] ... [Eingabe [Ausgabe]]\n"
CONVERT_M3_U "Optionen sind: -u      Datei Gro�schreibung\n\
\              -l      Datei Kleinschreibung\n\
\              -f      Gezwungen (alte Option)\n\
\              -b      Bin�r (alte Option)\n"
CONVERT_M4_U "\                -7      Gibt Warnung aus, wenn ein Zeichen das 8. Bit verwendet.\n\
\                -x      Direkt.  Nicht �bersetzen\n\n\
\                -i tbl  Eingabetabelle tbl �bersetzen\n\
\                -o tbl  Ausgabetabelle tbl �bersetzen.\n\
\n"
CONVERT_M5_U "\                -c c    c als Benutzerzeichen f�r fehlgeschlagene �bersetzung verwenden.\n\
\                -m      Mehrfachzeichen�bersetzungen verwenden.\n\
\                -a      Bei fehlgeschlagener �bersetzung abbrechen.\n\
\                -s      Beste Einzelzeichen�bersetzung verwenden.\n\
\                -z      C-Z richtig handhaben (dos2unix/unix2dos).\n\
\n"
CONVERT_M6_U "\                -d      F�r �bersetzung von Zeilenvorschub Wagenr�cklauf durchf�hren.\n\
\                -p      F�r �bersetzung von Wagenr�cklauf Zeilenvorschub durchf�hren.\n\
\n"
CONVERT_M7_U "\                -q      Anzeige von Warnmeldungen unterdr�cken.\n\
\                -v      Warnmeldungen und �bersetzungsstatistiken anzeigen.\n\
\                -h oder -? Diese Meldung drucken.\n\
"

$ %1 is the filename, %2 is the options
CONVERT_HELP00_D "Befehlsformat: %1 [/%2] [Eingabedatei [Ausgabedatei]]\n"
CONVERT_HELP00_U "Befehlsformat: %1 [-%2] [Eingabedatei [Ausgabedatei]]\n"
CONVERT_HELP01_D "       %1 -h oder -? F�r ausf�hrliche Hilfe\n"
CONVERT_HELP01_U "       %1 -h oder -? F�r ausf�hrliche Hilfe\n"
CONVERT_HELP1_D "Befehlsformat: %1 /%2 [Eingabedatei [Ausgabedatei]]\n"
CONVERT_HELP1_U "Befehlsformat: %1 -%2 [Eingabedatei [Ausgabedatei]]\n"

$ The following messages are from getopt.c
$ Do NOT change of the order of %1 and %2!
GETOPT1 "Unbekannte Option %1%2\n"
GETOPT2 "Fehlendes Argument bei Option %1%2\n"
$ SCCSID("@(#)messages	5.1	LCC")	/* Modified: 6/3/91 13:46:54 */
$quote "
$domain LCC.PCI.DOS.HOSTDRV
$ The following messages are from hostdrv.c
$ %1 is the drive letter given
HOSTDRV1 "%1: ist kein virtuelles Laufwerk"
HOSTDRV2 "%1 ist kein angeschlossener Host"

$domain LCC.PCI.DOS.HOSTOPTN
$ The following messages are from hostoptn.c
HOSTOPTN1 "Ung�ltige Host-Auswahloption(en)"

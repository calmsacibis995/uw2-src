$ SCSIDM @(#)messages	1.4    LCC     ;  Modified: 18:56:55 2/14/92
$domain LCC.MERGE.DOS.MERGE
$quote "
$ The following messages are from merge.c
$ 
MERGE2 "%1 : %2\n"
MERGE3 "%1 : %2\n"
MERGE4 "p->name : %1\n"
MERGE5 "p->addr : %1\n"
MERGE6 "p->boffset : %1\n"
MERGE7 "p->bsize : %1\n"
MERGE8 "p->truth : %1\n"
MERGE9 "p->type : %1\n"
MERGE10 "MERGE : %1"
MERGE11 "MERGE : syntaxe :\n"
MERGE12 "    merge set <nom-jeton> <valeur>\n"
MERGE13 "    merge display [<nom-jeton>]\n"
MERGE14 "    merge return <nom-jeton>\n"
MERGE15 "p->comment : %1\n"
MERGE16	"\nles noms-jetons corrects sont :\n"
$ These are error messages from merge.c
BAD_NAME "impossible de trouver nom-jeton.\n"
TOOBIG "valeur trop grande.\n"
BAD_VALUE "vous devez entrer une valeur num�rique.\n"
BAD_FILE "impossible d'ouvrir le fichier. \n"
USE_ONOFF "vous devez utiliser on|off.\n"
USE_LOCREM "vous devez utiliser remote|local\n"
BAD_SET "valeurs erron�es dans le fichier merge.set\n"
SET_MSG "set"
ON_MSG "activ�"
OFF_MSG "off"
REMOTE_MSG "distant"
LOCAL_MSG "local"
DISPLAY_MSG "afficher"
RETURN_MSG "return"
$	SCCSID(@(#)messages	9.2	LCC)	; Modified: 15:55:17 3/28/94
$domain LCC.PCI.DOS.ON

$quote "
$ The following messages are from on.c
$ NOTE: on.c is also the source for jobs and kill

$ this is the legend bar for the jobs command
ON1 "TACHE HOTE        ETAT QUITTER STATUT    COMMANDE\n"

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
ON4 "[%1] dissoci�\n"

$ this message is used if a job cannot be run, %1 is the program
$ name (on, jobs, kill) and %2 is the command give.
ON5 "%1 : %2 : acc�s refus� ou fichier introuvable\n"

$ this is the first part of an error message, %1 is the program name
ON6 "%1: "

$ %1 is the program name
ON7 "%1 : aucun processus � distance dans la table des t�ches."


ON8 "Signaux autoris�s :\n\t"

$ this is the format for the output of each allowable signal,
$ %1 is the signal name
ON9 "%1, "

ON12 "%1 : impossible de supprimer %2\n"

$ these two messages define the user's options
$ abort check defines which characters to match
$ for each of the three options both in upper
$ and lower case
ON13 "a - abandonner, c - continuer, d - dissocier : "
$ NOTE: ABORT_CHECK must have both upper and lower for each option,
$ immediately following each other or the program will behave
$ unexpectedly
ABORT_CHECK	 "AaCcDd"

ON16 "BRIDGE.DRV non install� ou version incompatible"
ON18 "version BRIDGE.DRV incompatible"

$ %1 is either the drive letter, the hostname or a '-'
ON19 "l'h�te <%1> n'est pas connect�"

$ drive %1 is not a virtual drive 
ON20 "<%1:> n'est pas une unit� virtuelle"

$ program %1 received a bad numeric argument %2
ON21 "%1 : mauvais argument num�rique (%2)\n"

$ %1 is the name of the jobs program
ON22 "Syntaxe : %1 [%%job ou -]\n"

$ %1 is the name of the jobs program
ON23 "%1 : t�che introuvable"

ON24 "ou un argument num�rique.\n"

$ %1 is the name of the kill program
ON25 "%1 : vous devez indiquer l'id t�che ou processus.\n"

$ %1 is the name of the jobs command, %2 is the job number or id given
ON26 "%1 : t�che %2 introuvable\n"

$ %1 is the program name, %2 is an error output, one of the ERROR
ON27 "%1\n"

$ %1 is the number of the unknown error
ON28 "Erreur (%1) uexec inconnue\n"

$ %1 is the name of the on program
ON29 "Syntaxe : %1 [-signal] %idt�che\n"

$ %1 is the name of the program
ON30 "%1 : table des t�ches satur�e\n"

ON31 "Version DOS incompatible."

$ %1 is the name of the program
ON32 "Syntaxe : %1 <commande h�te>\n"
ON33 "Syntaxe : %1 <h�te> <commande>\n"

$ %1 is the name of the program %2 is the lcs_errno encountered in the
$ failed translation
ON34 "%1 : erreur de traduction %2.\n"

ON35 "Sp�cification du fichier LMF \"ABORT_CHECK\" incorrecte\n"
ON36 "Impossible de cr�er le fichier temporaire %1\n"

$ shared usage message
ONUSAGE "       %1 /h ou /? imprime ce message\n"

$ internal variables that need to be patched for on.c
$ NOTE: these names were placed here by hand!

$ these are the possible named signals
SIGNAME0	"d�truire"
SIGNAME1	"terme"
SIGNAME2	"usr1"
SIGNAME3	"usr2"

$ these are the possible jobstates
JOBSTATES0	"Inconnu"
JOBSTATES1	"Ex�cution"
JOBSTATES2	"Termin�"
	
$ these are the possible exit statuses
EXTYPES0    	"quitter"
EXTYPES1	"signal"
EXTYPES2	"vidage m�moire"
EXTYPES3	"err3"
EXTYPES4	"inconnu"

$ these are the failure error messages
ERRVEC0		"Erreur au niveau du service r�seau."
ERRVEC1		"Pas de connexion � un h�te."
ERRVEC2		"L'ex�cution de l'h�te a �chou�."
ERRVEC3		"Format incorrect."
ERRVEC4		"Erreur d'affectation de m�moire DOS."
$ SCCSID(@(#)messages	7.7 changed 1/28/92 19:37:36)
$domain LCC.PCI.DOS.LOGOUT
$quote "

$ The following messages are from logout.c
LOGOUT1 "Fermeture de session pour toutes les unit�s virtuelles.\n"

$ This is the virtual drive identifier of the host
LOGOUT2 "%1: "

$ This is the name of the host
LOGOUT3 " (%1)"

$ This indicates that the host to be logged out was not logged in
LOGOUT4 " ouverture de session non effectu�e\n"

$ The host has been logged out
LOGOUT5 " fermeture de session effectu�e\n"

$ This is the usage message for the logout program
LOGOUT6 "\
Syntaxe : \n\
      %1            - tous les h�tes\n\
  ou  %2 <nom_h�te> - h�te d�sign�\n\
  ou  %3 <unit�> :   - h�te attach� � l'unit�\n\
  ou  %4 /h ou /?   - imprime ce message\n\
"

$ The logout program has run out of memory
LOGOUT7 "m�moire insuffisante"

$domain LCC.PCI.DOS.PCICONF
$ The following messages are from pciconf.c

$ This message is used to print an error message from pciconf
PCICONF1 "PCICONF : %1"

$ This is the usage message 
PCICONF2 "PCICONF : syntaxe :\n\
    pciconf set nom_de_jeton [on|off][remote|local]\n\
    pciconf display [nom_jeton]\n\
    pciconf return nom_de_jeton\n\
    pciconf /h ou /?  affiche ce message\n"

$ These messages are the commands that may be given as arguments to pciconf
PCICONF3 "set"
PCICONF4 "afficher"
PCICONF5 "return"

$ These messages are the names of values the options can have
VAL_ON     "activ�"
VAL_OFF    "off"
VAL_LOCAL  "local"
VAL_REMOTE "distant"

$ These messages are the token-names
$ (they will be truncated to 14 characters)
O_NETBIOS "netbios"
O_NETDR   "vdrive"
O_5F02    "5f02"
O_LPTBIN  "Ipt_binary"

$ This messages precedes the list of possible token names 
PCICONF6 "\nNoms-jetons possibles :\n"

$ This message formats the possible tokens
PCICONF7 "\t%1\n"

$ This tells the user that he has selected an invalid option (on/off are valid)
PCICONF8 "vous devez utiliser on|off.\n"

$ This tells the user that he has selected an invalid option 
$ (remote/local are valid)
PCICONF9 "vous devez utiliser remote|local\n"
PCICONF10 "m�moire ou fichier programme alt�r�\n"
PCICONF12 "nom-jeton inconnu.\n"

$ these are debug messages only
PCICONF15 "d�bogage activ�\n"
PCICONF16 "SET\n"
PCICONF17 "DISPLAY <NOM>\n"
PCICONF18 "RETURN\n"
PCICONF19 "Erreur : action incorrecte : %1\n"

$ These are used to list the values of local/remote drive and nbs,nd on/off 
PCICONF22 "%1 : %2\n"
PCICONF23 "activ�"
PCICONF24 "off"
PCICONF25 "distant"
PCICONF26 "local"

$domain LCC.PCI.DOS.PCIINIT
$ The following messages are from pciinit.c
PCIINIT1 "impossible de trouver le p�riph�rique BRIDGE correct\n"

$ The following shows an initialization error and the associated errno
$ PCIINIT2 is a discontinued message
PCIINIT2 "erreur survenue lors de l'initialisation $ %1\n"

PCIINIT_BAD_DOS     "erreur : initialisation non effectu�e, DOS non identifiable\n"
PCIINIT_BAD_BRIDGE  "erreur : initialisation non effectu�e, BRIDGE.DRV INCORRECT\n"
PCIINIT_DRIVER_VER  "avertissement : version du pilote r�seau incorrecte\n"
PCIINIT_DRIVER_INIT "avertissement : l'initialisation du pilote r�seau a �chou�\n"
PCIINIT_NO_NET \\x13
      "erreur : initialisation non effectu�e, pas d'interface r�seau ou de ports RS232\n"

PCIINIT_FOR_NET   "� utiliser avec l'interface r�seau\n"
PCIINIT_FOR_RS232 "� utiliser avec les ports RS232\n"
PCIINIT_FOR_BOTH  "� utiliser avec l'interface r�seau ou les ports RS232\n"

PCIINIT_DRIVES    "Unit�s PC-Interface %1 � %2\n"
PCIINIT_NETWARE_DRIVES "La premi�re unit� NetWare est %1\n"

PCIINIT3 "avertissement : impossible de d�finir le nom de la machine\n"

$ An improperly formed (one without a leading '-' or '/' ) was given.
PCIINIT4 "argument <%1> incorrect\n"

$ An invalid option was given
PCIINIT5 "option <%1> inconnue\n"

$ The broadcast address given is not in the proper format 
PCIINIT6 "adresse de diffusion <%1> incorrecte\n"

$ there was no 'localhost' internet address found for the PC
PCIINIT7 "entr�e '%1' introuvable\n"

$ the localhost address is not in the proper format or invalid
PCIINIT8 "adresse locale <%1> incorrecte\n"

$  The address does not conform to the internet Class A, B or C values
PCIINIT9 "Classe d'adresse \"%1\" incorrecte\n"

$ Both the EXCELAN and the HOSTS environment variables must agree
PCIINIT10 "\"HOSTS\" et \"EXCELAN\\tcp\" doivent concorder\n"

PCIINIT11 "impossible d'ouvrir %1\n"
PCIINIT12 "la version du pont doit �tre sup�rieure � 2.7.2\n"

$ The next two messages refer to the HOSTS file
PCIINIT13 "impossible d'ouvrir le fichier %1.\n"
PCIINIT14 "%1 est alt�r� ou incorrect.\n"


PCIINIT15 "unit� � distance : %1\n"
PCIINIT16 "Version %1.%2.%3 (s�rie n�%4.%5) initialis�e\n"

$ this prints the ip address
PCIINIT17 "%1"
PCIINIT18 "Adresse IP = "
PCIINIT19 "\nAdresse de diffusion = "

$ This is the program name
PCIINIT21 "pciinit : "

$ This is the product name
PCIINIT24 "PC-Interface "

$ This is printed if pciinit has already been run
PCIINIT25 "d�j� initialis�"

$ This warns the user to run PCIINIT without the /e option to initialize the
$ software.
PCIINIT26 "Avertissement : PC-I n'a pas �t� initialis�.\n\
Ex�cutez PCIINIT sans l'option /e pour initialiser PC-I.\n"

$ The subnet mask is not in the proper format 
PCIINIT27 "masque de sous-r�seau <%1> incorrect\n"

$ The subnet mask is not in the proper format 
PCIINIT28 "masque de sous-r�seau <%1.%2.%3.%4> incorrect\n"

$ Netware is already installed
PCIINIT30 "initialisation impossible apr�s l'initialisation de NetWare\n"

$ usage message for pciinit
PCIINIT_U "\
syntaxe : pciinit [/i<h�te>] [/b<h�te>] [/s<masque>] [<r�pertoire-en-s�rie>]\n\
       o� <h�te> est un nom ou une adresse IP\n\
       o� <masque> est un nom ou un masque de sous-r�seau\n\
       pciinit /e        imprime le param�trage actuel de l'adresse r�seau\n\
       pciinit /h ou /?  imprime ce message\n"

$ for bridge versions prior to release 3.2
PCIINIT_OLD_BRIDGE "avertissement : version ant�rieure de bridge.drv\n"

$ This is an error found in the bridge
ERROR_MSG "Erreur - "

$ This is used to indicate that a particular host did not respond
GEN_ERROR" pas de r�ponse. Voulez-vous renouveler l'op�ration ? (O ou N) : "


DISC_MSG "Session PC-Interface termin�e. Ouvrez une session pour renouveler l'op�ration."
ERR_MSG	"PARAMETRE ERRONE !\r\n$"
DRV_MSG	 " unit�s virtuelles et $"
JOBS_MSG " entr�es de la table des t�ches\r\n\n$"

$ The copy protection violation message
VIOLATION "Tentative de duplication du programme prot�g� PC Interface - SYSTEME DESACTIVE"


$domain LCC.PCI.DOS.PRINTER
$ The following messages are from printer.c.
$
$ These tokens are the words that the user types to select
$ remote/local printer operation.
PCI_REMOTE_TOKEN "distant"
PCI_LOCAL_TOKEN "local"
MRG_REMOTE_TOKEN "unix"
MRG_LOCAL_TOKEN "dos"
AIX_REMOTE_TOKEN "aix"

$ the DOS call to set the printer state failed
PRINTER1 "imprimante : impossible de d�finir l'�tat.\n"

$ the printer name given is not in the range allowed or is non-numeric
PRINTER2 "%1 : nom du train d'impression incorrect.\n"

$ This message says that you can't reset the printer program
$ when you are just setting it
PRINTER3 "%1 incorrect lors de la d�finition du programme d'impression\n"


PRINTER4 "Conflit entre les options /P et /D\n"

$ The following 5 thru 8 are the usage message for the multistream
$ version of PC-Interface, and the Merge version.
$ The PCI version uses PRINTER7 instead of PRINTER7_M.
$ The Merge version uses PRINTER7_M instead of PRINTER7.
PRINTER5 "Syntaxe : \nprinter\n"
$ %1 is the 'local' token, which is "local" in PCI and "dos" in Merge.
PRINTER6 "printer [LPTn] %1\n"
PRINTER7 "printer [LPTn] {h�te|unit�|-} [programme-impression|/R] [/X[0|1]] [/T[d�lai]]\n"
$ %1 is the 'remote' token, for Merge is "unix" (or "aix", on an AIX machine).
PRINTER7_M "printer [LPTn) %1 [programme-impression|/R]  [/X[0|1]] [/T[d�lai]]\n"
PRINTER8 "printer [LPTn] [/P|/D] [/X[0|1]] [/T[d�lai]]\n"
PRINTER_HELP "l'imprimante /h ou /? imprime ce message\n"

$ The following 9 and 10 are the usage message for the non-multistream
$ version of PC-Interface
PRINTER9 "syntaxe : imprimante {%1}\n"
PRINTER10 "               {%1} [/Tn]\n"

PRINTER11 "Impossible de d�finir les param�tres d'impression [LPT%1]\n"
PRINTER12 "Impossible de d�finir l'imprimante � distance LPT%1. Vous n'avez probablement pas ouvert de session.\n"
PRINTER13 "Seule l'imprimante LPT1 est support�e par votre version du pont.\n"
PRINTER14 "imprimante : impossible de d�finir l'�tat.\n"
PRINTER15 "imprimante : impossible d'obtenir l'�tat.\n"
PRINTER16 "L'option %1 n'est pas support�e par votre version du pont\n"
PRINTER22 "LPT%1 : "


$ This message prints out which drive is associated with which host
PRINTER23 " (%1 : %2)"

$ This message prints out the drive of the host for systems that support
$ only one host
PRINTER24 " (%1 :)"

$ the following messages, PRINTER25 through PRINTER33 are used
$ to create an informational display for each printer
$ the format of this message is
$ 	LPT#: [local|remote] , print on exit [on|off], 
$	[no timeout | timeout = # ], [default printer | <printprog>]
PRINTER25 ", impression en sortie "
PRINTER26 "activ�"
PRINTER27 "off"
PRINTER28 ", d�lai = %1"
PRINTER29 ", pas de d�lai"
PRINTER30 ",\n      "
PRINTER31 ", "
PRINTER32 "\"%1\"\n"
PRINTER33 ", imprimante par d�faut\n"

$ this says whether the printer's state is local or remote
PRINTER34 "Etat actuel de l'imprimante : %1"

PRINTER35 " avec un d�lai de %1 seconde."
PRINTER36 " sans d�lai."

$ This message indicates that a particular drive is not connected
PRINTER37 "%1 : n'est pas connect�"

$ This message is self-explanatory.
PRINTER38 "M�moire insuffisante.\n"

$domain LCC.PCI.DOS.UDIR
$ The following messages are from udir.c

$ %1 is drive number
UDIR1 " Volume de l'unit� %1 "
UDIR2 "n'a pas de libell�\n"

$ %1 is the drive label 
UDIR3 "est %1\n"

$ %1 is the path name
UDIR4 " R�pertoire de %1\n"

$ This is the format of the udir output line
$ %1 is the UNIX side name, %2 is the mapped DOS file name,
$ %3 is the owner's name, %4 is the file attributes
UDIR5 "%1 %2%3%4"

$ This is the message that the file is a directory
UDIR7 "<REP>    "

$ This completes the output line with date (%1) followed by time (%2)
UDIR9 "%1%2\n"

UDIR14 "\nFichier introuvable\n"

$ %1 is the number of files found, %2 is for spacing, %3 is the number
$ of bytes found
UDIR15 "\t%1 Fichier(s)%2%3 octets disponibles\n"

$ the drive given is invalid
UDIR17 "Sp�cification de l'unit� incorrecte\n"

$ udir usage message
UDIR18 "\
syntaxe : udir [/a] [lettre_unit�:][chemin][r�pertoire|nom_fichier]\n\
          udir /h ou /?    - imprime ce message\n"


$ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
$ ! 	NOTE - convert.c and getopt.c messages are		!
$ !	 stored on the UNIX side in util			!
$ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

$domain LCC.PCI.DOS.DOSWHAT

DOSWHAT2 "Syntaxe : %1 [/f] [/w] fichier [fichier] ...\n\
          %1 /h ou /? imprime ce message\n"

$ %1 is the file name
DOSWHAT3 "%1 : impossible de trouver %2\n"
DOSWHAT5 "%1 :\n\tr�pertoire\n"
DOSWHAT7 "%1 :\n\tp�riph�rique\n"
DOSWHAT8 "%1 : impossible d'ouvrir %2\n"
DOSWHAT9 "%1 : impossible d'ex�cuter fstat sur %2\n"
DOSWHAT11 "%1 :\n\tp�riph�rique\n"

DOSWHAT14 "Type de fichier ind�termin� : lancez la commande du fichier UNIX.\n"

DOSTYPE0 " Fichier binaire\n"
DOSTYPE1 " Fichier texte ascii DOS\n"
DOSTYPE2 " Fichier texte ascii UNIX\n"
DOSTYPE3 " R�pertoire\n"
DOSTYPE4 " P�riph�rique\n"

$domain LCC.PCI.DOS.PCIDEBUG
$quote "
$ The following messages are from pcidebug.c
$ %1 is the program name - pcidebug
PCIDEBUG1 "%1 : Syntaxe : %1 <h�te|unit�> <[=+-~]chanList|on|off|close> [...]\n\t\t\t\t\tchanList = chanNum1[,chanNum2[...]]\n\
\t\t %1 /h ou /?  imprime ce message\n"

$ %1 is the program name, %2 is the argument
PCIDEBUG2 "%1 : argument incorrect : \"%2\"\n"

$ %1 is the program name, %s is the bit value
PCIDEBUG3 "%1 : bit %2 hors limites\n"

$ %1 is the program name, %2 is the ioctl value
PCIDEBUG4 "%1 : erreur dans ioctl %2\n"

$ the next three are tokens 
$ if they are changed, change them in PCIDEBUG1 to agree
PCIDEBUG10 "off"
PCIDEBUG11 "activ�"
PCIDEBUG12 "fermer"


$ The following messages are from vdrive.c
$domain	LCC.PCI.DOS.NLSVD
$quote "

USEAGENODRIVE "Aucune unit� virtuelle PCI n'a ouvert de session.\n"
NODRIVE "Aucune unit� virtuelle PCI n'a ouvert de session.\n"
BADDRIVE "Identificateur d'unit� %1 incorrect\n"
USEAGEBADDRIVE "Identificateur d'unit� %1 incorrect\n"
USEAGENOTADRIVE "L'unit� %1 n'est pas une unit� virtuelle.\n"
FATAL "Erreur bloquante survenue lors de l'obtention du nom de l'h�te.\n"
CONNECT "%1 : est connect� � %2.\n"

$ SCCSID("@(#)messages	6.1	LCC")	/* Modified: 10/15/90 15:48:13 */
$domain LCC.PCI.DOS.CONVERT
$quote "
$ NOTE: '\n' indicates that a new line will be printed
$ The following messages are from convert.c
CONVERT1 "Majuscules ET minuscules sp�cifi�es\n"
CONVERT1A "Des options incompatibles on �t� sp�cifi�es\n"
CONVERT2 "unix2dos ET dos2unix sp�cifi�s\n"

$ %1 is the file name on which a read error occured
CONVERT3 "Erreur survenue lors de la lecture de %1\n"

$ %1 is the fiel name which cannot be opened
CONVERT4 "Impossible d'ouvrir %1\n"

$ input file %1 and output file %2 are identical
CONVERT5 "Les fichiers %1 et %2 sont identiques\n"

$ an error was encountered writing file %1
CONVERT6 "Erreur survenue lors de l'�criture dans %1\n"

CONVERT7 "Table de traduction des r�sultats non fournie\n"

CONVERT8 "Majuscules/minuscules sp�cifi�es sans indication d'une option de traduction\n"

$ translation table %1 cannot be opened
CONVERT10 "Impossible d'ouvrir la table de traduction %1\n"

CONVERT15 "Table de traduction incorrecte !\n"
CONVERT17 "Table de traduction des donn�es d'entr�e non fournie\n"
CONVERT21 "Echec de l'�criture dans le fichier de sortie !\n"
CONVERT30 "Impossible d'affecter de l'espace pour le tampon de traduction !\n"
CONVERT31 "Tables de traduction non d�finies !\n"

$ character %1 was untranslatable with the options used
CONVERT32 "\nCaract�re intraduisible � la ligne n� %1\n"

$ unknown error %1 occurred
CONVERT42 "Erreur de traduction inconnue : %1\n"

CONVERT45 "Table(s) de traduction introuvable(s) !\n"
CONVERT46 "Table(s) de traduction incorrecte(s) !\n"
CONVERT60 "Variable d'environnement COUNTRY non d�finie, utilisation de 1\n"

$ code page %1 will be used as no other was specified
CONVERT61 "Variable d'environnement CODEPAGE non d�finie, utilisation de %1\n"

CONVERT77 "Tables de traduction des donn�es d'entr�e et des r�sultats non fournies\n"
CONVERT80 "Avertissement ! Caract�re � 8 bits\n"
CONVERT86 "M�moire insuffisante pour ex�cuter CONVOPT !\n"
CONVERT90 "Impossible d'affecter davantage de m�moire !\n"
CONVERT_B1 "Impossible d'ex�cuter la meilleur conversion possible en un caract�re unique avec le jeu de caract�res par d�faut !\n"
CONVERT_S1 "\nInformations relatives � la traduction :\n"

$ %1 is the number of glyphs
CONVERT_S2 "Traduction exacte des signes :\t\t%1\n"
CONVERT_S3 "Traduction des signes en plusieurs octets :\t%1\n"
CONVERT_S4 "Traduction des signes en valeurs par d�faut utilisateur :\t%1\n"
CONVERT_S5 "Meilleure conversion possible des signes en un signe unique :\t%1\n"
CONVERT_S6 "\nNombre total de signes trait�s :\t%1\n"

$ %1 is the number of bytes, %2 is the number of lines in the text file
CONVERT_S7 "\n%1 octets ont �t� trait�s sur %2 lignes\n"

$ %1 is the name of the program
CONVERT_M1_D "syntaxe : %1   [/options] ... [entr�e [sortie]]\n"
CONVERT_M3_D "Les options comprennent : /u      Fichier de majuscules.\n\
\                         /l      Fichier de minuscules.\n\
\                         /f      Forcer (ancienne option).\n\
\                         /b      Binaire (ancienne option).\n"
CONVERT_M4_D "\                /7      G�n�rer un avertissement si un caract�re utilise le 8�me bit.\n\
\                /x      Transmission directe.  Ne pas traduire.\n\n\
\                /i tbl  Traduire les donn�es d'entr�e au moyen de la table tbl.\n\
\                /o tbl  Traduire les r�sultats au moyen de la table tbl.\n\
\n"
CONVERT_M5_D "\                /c c    Utiliser c comme user_char en cas d'�chec de la traduction.\n\
\                /m      Autoriser les traductions en plusieurs caract�res.\n\
\                /a      Abandonner la traduction en cas d'�chec.\n\
\                /s      Utiliser la meilleure conversion possible en un caract�re unique.\n\
\                /z      Traiter C-Z correctement (dos2unix/unix2dos).\n\
\n"
CONVERT_M6_D "\                /d      Introduire un retour chariot pour ex�cuter un changement de ligne avant la traduction.\n\
\                /p      Ex�cuter un changement de ligne pour introduire un retour chariot avant la traduction.\n\
\n"
CONVERT_M7_D "\                /q      Supprimer l'affichage des messages d'avertissement.\n\
\                /v      Afficher les messages d'avertissement et les statistiques de traduction.\n\
\                /h ou /? Imprimer ce message.\n\
"


CONVERT_M1_U "syntaxe : %1   [-options] ... [entr�e [sortie]]\n"
CONVERT_M3_U "Les options comprennent : -u      Fichier de majuscules.\n\
\                         -l      Fichier de minuscules.\n\
\                         -f      Forcer (ancienne option).\n\
\                         -b      Binaire (ancienne option).\n"
CONVERT_M4_U "\                -7      G�n�rer un avertissement si un caract�re utilise le 8�me bit.\n\
\                -x      Transmission directe.  Ne pas traduire.\n\n\
\                -i tbl  Traduire les donn�es d'entr�e au moyen de la table tbl.\n\
\                -o tbl  Traduire les r�sultats au moyen de la table tbl.\n\
\n"
CONVERT_M5_U "\                -c c    Utiliser c comme user_char en cas d'�chec de la traduction.\n\
\                -m      Autoriser les traductions en plusieurs caract�res.\n\
\                -a      Abandonner la traduction en cas d'�chec.\n\
\                -s      Utiliser la meilleure conversion possible en un caract�re unique.\n\
\                -z      Traiter C-Z correctement (dos2unix/unix2dos).\n\
\n"
CONVERT_M6_U "\                -d      Introduire un retour chariot pour ex�cuter un changement de ligne avant la traduction.\n\
\                -p      Ex�cuter un changement de ligne pour introduire un retour chariot avant la traduction.\n\
\n"
CONVERT_M7_U "\                -q      Supprimer l'affichage des messages d'avertissement.\n\
\                -v      Afficher les messages d'avertissement et les statistiques de traduction.\n\
\                -h ou -? Imprimer ce message.\n\
"

$ %1 is the filename, %2 is the options
CONVERT_HELP00_D "syntaxe : %1 [/%2] [fichier_entr�e [fichier_sortie]]\n"
CONVERT_HELP00_U "syntaxe : %1 [-%2] [fichier_entr�e [fichier_sortie]]\n"
CONVERT_HELP01_D "       %1 -h ou /? pour obtenir des informations d�taill�es\n"
CONVERT_HELP01_U "       %1 -h ou -? pour obtenir des informations d�taill�es\n"
CONVERT_HELP1_D "syntaxe : %1 /%2 [fichier_entr�e [fichier_sortie]]\n"
CONVERT_HELP1_U "syntaxe : %1 -%2 [fichier_entr�e [fichier_sortie]]\n"

$ The following messages are from getopt.c
$ Do NOT change of the order of %1 and %2!
GETOPT1 "Option %1%2 inconnue\n"
GETOPT2 "Argument manquant pour l'option %1%2\n"
$ SCCSID("@(#)messages	5.1	LCC")	/* Modified: 6/3/91 13:46:54 */
$quote "
$domain LCC.PCI.DOS.HOSTDRV
$ The following messages are from hostdrv.c
$ %1 is the drive letter given
HOSTDRV1 "%1 :  n'est pas une unit� virtuelle"
HOSTDRV2 "L'h�te %1 n'est pas connect�"

$domain LCC.PCI.DOS.HOSTOPTN
$ The following messages are from hostoptn.c
HOSTOPTN1 "option(s) de s�lection de l'h�te incorrecte(s)"

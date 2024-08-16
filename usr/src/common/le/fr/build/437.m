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

UNKNOWN "Message inconnu :\n"
ERROR	"ERREUR : %1 %2 %3 %4 %5 %6\n"
WARNING	"AVERTISSEMENT :\n"
$	-	dosexec
AOPT_BAD "AOPT_BAD : %1 : ERREUR : mauvaise utilisation de l'option 'a' '%2=%3'\n"
$	-	dosexec
$	-	dosopt
AOPT_TOOMANY "AOPT_TOOMANY : %1 : ERREUR : trop de p‚riph‚riques affect‚s.\n\
Impossible de continuer.\n"
$	-	crt
$	-	tkbd
BAD_ARGS "BAD_ARGS : %1 : ERREUR : des paramŠtres incorrects ont ‚t‚ transmis … ce programme.\n"
$	-	dosexec
BAD_IMG "BAD_IMG : %1 : ERREUR : le fichier image '%2' ne correspond pas au type attendu.\n"
$	-	crt
$	-	tkbd
BAD_LCS_SET_TBL "BAD_LCS_SET_TBL : %1 : ERREUR : table des jeux de codes incorrecte.\n\
\   Impossible d'utiliser la table %2 ou %3. n§ d'erreur=%4."
$	-	dosopt
BAD_OPT_USAGE "BAD_OPT_USAGE : %1 : ERREUR : utilisation incorrecte de l'option '%2'.\n"
$	-	dosexec
BAD_SWD "BAD_SWD : %1 : ERREUR : ‚chec de la communication UNIX-DOS.\n"
$	-	dosexec
BOPT_ERR "BOPT_ERR : %1 : ERREUR : 'dos +b' non autoris‚.\n"
$	-	dosexec
CD_X "CD_X : %1 : ERREUR : la commande 'cd' initiale a ‚chou‚ sur l'unit‚ %2.\n\
Impossible de passer au r‚pertoire : %3%4\n"
$	-aix	pssnap
$	-	romsnap
CHOWN_ERR "CHOWN_ERR : %1 : ERREUR :  impossible d'ex‚cuter la commande chown sur le fichier '%2', n§ d'erreur = %3.\n"
$	-	dosexec
CL_TOOLONG "CL_TOOLONG : %1 : ERREUR : ligne de commande trop longue.\n"
$	-	dosexec
CODEPAGE_NS "CODEPAGE_NS : %1 : ERREUR : traduction de DOS CODEPAGE '%2' non support‚e.\n\
\    (fichier CODEPAGE '%3/%4' introuvable. err=%5)\n"
$	-	dosexec
CODESET_NS "CODESET_NS : %1: ERREUR : traduction du jeu de codes UNIX '%2' non support‚e.\n\
\    (Fichier de jeux de codes '%3/%4' introuvable. err=%5)\n"
$	-	dosexec
CODESET_USE "\n\
Utilisation du jeu de codes '%2'.\n"
$	-	dosexec
CONFIG_NF "CONFIG_NF  : %1 : ERREUR : impossible d'utiliser le fichier config.sys %2.\n\
Impossible d'acc‚der au fichier en lecture.\n"
$	-	newunix
CONS_ONLY "CONS_ONLY : %1 : ERREUR : ex‚cutable uniquement sur la console.\n"
$	-	romsnap
$	-	dosopt
CREATE_ERR "CREATE_ERR : %1 : ERREUR : impossible de cr‚er le fichier '%2', n§ d'erreur = %3.\n"
$	-	dosopt
CRT_DISPLAY "CRT_DISPLAY : %1 : ERREUR : impossible d'utiliser le type d'affichage '%2' avec 'crt'.\n"
$	-	dosexec
CWD_TOOLONG "CWD_TOOLONG : %1 : ERREUR : le nom du r‚pertoire de travail actuel est trop long %2.\
\nMax=%3.\n"
$	-	dosexec
DA_FAIL "DA_FAIL : impossible d'attacher '%2' directement. Erreur au niveau du p‚riph‚rique '%3'.\n"
$	-	dosexec
DEV_NA "DEV_NA : %1 : ERREUR : le p‚riph‚rique requis %2 n'est pas disponible.\n"
$	-	dosexec
DEV_TOOMANY "DEV_TOMANY : %1  ERREUR : trop de p‚riph‚riques attach‚s.\n"
$	-	dosopt
DFLTS "* paramŠtres par d‚faut de %2.\n"
$	-	dosopt
NO_OPTIONS "Pas d'option dans %1.\n"
$	-	dosexec
DINFO_BYTES "Octets"
DINFO_CGA "CGA"
DINFO_COM1 "COM1"
DINFO_COM2 "COM2"
DINFO_DIRECT "direct"
DINFO_DOS_DIR "DOS (direct)"
DINFO_DOSPART "Partitions DOS :"
DINFO_EGA "EGA"
DINFO_RAM "RAM"
DINFO_EMS "EMS"
DINFO_FDRIVES "Unit‚s de disquettes :"
DINFO_HERC "HERC"
DINFO_LPT "LPT"
DINFO_MONO "MONO"
DINFO_MOUSE "Souris"
DINFO_NONE "aucun"
DINFO_OTHER "AUTRE"
DINFO_PMSG1 "Informations relatives aux p‚riph‚riques DOS"
DINFO_PMSG2 "Appuyez sur la <barre d'espacement> pour continuer"
DINFO_TTY "TTY"
DINFO_UD_SHARED "Partage UNIX/DOS :"
DINFO_UNIX_DEF "UNIX par d‚faut"
DINFO_UNKNOWN "inconnu"
DINFO_USIZE "taille inconnue"
DINFO_VGA "VGA"
DINFO_VIDEO "Vid‚o"
DINFO_2BUTTON "Souris … 2 boutons"
$	-	display
DISPLAY_USAGE	"Syntaxe : display [nom_de_p‚riph‚rique]\n\
\    Si le nom de p‚riph‚rique n'est pas sp‚cifi‚, la valeur\n\
\    par d‚faut correspond au p‚riph‚rique standard.\n"
$	-	dosexec
DMA_NA "DMA_NA : %1 : ERREUR : impossible d'affecter le canal dma %2\n"
$	-	dosexec
DOSDEV_ERR "DOSDEV_ERR : %1 : ERREUR : la ligne %2 du fichier de d‚finition des p‚riph‚riques\n\
'%3'\n\
est incorrecte :\n\
%4\n"
$	-	dosexec
DOSPART_TWOSAME "%1 : avertissement : vous tentez d'attacher la mˆme partition DOS \n\
aux deux unit‚s %2 et %3 accessibles EN ECRITURE. Cette op‚ration n'est pas autoris‚e.\n\
D‚finissez l'accŠs … ces unit‚s EN LECTURE UNIQUEMENT.\n"
$	-	dosexec
DPOPT_ERR "DPOPT_ERR : %1: ERREUR : impossible d'ex‚cuter automatiquement le\
\  fichier autoexec.bat sur l'unit‚ %2.\n\
(Les options +p et +d%2 ne peuvent pas ˆtre utilis‚es simultan‚ment)\n"
$	-	dosexec
DRIVE_LETTER "DRIVE_LETTER : %1 : ERREUR : lettre de l'unit‚ %2 non autoris‚e\n"
$	-	dosexec
EM_FAIL "EM_FAIL : %1 : ERREUR : l'‚mulateur n'a pu ˆtre initialis‚.\n"
$	-	dosexec
EM_VER "EM_VER : %1: ERREUR : vous n'avez pas install‚ la bonne version de l'‚mulateur.\n"
$	-	dosexec
ENV_NOTSET "ENV_NOTSET : %1 : ERREUR : la variable d'environnement requise '%2'\
\  n'est pas d‚finie.\n"
$	-	dosexec
ENV_TOOMUCH "ENV_TOOMUCH : %1 : ERREUR : environnement trop long %2. max=%3.\n"
$	-	crt
ERR_SEMAPHORE "ERR_SEMAPHORE : %1 : ERREUR : accŠs au s‚maphore.\n"
$	-	dosexec
EXEC_CMDCOM "EXEC_CMDCOM : %1 : ERREUR : l'ex‚cution de command.com a ‚chou‚.\n"
$	-	dosexec
EXEC_ERR "EXEC_ERR : %1 : ERREUR : %2 n'est pas install‚ correctement.\n"
$	-	dosexec
EXEC_FAILED "EXEC_FAILED :  %1 :  ERREUR : l'ex‚cution de %2 a ‚chou‚, n§ d'erreur= %3.\n"
$	-	dosexec
EXIT_SIGNAL "EXIT_SIGNAL : %1 : ERREUR : signal %2 d‚tect‚.\n"
$	-	dosexec
FATAL "FATAL : erreur bloquante dans %1 :\n%2\n"
$	-	dosopt
FILE_OPTS_ERR "FILE_OPTS_ERR : %1 : ERREUR : des op‚rations incorrectes ont ‚t‚ ex‚cut‚es sur les fichiers.\n"
$	-	dosopt
FN "\nFichier : %2\n"
$	-	dosopt
FN_TOOLONG "FN_TOOLONG : %1 : ERREUR : nom de fichier trop long : %2. max=%3.\n"
$	-	dosexec
FORK_FAIL "FORK_FAIL : %1 : ERREUR : ‚chec du processus fourche interne.  n§ d'erreur=%2\n\
(Trop de processus en cours ou m‚moire insuffisante.)\n"
$
FORK_EXEC_FAIL "FORK_EXEC_FAIL : %1 : ERREUR : ‚chec du processus fourche interne.  n§ d'erreur=%2\n\
(Trop de processus en cours ou m‚moire insuffisante.)\n\
Impossible d'ex‚cuter %3\n"
$	-	dosexec
IA_FAIL "IA_FAIL : '%2' non disponible.\n\
Le p‚riph‚rique '%3' est actif ou ne dispose pas des autorisations appropri‚es.\n"
$	-	dosexec
$	-	dosopt
ILL_DRV "ILL_DRV : %1 : ERREUR : unit‚ '%2' non autoris‚e.\n"
$	-	dosexec
$	-	dosopt
ILL_MX "ILL_MX : %1 : ERREUR : s‚lecteur de m‚moire '%2' non autoris‚.\n"
$	-	dosexec
$	-	dosopt
ILL_OPT "ILL_OPT : %1 : ERREUR : option '%2' non autoris‚e.\nUtilisez +h pour obtenir la liste des options.\n"
$	-	dosexec
IMPROPER_INSTL "IMPROPER_INSTL : %1 : ERREUR : installation incorrecte.\n\
\  Autorisations ou privilŠges incorrects.\n"
$	-	dosexec
INITDEV_FAIL "INITDEV_FAIL : %1 : ERREUR : impossible d'initialiser '%2=%3'.\n"
$	-	dosexec
INIT_FAIL "INIT_FAIL : %1 : ERREUR : impossible d'initialiser le p‚riph‚rique '%2'.\n"
$	-	dosexec
INTERNAL_ERR "INTERNAL_ERR : %1 : ERREUR : erreur interne. Echec de %2. %3 %4 %5 %6\n"
$	-	dosexec
MEMORY_ERR "MEMORY_ERR : %1 : ERREUR : m‚moire insuffisante.\n"
$	-	dosexec
INVALID_DEV "INVALID_DEV : %1 : ERREUR : %2 n'est pas un p‚riph‚rique correct. (%3)\n"
$	-	dosexec
INVALID_TMM "INVALID_TMM : %1 : ERREUR : %2 n'est pas un p‚riph‚rique correct.\n\
(Espace m‚moire incorrect affect‚ au registre d'E/S %3-%4)\n"
$	-	dosexec
INVALID_MEMMAP "INVALID_MEMMAP : %1 : ERREUR : l'espace m‚moire \n\
directement accessible affect‚ au registre E/S %2-%3 est incorrect\n"
$	-	dosexec
BAD_PRINTER_CMD "BAD_PRINTER_CMD : commande d'imprimante incorrecte pour %s.\n"
$	-	crt
$	-	xcrt
IOCTL_FAIL "IOCTL_FAIL : %1 : ERREUR : ioctl du p‚riph‚rique %2 sur %3 a ‚chou‚. n§ d'erreur=%4.\n"
$	-	dosexec
KILL_FAIL "KILL_FAIL : %1 : AVERTISSEMENT: problŠmes survenus lors de l'arrˆt du processus ‚cran.\n"
$	-	dosexec
LOADDEV "LOADDEV : %1 : ERREUR : erreur Loaddev.\n"
$	-	dosexec
LOAD_FAIL "LOAD_FAIL : %1 : ERREUR : le chargement du fichier image '%2' a ‚chou‚.\nn§ d'erreur = %3.\n"
$	-	dosexec
MAX_USER "Impossible de lancer une autre session DOS/Windows car le \n\
nombre maximum d'utilisateurs DOS/Windows (%2) autoris‚s emploient actuellement le systŠme.\n"
$	-	dosexec
MEM_NA "MEM_NA : %1 : ERREUR : impossible d'affecter la m‚moire %2\n"
$	-	dosexec
$	-	dosopt
MEM_OOR "MEM_OOR : %1 : ERREUR : la quantit‚ de m‚moire requise est hors limites. Max=%2 Min=%3.\n"
$	-	dosexec
MERGE_NA "MERGE_NA : %1 : ERREUR : Merge n'est pas configur‚ sur votre systŠme.\n\
\  Impossible d'ex‚cuter DOS.\n"
$	-	dosexec tkbd ...
MERGE_DEV_NA "MERGE_DEV_NA : %1 : ERREUR : impossible d'acc‚der aux appels systŠme Merge.\n"
$	-	newunix
NEWUNIX_USAGE "\nSyntaxe :  %2 [-n nombre_terminaux_virtuels] [-p] [[-e] commande]\n"
$	-	dosexec
NO_DOS "NO_DOS : %1 : ERREUR : DOS n'est pas install‚.\n\
\  Vous devez ex‚cuter 'dosinstall' pour installer DOS avant d'utiliser Merge.\n"
$	-	dosexec
NO_PATH "NO_PATH : %1 : ERREUR : le CHEMIN UNIX  n'est pas d‚fini.\n"
$	-	dosexec
NO_LICENCE "NO_LICENCE : %1 : aucune licence n'est disponible pour ex‚cuter\nDOS/Windows maintenant.\n"
$	-	dosexec
NO_USERS "NO_USERS : %1 : les services DOS/Windows Merge ont ‚t‚ arrˆt‚s.\n\
Vous n'ˆtes pas autoris‚ … utiliser DOS/Windows maintenant.\n"
$	-	dosexec
$	-	crt
$	-	tkbd
$	-	dosexec
NOT_DEV "NOT_DEV : %1 : ERREUR : %2 (%3) n'est pas un p‚riph‚rique.\n"
$	-	dosexec
$	-	dosopt
NOT_DOS_PROG "NOT_DOS_PROG : %1 : ERREUR : '%2' n'est pas un programme DOS.\n"
$	-	dosopt
NOT_RW_ERR "NOT_RW_ERR : %1 : ERREUR : impossible d'ouvrir le fichier '%2'.\n\
\  Le fichier n'existe pas ou est prot‚g‚ en lecture/‚criture.\n"
$	-	dosopt
NOT_RW_WARN "NOT_RW_WARN : %1 : ATTENTION Impossible d'ouvrir le fichier '%2'.\n\
\  Le fichier n'existe pas ou est prot‚g‚ en lecture/‚criture.\n"
$	-	dosexec
$	-	dosopt
$	-	display
$	-aix	pssnap
$	-	romsnap
NOT_R_ERR "NOT_R_ERR : %1 : ERREUR : impossible d'ouvrir le fichier '%2'.\n\
\  Le fichier n'existe pas ou est prot‚g‚ en lecture.\n"
$	-	dosopt
NOT_R_WARN "NOT_R_WARN : %1 : ATTENTION Impossible d'ouvrir le fichier '%2'.\n\
\  Le fichier n'existe pas ou est prot‚g‚ en lecture.\n"
$	-	dosopt
NOT_W_ERR "NOT_W_ERR : %1 : ERREUR : impossible d'ouvrir le fichier '%2'.\n\
\  Le fichier n'existe pas ou est prot‚g‚ en ‚criture.\n"
$	-	dosexec
NO_BG_CMD "NO_BG_CMD : %1 : ERREUR : impossible d'ex‚cuter un programme\n\
\ d'affichage DOS en arriŠre-plan. Utilisez 'dos ...' au lieu de 'dos ... &'\n"
$	-	dosexec
NO_BG_DOS "NO_BG_DOS : %1 : ERREUR : impossible de rediriger une\n\
\  session DOS ou d'ex‚cuter une session DOS en arriŠre-plan.\n"
$	-	dosopt
NO_FILENAME "NO_FILENAME : %1 : ERREUR : vous devez indiquer un nom de fichier.\n"
$	-	crt
NO_GRAPHICS "\
\r\n\
\          +============================+ \r\n\
\          |                            | \r\n\
\          |            PAS             | \r\n\
\          |         DE GRAPHIQUE       | \r\n\
\          |            SUR             | \r\n\
\          |       LE TERMINAL ASCII    | \r\n\
\          |                            | \r\n\
\          |  Quittez le mode graphique | \r\n\
\          |                            | \r\n\
\          +============================+ \r\n\
\r\n\
\n"
$	-	dosexec
NO_INIT_DATA "NO_INIT_DATA : %1 : ERREUR : impossible d'obtenir les donn‚es d'initialisation.\n"
$	-	dosexec
NO_MORE "NO_MORE : %1 : ERREUR : m‚moire insuffisante. n§ d'erreur=%2\n"
$	-	dosexec
$	-	newunix
NO_RESOURCES "NO_RESOURCES : %1 : ERREUR : plus de ressources disponibles.\n"
$	-	newunix
NO_VT "NO_VT : %1 : ERREUR : aucun terminal virtuel disponible.\n"
$	-	dosexec
NO_VMPROC "NO_VMPROC : %1 : ERREUR : pas de processus de m‚moire virtuelle.\n"
$	-	crt
NO_WIDE "\
\r\n\
\          +===============================+ \r\n\
\          |                               | \r\n\
\          |       80 COLONNES SEULEMENT   | \r\n\
\          |              SUR              | \r\n\
\          |        UN TERMINAL ASCII      | \r\n\
\          |                               | \r\n\
\          |  Quittez le mode 132 colonnes | \r\n\
\          |                               | \r\n\
\          +===============================+ \r\n\
\r\n\
\n"
$	-	dosexec
ON_ERR "ON_ERR : %1 : ERREUR : impossible d'ex‚cuter le programme d'affichage DOS avec ON.\n"
$	-	dosexec
PATH_TOOMUCH "PATH_TOOMUCH : %1 : ERREUR : environnement PATH trop long %2. max=%3.\n"
$	-	dosexec
PROC_EXIT "PROC_EXIT : %1 : ERREUR : arrˆt inattendu du processus obligatoire %2.\n\
Abandon.\n"
$	-	dosexec
$	-	dosopt
$	-aix	pssnap
$	-	romsnap
$	-	tkbd
READ_ERR "READ_ERR : %1 : ERREUR : impossible de lire le fichier '%2'.\n"
$	-	dosexec
PORT_NA "PORT_NA : %1 : ERREUR : impossible d'affecter port(s) %2\n"
$	- dosexec (SVR4 only)
REBOOT_FOR_PARTITIONS "\
%1 : avertissement :\n\
Les partitions DOS ne seront pas accessibles tant que vous n'aurez pas red‚marr‚ le systŠme.\n"
$	-	dosopt
RMV_OPTS_ERR1 "RMV_OPTS_ERR1 : %1 : ERREUR : impossible de supprimer l'option %2.\n\
\  Cette option n'a pas ‚t‚ install‚e."
$	-	dosopt
RMV_OPTS_ERR2 "RMV_OPTS_ERR2 : %1 : ERREUR : impossible de supprimer l'option %2.\n\
\  Option non autoris‚e.\n"
$	-	romsnap
ROMSNAP_BAD_ADDR "ROMSNAP_BAD_ADDR : %1 : ERREUR :  Adresse de d‚but rom %2 erron‚e.\n"
$	-	romsnap
ROMSNAP_USAGE "\nSyntaxe : romsnap  adresse-d‚but-rom  fichier-image-rom [-k].\n"
$	-	dosopt
$	-aix	pssnap
$	-	romsnap
SEEK_ERR "SEEK_ERR : %1 : ERREUR : problŠme survenu lors de l'utilisation du fichier %2. (La recherche a ‚chou‚).\n"
$	-	dosexec
SEND_SIG_ERR "SEND_SIG_ERR : %1  Impossible d'envoyer le signal %2 … %3, n§ d'erreur = %4.\n"
$	-	dosexec
SERIOUS	"SERIOUS : erreur grave dans %1 :\n%2\n"
$	-	dosexec
SHAREINIT "SHAREINIT : %1 : ERREUR : impossible d'initialiser le segment de m‚moire partag‚e.\n\
Erreur = %2 (%3)\n"
$	-	dosexec
SHELL_NF "SHELL_NF : %1 : ERREUR : impossible d'utiliser SHELL=%2. Fichier introuvable.\n"
$	-	crt
SHMA_FAILED "SHMA_FAILED : %1 : ERREUR : impossible d'utiliser la m‚moire partag‚e. n§ d'erreur=%2\n"
$	-	dosexec
SHMC_FAILED "SHMC_FAILED : %1 : ERREUR : impossible de cr‚er la m‚moire partag‚e.\n\
Erreur=%2 a=%3 s=%4\n"
$	-aix	dosexec
SITE_NI "SITE_NI : %1 : ERREUR : Merge n'est pas install‚ sur le site %2.\n"
$	-	dosexec
$	-	dosopt
SOPT_USAGE "SOPT_USAGE : %1 : ERREUR : valeur du d‚lai imparti +s hors limites.\n\
Pour n'indiquer aucun d‚lai, utilisez 0.\n\
Pour indiquer un d‚lai, utilisez les valeurs %2 … %3.\n"
$	-	dosexec
SWITCHDOS_FAIL "SWITCHDOS_FAIL : %1 : ERREUR : ‚chec de la communication UNIX-DOS.\n\
n§ d'erreur=%2.\n"
$	-	dosexec
$	-	crt
SWKEY_FAIL "SWKEY_FAIL : ERREUR : impossible d'ex‚cuter switchkey … partir d'un terminal.\n"
$	-	swkey
SWKEY_CUR "\nS‚quence actuelle de permutation des modes d'‚cran Merge 386 et Xsight :  %2%3%4F<n>.\n"
$	-	swkey
SWKEY_USAGE "\n%1 : ERREUR : option incorrecte\n\
Syntaxe : switchkey [-acs]\n"
$	-	swkey
SWKEY_NEW "\nNouvelle s‚quence de permutation des modes d'‚cran Merge 386 et Xsight : %2%3%4F<n>.\n"
$	-	swkey
TERM_CAP "TERM_CAP : %1 : ERREUR : votre terminal ne dispose pas des fonctions\n\
\   permettant la prise en charge d'un programme d'affichage DOS … distance.\n"
$	-	dosexec
TERM_ERR "TERM_ERR : %1 : ERREUR : pas de terminfo associ‚ … TERM '%2'.\n"
$	-	dosexec
TOKENPAIR_NF "TOKENPAIR_NF : %1 : ERREUR : impossible de trouver\n\
les types de jetons correspondant …\n'%2=%3'.\n"
$	-	dosexec
TOKEN_BAD "TOKEN_BAD : %1 : ERREUR : jeton %2 (%3) incorrect.\n"
$	-	dosexec
TOKEN_NF "TOKEN_NF : %1 : ERREUR : impossible de trouver le type de jeton '%2' requis.\n"
$	-	dosexec
BAD_DRIVE "BAD_DRIVE : %1 : ERREUR : lettre %2 incorrecte associ‚e … l'unit‚.\n"
$	-	dosexec
LPT_BAD "LPT_BAD : %1 : ERREUR : LPT2 et LPT3 ne peuvent ˆtre associ‚s … un type\nde jeton d'attache direct.\n"
$	-	dosopt
UI_PROB "UI_PROB : %1 : ERREUR : impossible de d‚sinstaller le fichier '%2'.\n\
\     Le fichier n'est pas install‚.\n"
$	-	dosexec
UMB_CONFLICT "UMB_CONFLICT : %1 : ERREUR : conflit lors de l'affectation\n\
\ des blocs de la partie sup‚rieure de la m‚moire (UMB ou Upper Memory Blocks) %2-%3\n"
$	-	dosexec
UPOPT_ERR "UPOPT_ERR : %1 : ERREUR : les options +p et +u ne peuvent\
\  ˆtre utilis‚es simultan‚ment.\n"
$	-	dosexec
VECT_NA "VECT_NA : %1 : ERREUR : impossible d'affecter le vecteur d'interruption %2\n"
$	-	crt
VMATTACH_FAILED "VMATTACH_FAILED : %1 : ERREUR : impossible d'attacher la m‚moire vm86.\n\
n§ d'erreur=%2\n"
$	-	dosexec
VMSVR_FAIL "VMSVR_FAIL : %1 : l'initialisation du serveur VM a ‚chou‚, n§ d'erreur=%2.\n"
$	-	crt
$	-	dosexec
VM_DIED "VM_DIED : %1 : ERREUR : processus VM86 supprim‚.\n"
$	-	dosexec		janus_main.c
WAIT_RET_ERR "WAIT_RET_ERR : %1 :  ERREUR : appel systŠme wait renvoy‚ pr‚matur‚ment\n\
n§ d'erreur = %2.\n"
$	-	dosopt
$	-aix	pssnap
$	-	romsnap
WRITE_ERR "WRITE_ERR : %1 : ERREUR : impossible d'‚crire dans le fichier '%2'.\n"
$	-	dosopt
ZOPT_ERR "ZOPT_ERR : %1 : ERREUR : impossible d'utiliser l'option\n\
'z' ou 'Z' dans un fichier de paramŠtres par d‚faut %2.\n"
$	-	dosopt
ZOPT_EXTRA "ZOPT_EXTRA : %1 : ERREUR : vous ne pouvez indiquer qu'une option aprŠs 'z'.\n"
$	-	dosopt
ZOPT_MISSING "ZOPT_MISSING : %1 : ERREUR : option manquante aprŠs 'z'.\n"
$	-	dosopt
ZOPT_TOOMANY "ZOPT_TOOMANY : %1 : ERREUR : trop d'‚l‚ments +z sp‚cifi‚s. Impossible de continuer.\n"
$	-	dosexec
HELP_EXEC "\n\
\   Syntaxe : %1 [drapeaux] nom_de_fichier args ...\n\
\   Syntaxe : %1 [drapeaux]\n\
\   Le premier ‚cran utilise le nom de fichier DOS et le second utilise\n\
\   'command.com'de DOS.  Les drapeaux remplacent les options install‚es ou\n\
\   les paramŠtres par d‚faut et donnent des pseudo-instructions\n\
\   permettant de remplacer les op‚rations dosexec standard. \n\
\   ('+' indique que vous s‚lectionnez l'option et '-' indique que vous la\n\
\        d‚s‚lectionnez.)\n\
\     +- a[x] Affecte le p‚riph‚rique 'x'.\n\
\     +- b    Programme coop‚ratif. (redirige stdio vers/depuis UNIX)\n\
\     +  c    Commande : le nom du fichier de commandes DOS est transmis tel\n\
\             quel … DOS.\n\
\     +  dX   D‚finit l'unit‚ active initiale. X= a … z.\n\
\     +- e[f] Utilise le fichier de configuration des p‚riph‚riques 'f'.\n\
\     +  h    Affiche les informations d'aide (cet ‚cran). DOS n'est pas\n\
\             ex‚cut‚.\n\
\     +- l[f] Charge le fichier image dos 'f' ('f' peut ‚galement correspondre\n\
\             … un nom de r‚pertoire).\n\
\     +  mn   Capacit‚ de la m‚moire. Valeur 'n' (d‚cimale) en m‚ga-octets.\n\
\     +- p[f] D‚finit command.com comme "Permanent". 'f' indique le fichier\n\
\             autoexec.bat … ex‚cuter.\n\
\     +- s(n) Met la sortie imprimante DOS en attente sur l'imprimante UNIX.\n\
\             Le d‚lai est de 'n' secondes.\n\
\     +- t    Traduit les arguments de la ligne de commande DOS.\n"

$ 	-	dosopt
HELP_INST "\n\
Syntaxe : %1 [drapeaux] noms_fichiers\n\
\   Options d'installation associ‚es aux noms de fichiers. Les drapeaux\n\
\   d'installation sont pr‚c‚d‚s de '+' ou de '-'.  Le caractŠre en tˆte '+'\n\
\   ou '-' indique la s‚lection ou la suppression de cette option.\n\
\   Lorsqu'aucune option ou pseudo-instruction n'est indiqu‚e, les options\n\
\   actuelles sont imprim‚es.\n\
\     +- a[x] Affecte le p‚riph‚rique 'x'.\n  +- b Traite le programme.\n\
\             (redirige stdio vers/depuis UNIX)\n\
\     +  dX   D‚finit l'unit‚ active initiale. X= a … z.\n\
\     +- e[f] Utilise le fichier de configuration des p‚riph‚riques 'f'.\n\
\     +  h    Affiche les informations d'aide (cet ‚cran).\n\
\     +- l[f] Charge le fichier image dos 'f' ('f' peut ‚galement correspondre\n\
\             … un nom de r‚pertoire).\n\
\     +  mn   Capacit‚ de la m‚moire. Valeur 'n' (d‚cimale) en m‚ga-octets.\n\
\     +- p[f] D‚finit command.com comme "Permanent". 'f' indique le fichier\n\
\             autoexec.bat … ex‚cuter.\n\
\     +- s[n] Met la sortie imprimante DOS en attente sur l'imprimante UNIX.\n\
\             Le d‚lai est de 'n' secondes.\n\
\     +- t    Traduit les arguments de la ligne de commande DOS.\n\
\     +- v    Verbose. Affiche un accus‚ de r‚ception.\n\
\     +  y    Permet d'ex‚cuter les fichiers COM … partir d'UNIX. Cette option\n\
\             n'est pas n‚cessaire lorsque d'autres options sont d‚finies.\n\
\     +  zX   Supprime ou r‚initialise l'option X.\n\
\     +  Z    Supprime toutes les donn‚es d'installation du fichier.\n"
$
$ Note: The messages for the ifor_pm_* calls are for SCO only dosexec.
$
$ Error messages for error returns from 'ifor_pm_init_sco()'
$
$ --- For return value:  IFOR_PM_REINIT
PM_INIT_REINIT "%1: ATTENTION-  Tentative de r‚-initialisation du gestionnaire de proc‚dure.  (IFOR_PM_REINIT)\n"
$
$ --- For return value:  IFOR_PM_BAD_PARAM
PM_INIT_BAD_PARAM "%1: ERREUR-  ParamŠtre incorrect pour ifor_pm_init_sco.  (IFOR_PM_BAD_PARAM)\n"
$
$ --- For return value:  IFOR_PM_FATAL
PM_INIT_FATAL "%1: ERREUR-  L'initialisation du gestionnaire de proc‚dure a ‚chou‚.  (IFOR_PM_FATAL)\n"
$
$ Error messages for error returns from 'ifor_pm_request_sco()'
$
$ --- For return value:  IFOR_PM_BAD_PARAM
PM_REQ_BAD_PARAM "%1: ERREUR-  ParamŠtre incorrect pour ifor_pm_request_sco.  (IFOR_PM_BAD_PARAM)\n"
$
$ --- For return value:  IFOR_PM_NO_INIT
PM_REQ_NO_INIT "%1: ERREUR-  Licence requise avant l'initialisation du gestionnaire de proc‚dure.  \n\
(IFOR_PM_NO_INIT)\n"
$
$ --- For return value:  IFOR_PM_FATAL
PM_REQ_FATAL "%1: ERREUR-  La demande de licence … partir du gestionnaire de rŠglement … ‚chou‚e.  (IFOR_PM_FATAL)\n"
$
$ Error messages for error returns from 'ifor_pm_release()'
$
$ --- For return value:  IFOR_PM_BAD_PARAM
PM_RELEASE_BAD_PARAM "%1: ATTENTION-  ParamŠtre incorrect pour ifor_pm_release.  (IFOR_PM_BAD_PARAM)\n"
$
$ --- For return value:  IFOR_PM_FATAL
PM_RELEASE_FATAL "%1: WARNING-  L'emission de la licence a ‚chou‚.  (IFOR_PM_FATAL)\n"
$
$	- 	xdosopt
XDOSOPT_TITLE "Options DOS"
XDOSOPT_START_BUTTON "D‚but"
XDOSOPT_SAVE_BUTTON "Appliquer"
XDOSOPT_DEFAULT_BUTTON "Valeurs par d‚faut"
XDOSOPT_HELP_BUTTON "Aide"
XDOSOPT_CANCEL_BUTTON "Annuler"
XDOSOPT_VIDEO_TITLE "Vid‚o"
XDOSOPT_VGA_LABEL "VGA"
XDOSOPT_CGA_LABEL "CGA"
XDOSOPT_MDA_LABEL "MDA"
XDOSOPT_HERC_LABEL "Hercules"
XDOSOPT_COM_TITLE "Ports COM"
XDOSOPT_COM1_LABEL "COM1"
XDOSOPT_COM2_LABEL "COM2"
XDOSOPT_EMS_TITLE "SystŠme de messagerie ‚lectronique"
XDOSOPT_DRIVES_TITLE "Unit‚s"
XDOSOPT_DEFAULT_DRIVE "Aucun"
XDOSOPT_LPT_TITLE "Ports LPT"
XDOSOPT_LPT_NAME "LPT1"
XDOSOPT_SPOOL_LP_NAME   "UNIX (Spoul‚)"
XDOSOPT_DIRECT_LP0_NAME "DOS (Direct) : lp0"
XDOSOPT_DIRECT_LP1_NAME "DOS (Direct) : lp1"
XDOSOPT_DIRECT_LP2_NAME "DOS (Direct) : lp2"
XDOSOPT_STATUS_DONE_MSG "Termin‚"
XDOSOPT_QUIT_MSG "Vous n'avez pas appliqu‚ vos modifications.\nEtes-vous s–r de vouloir quitter?"
XDOSOPT_YES_BUTTON "Oui"
XDOSOPT_NO_BUTTON "Non"
XDOSOPT_OK_BUTTON "OK"
XDOSOPT_SAVE_ERR_MSG "Vous n'ˆtes pas autoris‚ …\nsauvegarder les changements apport‚s … ce fichier."
XDOSOPT_MEM_TITLE "M‚moire"
XDOSOPT_XMEM_NAME "Standard"
XDOSOPT_EMEM_NAME "SystŠme de messagerie ‚lectronique"
XDOSOPT_CONFIG_READ_MSG "Impossible de lire la configuration DOS ou Windows."
XDOSOPT_MEMORY_MSG	"M‚moire insuffisante."
XDOSOPT_INTERNAL_ERROR	"Erreur interne."
$	- 	x_msg
XMSG_DOSERRTITLE "Erreur DOS"
XMSG_OKTITLE "OK"
$	- 	xcrt
XCRT_DOS_TITLE	"DOS"
XCRT_WINDOWS_TITLE "Windows"
XCRT_FILE	"Fichier"
XCRT_FILE_M	"F"
XCRT_ZOOM	"Agrandir"
XCRT_ZOOM_M	"A"
XCRT_REFRESH	"RafraŒchir"
XCRT_REFRESH_M	"R"
XCRT_EXIT	"Quitter"
XCRT_EXIT_M	"Q"
XCRT_OPTIONS	"Options"
XCRT_OPTIONS_M	"O"
XCRT_FOCUS	"Actions souris seront transmises … DOS"
XCRT_FOCUS_M	"A"
XCRT_FONTS	"Polices DOS"
XCRT_FONTS_M	"D"
XCRT_KEYS	"Touches d'acc‚l‚ration"
XCRT_KEYS_M	"T"
XCRT_TUNE	"Mettre au point..."
XCRT_TUNE_M	"M"
XCRT_AUTO	"Automatique"
XCRT_AUTO_M	"A"
XCRT_SMALL	"Petit"
XCRT_SMALL_M	"P"
XCRT_MEDIUM	"Moyen"
XCRT_MEDIUM_M	"y"
XCRT_TO_DOS	"Passage sous Dos/Windows"
XCRT_TO_DOS_M	"W"
XCRT_TO_X	"Passage dans Bureau X"
XCRT_TO_X_M	"X"
XCRT_OK		"OK"
XCRT_CANCEL	"Annuler"
XCRT_HELP	"Aide"
XCRT_HELP_M	"A"
XCRT_ON_WINDOW	"Sur la fenˆtre"
XCRT_ON_WINDOW_M "f"
XCRT_ON_KEYS	"Sur les touches"
XCRT_ON_KEYS_M	"t"
XCRT_INDEX	"Index"
XCRT_INDEX_M	"I"
XCRT_ON_VERSION "Sur la version"
XCRT_ON_VERSION_M "v"
XCRT_TUNE_TITLE		"Options"
XCRT_TUNE_COLORMAP	"Carte couleur"
XCRT_TUNE_AUTOZOOM	"Zoom automatique"
XCRT_TUNE_DEFAULTS	"Param‚trages d'usine"
XCRT_CLIPBOARD	"Mise … jour du presse-papiers X..."
XCRT_CLIPBOARD_DONE	"Mise … jour du presse-papiers X...termin‚e."
XCRT_VERSION		"Version"
XCRT_VERSION_TEXT	"DOS Merge\n%1\nCopyright %2\nLocus Computing Corporation"
XCRT_QUIT_MSG "Quitter DOS?"
XCRT_NOHELP_MSG "Pas d'aide disponible."
XCRT_YES_BUTTON_TEXT "Oui"
XCRT_NO_BUTTON_TEXT "Non"
XCRT_OK_BUTTON_TEXT "OK"
XCRT_NODEV_ERR "Erreur : le p‚riph‚rique %1 n'est pas disponible."
XCRT_INTR_ERR "Erreur : erreur interne Merge."
XCRT_VT_ERR "Erreur : impossible d'obtenir un nouvel ‚cran."
XCRT_SERVER_ERR "Erreur : le serveur ne va pas lib‚rer l'‚cran"
XCRT_NOMSE_ERR "Session DOS non configur‚e pour une souris."
XCRT_TMPL1 "%1 est agrandi sur l'‚cran vt%2."
XCRT_TMPL2 "Appuyez sur %1 pour appeler le menu dos."
XCRT_MSG1 "Vous ne pouvez utiliser les graphiques EGA/VGA dans une fenˆtre."
XCRT_MSG2 "Quittez la fenˆtre dos et relancez dos."
XCRT_MSG3 "S‚lectionnez Agrandir dans le menu Fenˆtre pour utiliser des graphiques."
XCRT_MSG4 "Impossible de poursuivre l'agrandissement."
XCRT_MSG5 "La fonction de mise au point ne fonctionne pas."
XCRT_VERSION_ERROR "Votre pilote d'affichage DOS Merge Windows/X n‚cessite\n\
une mise … jour.  Consultez les indications relatives\n\
… la lib‚ration, votre administrateur, ou suivez la\n\
proc‚dure ci-aprŠs : \n\
1) d‚marrez DOS \n\
2) tapez\n\
\   WINXCOPY <unit‚>:<windows\\r‚p systŠme>\n\
\   exemple : WINXCOPY D:\\WINDOWS\\SYSTEM"
XCRT_PICTURE_ERROR "Impossible d'afficher l'image. Utiliser une r‚solution d'au moins 800x600\net un minimum de 256 couleurs."
$	- 	xcrt - Old GUI Messages
XCRT_MENU_TITLE "Menu DOS"
XCRT_UNFOCUS_LABEL "D‚centrer"
XCRT_X_COLORS_LABEL "Couleurs X"
XCRT_DOS_COLORS_LABEL "Couleurs DOS"
XCRT_FREEZE_LABEL "Blocage automatique activ‚"
XCRT_UNFREEZE_LABEL "Blocage automatique d‚sactiv‚"
XCRT_UNZOOM_LABEL "R‚duire"
XCRT_NORMAL_KEYS_LABEL "Touches DOS"
XCRT_WINDOW_KEYS_LABEL "Touches du Bureau"
XCRT_OLD_MSG3 "Cliquez sur Agrandir dans le menu dos pour utiliser des graphiques."
$	- 	xmrgconfig
GUI_OK			"OK"
GUI_CANCEL		"Annuler"
GUI_HELP		"Aide"
GUI_YES			"Oui"
GUI_NO			"Non"
GUI_DELETE		"Supprimer"
GUI_MODIFY		"Modifier"
GUI_TITLE 		"DOS Merge"
GUI_AUTOMATIC_LABEL	"Automatique"
GUI_CONFIG_TITLE	"Configuration de DOS et de Windows"
GUI_NONE_LABEL		"Aucun"
GUI_HOME_LABEL		"Origine"
GUI_DEVICES_BUTTON_LABEL "P‚riph‚riques"
GUI_OPTIONS_BUTTON_LABEL "Options"
GUI_SAVE_LABEL		"Sauvegarder"
GUI_SAVE_AS_LABEL	"Sauvegarder sous..."
GUI_START_LABEL		"D‚but"
GUI_DRIVES_LABEL	"Unit‚s"
GUI_CONFIGURE_LABEL	"Configurer"
GUI_DOS_DRIVE_LABEL	"Unit‚ DOS..."
GUI_UNIX_FILESYS_LABEL	"SystŠme de fichiers UNIX..."
GUI_COM_PORTS_LABEL	"Ports COM"
GUI_COM1_LABEL		"COM1"
GUI_COM2_LABEL		"COM2"
GUI_LPT_PORTS_LABEL	"Ports LPT"
GUI_LPT1_LABEL		"LPT1"
GUI_LPT2_LABEL		"LPT2"
GUI_LPT3_LABEL		"LPT3"
GUI_TIMEOUT_LABEL	"D‚passement de temps"
GUI_DEVICES_LABEL	"P‚riph‚riques"
GUI_DEVICES_STATUS_MSG	"%1 et %2 sont en conflit\n."
GUI_MULTI_CONFLICT_MSG  "De multiples conflits \nentre p‚riph‚riques ont ‚t‚ d‚tect‚s."
GUI_MEMORY_LABEL	"M‚moire"
GUI_STANDARD_LABEL	"Standard"
GUI_EMS_LABEL		"SystŠme de messagerie ‚lectronique"
GUI_AUTOEXEC_LABEL	"AUTOEXEC.BAT"
GUI_RUN_SYS_LABEL	"Ex‚cuter … l'‚chelle du systŠme"
GUI_PERSONAL_LABEL	"Ex‚cuter personnel"
GUI_OTHER_LABEL		"Autre"
GUI_BROWSE_LABEL	"Consulter..."
GUI_EDIT_LABEL		"Modifier fichiers"
GUI_AUTOEXEC_MNEMONIC	"A"
GUI_SYS_AUTO_LABEL	"AUTOEXEC.BAT … l'‚chelle du systŠme..."
GUI_PERSONAL_AUTO_LABEL "AUTOEXEC.BAT personnel..."
GUI_OTHER_AUTO_LABEL	"Autre AUTOEXEC.BAT..."
GUI_CONFIG_LABEL	"CONFIG.SYS"
GUI_CONFIG_MNEMONIC	"C"
GUI_SYS_CONFIG_LABEL	"CONFIG.SYS … l'‚chelle du systŠme..."
GUI_PERSONAL_CONFIG_LABEL "CONFIG.SYS personnel..."
GUI_OTHER_CONFIG_LABEL	"Autre CONFIG.SYS"
GUI_WINDOWS_SIZE_LABEL	"Taille des fenˆtres"
GUI_CUSTOM_LABEL	"Personnaliser :"
GUI_RESIZE_LABEL	"Redimensionnement manuel..."
GUI_WIDTH_LABEL		"Largeur :"
GUI_HEIGHT_LABEL	"Hauteur :"
GUI_DOS_SIZE_LABEL	"Taille DOS"
GUI_DOS_FONT_LABEL	"Police DOS"
GUI_SCALE_DOS_LABEL	"Changement d'‚chelle des graphiques DOS"
GUI_DISPLAY_TYPE_LABEL	"Type d'affichage"
GUI_SMALL_LABEL		"Petit"
GUI_MEDIUM_LABEL	"Moyen"
GUI_LARGE_LABEL		"Grand"
GUI_X1_LABEL		"x1"
GUI_X2_LABEL		"x2"
GUI_MDA_LABEL		"MDA"
GUI_HERCULES_LABEL	"Hercules"
GUI_CGA_LABEL		"CGA"
GUI_VGA_LABEL           "VGA"
GUI_OPTIONS_LABEL	"Options"
GUI_AUTOZOOM_LABEL	"Zoom automatique"
GUI_COLORMAP_LABEL	"Carte couleur"
GUI_ACCEL_KEYS_LABEL	"Touches acc‚l‚ratrices pour X"
GUI_FACTORY_LABEL	"Param‚trages d'usine"
GUI_COMMAND_LABEL	"Commande"
GUI_DOS_DRIVE_TITLE	"Unit‚ DOS"
GUI_READ_ONLY_LABEL	"Lecture seulement"
GUI_EXCLUSIVE_LABEL	"Exclusif"
GUI_FILES_LABEL	 	"Fichiers"
GUI_FILTER_LABEL	"Filtre"
GUI_DIRECTORIES_LABEL	"R‚pertoires"
GUI_SELECTION_LABEL	"S‚lection"
GUI_RESIZE_TITLE	"Taille des fenˆtres Windows"
GUI_FILE_LABEL		"Fichier"
GUI_RESIZE_MESSAGE	"Utilisez votre souris ou votre clavier pour redonner … cette fenˆtre la taille Microsoft Windows que vous pr‚f‚rez."
GUI_STATUS_MSG		"Ligne du statut"
GUI_CONFIRM_MSG		"Vous avez apport‚ des modifications … la configuration.\n_tes-vous s–r de vouloir quitter ?"
GUI_FILE_DOESNT_EXIST_MSG "Le fichier n'existe pas."
GUI_DIR_DOESNT_EXIST	"Ce r‚pertoire n'existe pas."
GUI_FILE_IS_DIR_MSG	"Le fichier s‚lectionn‚ est un r‚pertoire, il est impossible de l'ouvrir pour le modifier."
GUI_PERMISSION_DENIED	"Autorisation refus‚e."
GUI_NOT_DIR		"La s‚lection sp‚cifi‚e n'est pas un r‚pertoire."
GUI_EDIT_ERROR		"Erreur de modification du fichier"
GUI_OPEN_CONFIG_ERROR	"Erreur lors de l'ouverture du fichier de configuration."
GUI_CONFIG_DOESNT_EXIST "La configuration \"%1\" n'existe pas."
GUI_INTERNAL_ERROR	"Erreur interne"
GUI_MEMORY_ERROR	"M‚moire insuffisante."
GUI_WRITE_CONFIG_ERROR	"Impossible de sauvegarder le fichier de configuration : \n%1"
GUI_DELETE_CONFIG_ERROR	"Impossible de supprimer le fichier de configuration."
GUI_ALREADY_EXISTS	"La configuration de \"%1\" existe d‚j…."
GUI_CONFLICT_MSG	"Des conflits mat‚riels opposent les p‚riph‚riques suivants : \n%1. Pour r‚soudre ces conflits, d‚s‚lectionnez au moins un de\n ces objets dans la zone \"P‚riph‚riques\". Si vous \nsouhaitez utiliser ces p‚riph‚riques conjointement, il vous \nfaudra en reconfigurer au moins un pour pr‚venir un conflit mat‚riel."
GUI_DRIVE_WARNING	"Avertissement : \"%1\" n'existe pas."
GUI_CANT_START		"Impossible de d‚marrer la session DOS ou Windows."
GUI_NO_DRIVES_ERROR	"Aucune unit‚ DOS disponible."
GUI_CANT_READ_FILE	"Impossible de lire le fichier \"%1\"."
GUI_CANT_CREATE_FILE	"Impossible de cr‚er le fichier \"%1\"."
GUI_VIEW_FILE_MSG	"Impossible d'‚crire le fichier \"%1\". Voulez-vous le consulter ?"
GUI_CREATE_FILE_MSG	"Le fichier \"%1\" n'existe pas. Voulez-vous le cr‚er ?"
$	-	mrgadmin messages
ADM_USAGE_CMD "Syntaxe : mrgadmin class add jeton:fichier:nom complet\n          mrgadmin class delete jeton\n          mrgadmin class list [jeton]\n          mrgadmin class update jeton:fichier:nom complet\n          mrgadmin class printdef jeton\n"
ADM_CLASS_MSG "Erreur : les classes autoris‚es sont : \n"
ADM_NO_CLASS_MSG "Impossible d'obtenir les informations relatives … la classe \"%1\".\n"
ADM_NO_CLASS_ENTRIES_MSG "Pas d'entr‚e d‚tect‚e pour la classe \"%1\".\n"
ADM_BAD_TOKEN_MSG "Impossible d'obtenir les informations relatives au jeton \"%1\".\n"
ADM_PERMISSION_MSG "Vous devez ˆtre l'utilisateur racine pour modifier l'administration de Merge.\n"
ADM_CANT_DELETE_TOKEN_MSG "Impossible de supprimer le jeton \"%1\"\n"
ADM_BAD_TOKEN_DEF_MSG "La d‚finition du jeton doit se pr‚senter sous la forme \"token:file:name\".\n"
ADM_CANT_READ_MSG	"Impossible de lire \"%1\" : %2.\n"
$	-	mrgadmin commands
ADM_ADD_STR		"add"
ADM_DELETE_STR		"delete"
ADM_LIST_STR		"list"
ADM_UPDATE_STR		"update"
ADM_PRINTDEF_STR	"printdef"
$	-	admin library messages
ADM_NO_ERROR "Erreur interne - pas d'erreur."
ADM_INTERNAL_ERROR "Erreur interne dans la bibliothŠque admin."
ADM_PARSE_VARIABLE "Nom de variable non reconnu."
ADM_PARSE_NO_NUMBER "Vous devez sp‚cifier un num‚ro."
ADM_PARSE_NUMBER "Valeur num‚rique non autoris‚e."
ADM_PARSE_ILLEGAL_VARIABLE "Cette variable n'est pas autoris‚e pour le type d'attache sp‚cifi‚."
ADM_PARSE_MAX_NUMBERS "Nombre maximal d'objets autoris‚s d‚pass‚."
ADM_MEMORY "M‚moire insuffisante."
ADM_PARSE_MAX_NUMBER "Valeur num‚rique maximale d‚pass‚e."
ADM_PARSE_RANGE "Sp‚cification de registre incorrecte."
ADM_BAD_ATTACH "Sp‚cification du type d'attache incorrecte."
ADM_BAD_DEV_TYPE "Sp‚cification du type de p‚riph‚rique incorrecte."
ADM_BAD_FAILURE_ACTION "Sp‚cification de l'op‚ration … effectuer en cas d'incident incorrecte."
ADM_BAD_USER_ACCESS "Sp‚cification de l'accŠs utilisateur incorrecte."
ADM_BAD_VPI_PPI_OPTION "Option VPI/PPI incorrecte."
ADM_BAD_DRIVE_OPTION "Option d'unit‚ incorrecte."
ADM_MISSING_VPI_DEV "Il manque la sp‚cification requise concernant le p‚riph‚rique vpi."
ADM_MISSING_PPI_DEV "Il manque la sp‚cification requise concernant le p‚riph‚rique ppi."
ADM_MISSING_DRIVE_NAME "Il manque la sp‚cification requise concernant le nom de l'unit‚."
ADM_MISSING_PRINTER_CMD "Il manque la sp‚cification requise concernant la commande printer."
ADM_IRQ_MISMATCH "Non-concordance entre irq et physicalirq."
ADM_DMA_MISMATCH "Non-concordance entre dma et physicaldma."
ADM_IOPORTS_MISMATCH "Non-concordance entre ioports et physicalioports."
ADM_MEM_MISMATCH "Non-concordance entre memmappedio et physicalmemmappedio."
ADM_SYSERR "Erreur systŠme."
ADM_CLASS_NOT_FOUND "Classe introuvable"
ADM_TOKEN_NOT_FOUND "Jeton introuvable."
ADM_LOCK "Impossible de verrouiller le fichier des jetons."
ADM_TOKEN_EXISTS "Le jeton existe d‚j…."
ADM_BAD_TOKEN "Les noms de jetons ne peuvent ˆtre compos‚s que des caractŠres A-Z, a-z, 0-9 et \"-\"."
ADM_NO_ATTACH "Il manque la sp‚cification requise concernant le type d'attache."
ADM_PARSE_OPTION "Valeurs de l'option non reconnues"
ADM_BAD_UMB "Sp‚cification UMB non autoris‚e. Elle doit faire partie du registre 0xA0000-0xFFFF."
$       -       messages admconvert
ADMCVT_CANT_OPEN "Impossible d'ouvrir \"%1\".\n"
ADMCVT_CANT_CREATE "Impossible de cr‚er \"%1\".\n"
ADMCVT_BAD_FORMAT "Erreur de format dans /etc/dosdev.\n"
ADMCVT_MEMORY "M‚moire insuffisante.\n"
ADMCVT_CANT_ADD "Impossible d'ajouter le jeton \"%1\" : %2\n"
ADMCVT_LP_CONVERT "Impossible de convertir /usr/lib/merge/lp.\n"
ADMCVT_DOS_DRIVE_NAME "Natif DOS %1 :"
ADMCVT_COM1_NAME "IRQ 4 DOS Direct"
ADMCVT_COM2_NAME "IRQ 3 DOS Direct"
ADMCVT_PORT1_NAME "Port 1 DOS Direct"
ADMCVT_PORT2_NAME "Port 2 DOS Direct"
$	-	Configuration library errors
CFG_NO_ERROR		"Erreur interne - pas d'erreur."
CFG_PARSE_INTERNAL_ERROR "Erreur interne dans l'analyse syntaxique."
CFG_PARSE_VARIABLE	"Nom de variable non reconnu."
CFG_PARSE_ACCEL		"Valeur associ‚e aux touches acc‚l‚ratrices non reconnue. Utilisez \"dos\" ou \"x\"."
CFG_PARSE_BOOLEAN	"Valeur bool‚enne non reconnue. Utilisez \"vrai\" or \"faux\"."
CFG_PARSE_DISPLAY	"Valeur du type d'affichage non reconnue. Utilisez \"auto\", \"vga\", \"cga\", \"mono\" ou \"hercules\"."
CFG_PARSE_DRIVE_DEF	"D‚finition de l'unit‚ non reconnue. Utilisez un jeton d'unit‚ ou \"none\"."
CFG_PARSE_DRIVE_OPTION	"Option d'unit‚ non reconnue. Utilisez \"readonly\" ou \"exclusive\"."
CFG_PARSE_DRIVE_LETTER	"Lettre non autoris‚e pour l'unit‚. Seules les lettres C … J sont autoris‚es."
CFG_PARSE_NUMBER	"Valeur num‚rique non autoris‚e."
CFG_PARSE_SCALE_VALUE	"Valeur des graphiques scaledos non reconnue. Utilisez \"auto\", \"1\"," ou \"2\"."
CFG_PARSE_NO_NUMBER	"Vous devez sp‚cifier un num‚ro."
CFG_PARSE_OPTION	"Valeurs de l'option non reconnues"
CFG_PARSE_FONT		"Valeur de police non reconnue. Utilisez \"auto\", \"small\" ou \"medium\"."
CFG_UNZOOM_KEY		"Vous devez sp‚cifier un nom de code X."
OPTCVT_BAD_USER		"Impossible d'obtenir les informations relatives … l'utilisateur actuel."
OPTCVT_BAD_USER2	"Impossible d'obtenir les informations relatives … l'utilisateur %1.\n"
OPTCVT_NOT_DIR		"Le r‚pertoire de configuration existe d‚j…."
OPTCVT_MKDIR_FAILED	"Impossible de cr‚er le r‚pertoire de configuration."
OPTCVT_WRITE_FAILED	"Impossible d'‚crire dans les fichiers de configuration."
OPTCVT_UNLINK		"OPtconvert n'a pas pu supprimer les anciens fichiers de configuration."
OPTCVT_ADM_ERROR	"Erreur d'administration au cours de la conversion."
OPTCVT_BAD_OPTIONS	"Syntaxe : optconvert [-c utilisateur]\n"
OPTCVT_NO_CONVERT	"La configuration existe d‚j… pour l'utilisateur %1.\n"
OPTCVT_BAD_LOG		"Impossible d'‚crire dans le fichier journal %1.\n"
OPTCVT_BAD_CONVERT	"Impossible de convertir +a%1.\n"
CFG_BAD_DIR		"Impossible d'ouvrir le r‚pertoire de configuration."
CFG_MEMORY		"M‚moire insuffisante."
CFG_SYSERR		"Erreur systŠme."
CFG_BAD_NAME		"Nom de configuration non autoris‚."
CFG_NOT_DEL	"Vous ne pouvez supprimer les configurations \"dos\" ou \"win\"."
CFG_HEADER1	"Ce fichier a ‚t‚ g‚n‚r‚ par des utilitaires de configuration Merge."
CFG_HEADER2	"Faites appel aux utilitaires de configuration pour modifier ce fichier."
$	-	mrgconfig
CFG_BAD_CMD	"Erreur interne - commande non reconnue.\n"
CFG_YES_RESP	"y"
CFG_NO_RESP	"n"
CFG_CONFIRM	"_tes-vous certain de vouloir supprimer \"%1\" ?"
CFG_RESPONSE	"Veuillez r‚pondre \"%1\" ou \"%2\"\n"
CFG_CANT_DELETE "Impossible de supprimer \"%1\".\n"
CFG_DELETED	"Configuration \"%1\" supprim‚e.\n"
CFG_NOT_DELETED	"Configuration \"%1\" non supprim‚e.\n"
CFG_ADD_USAGE_CMD "Syntaxe : addmrgconfig ancienne-config [copyto] nouvelle-config.\n"
CFG_DEL_USAGE_CMD "Syntaxe : delmrgconfig config.\n"
CFG_LIST_USAGE_CMD "Utilisation : listmrgconfig\n"
CFG_DEL_OPT_ERROR "Vous ne pouvez supprimer que les options customdev.\n"
CFG_SET_OPT_ERROR "Vous ne pouvez sp‚cifier qu'une seule option.\n"
CFG_USAGE_CMD	"Syntaxe : mrgconfig config list option[,option...].\n          mrgconfig config set option\n          mrgconfig config delete option\n"
CFG_COPYTO_STR	"copyto"
CFG_LIST_STR	"list"
CFG_SET_STR	"set"
CFG_DELETE_STR	"delete"
CFG_ALREADY_EXISTS "La configuration \"%1\" existe d‚j….\n"
CFG_BAD_OPTION	"Option \"%1\" non reconnue.\n"
CFG_WRITE_ERROR	"Erreur interne - l'op‚ration d'‚criture a ‚chou‚.\n"
CFG_DEL_VALUE_ERROR "Vous devez sp‚cifier une valeur customdev.\n"
CFG_DEL_NOT_FOUND "Le p‚riph‚rique personnalis‚ \"%1\" n'existe pas dans la configuration \"%2\".\n"
CFG_READ_ERROR  "Erreur interne - impossible de lire la configuration.\n"
CFG_LIST_ERROR	"Impossible d'obtenir une liste de configurations.\n"
CFG_NOLIST_MSG	"Aucune configuration … lister.\n"
$ SCCSID(@(#)install.msg	1.23 LCC) Modified 16:45:28 9/7/94
$ Messages for the install and remove scripts.
$domain LCC.MERGE.UNIX.INSTALL
$quote "

GEN_ERR_1 "Impossible d'achever l'installation en raison d'une erreur.\n"
GEN_ERR_2 "Recherche de r‚sultats erron‚s dans %1.\n"
GEN_ERR_3 "Elimination des erreurs et sortie du programme...\n"
GEN_ERR_4 "Erreur survenue lors de la suppression.\n"
GEN_ERR_5 "Processus en cours...\n"
GEN_ERR_6 "Pas de reconstitution du noyau.\n"

BUILD_FAIL "La constitution du noyau a ‚chou‚.\n"
LU_FAIL   "Echec de link_unix\n"
IDB_FAIL  "Echec de idbuild\n"

REPL_ERR  "Impossible de remplacer %1 actuellement install‚.\n\
Supprimez %2 avant de proc‚der … la r‚installation.\n"

I_WARN_1   "%1 est d‚j… install‚ sur le noyau en cours d'ex‚cution.\n"
I_ERR_1   "Impossible d'installer %1 sur ce systŠme.\n"
I_ERR_2   "Le Paquet de base Unix doit inclure le\n\
support %1 ; sinon, vous devez d'abord\n\
installer la nouvelle version de ce paquet.\n"

KERNEL_HAS_MERGE "\n\
%1 est d‚j… install‚ sur l'‚cho du noyau UNIX en cours d'ex‚cution.\n\
%2 doit ˆtre supprim‚ complŠtement avant la r‚installation ; \n\
sinon l'installation risque d'‚chouer et le noyau ne sera pas li‚ correctement.\n"

NO_STREAMS "\n\
Pilote de flux non install‚. Vous devez installer ce pilote avant de pouvoir\n\
installer %1.\n"

CANT_INST  "Impossible d'installer %1.\n"
CANNOT_REMOVE "Impossible de supprimer %1.\n"
NOSPACE    "\       Espace insuffisant pour reconstituer le noyau.\n"

INSTALL_1 "Installation des fichiers %1. Veuillez patienter...\n"
INSTALL_2 "Configuration des fichiers %1. Veuillez patienter...\n"

REMOVE_3 "Suppression des fichiers %1. Veuillez patienter...\n"
REMOVE_4 "Les fichiers de distribution %1 ont ‚t‚ d‚sinstall‚s.\n"

IDIN_FAIL "idinstall %1 a ‚chou‚.\n" \
ALREADY_INSTALLED "Avertissement : le pilote %1 est d‚j… install‚.\n"
IDCHK_FAIL  "idcheck -p de %1 a g‚n‚r‚ %2\n"

LINK_FAIL_1 "Le fichier n'a pas pu ˆtre li‚.\n"
LINK_FAIL_2 "La cr‚ation d'un lien a ‚chou‚ : %1 %2\n"
CPIO_CP_FAIL "La copie des fichiers au moyen de cpio a ‚chou‚.\n"
IDTUNE_FAIL "idtune a ‚chou‚\n"

$ The following five are used in install and remove when the user is
$ prompted if a re-link of the kernel is wanted.  YESES and NOES are
$ each a list of the acceptable single character responses for yes and no.
YESES "oO"
NOES "nN"
LINK_NOW_Q "Souhaitez-vous cr‚er un noyau maintenant ? (o/n) "
Y_OR_NO "Veuillez r‚pondre o ou n : "
REBUILD_WARN "Pour que %1 fonctionne correctement, vous devez reconstituer le noyau.\n"
REBUILD_NEED "Pour que le systŠme fonctionne correctement, vous devez reconstituer le noyau.\n"

$ This section (to the line with END-PKGADD) is PKGADD/SVR4 specific.

INST_LOG_HDR "\
# Ce fichier-journal a ‚t‚ cr‚‚ par le processus d'installation %1.\n\
# Toute erreur survenue lors de la phase contr“l‚e %2 de l'installation\n\
# apparaŒtra dans ce fichier-journal. Ce dernier est cr‚‚ lors de\n\
# l'installation, et peut ˆtre supprim‚ par l'utilisateur. Outre des\n\
# erreurs, ce fichier peut aussi contenir des donn‚es g‚n‚r‚es par\n\
# d'autres programmes UNIX (standard).\n"

REM_LOG_HDR "\
# Ce fichier-journal a ‚t‚ g‚n‚r‚ par le processus de suppression\n\
# %1. Toute erreur survenue lors de la phase contr“l‚e %2 de la\n\
# suppression apparaŒtra dans ce fichier-journal. Ce dernier est cr‚‚\n\
# lors de la suppression, et peut ˆtre supprim‚ par l'utilisateur.\n\
# Outre des erreurs, ce fichier peut ‚galement contenir des donn‚es\n\
# g‚n‚r‚es par d'autres programmes UNIX (standard).\n"

INST_NO_VERS "\
ERROR : Impossible de trouver le fichier de versions\n\
        Installation de Merge impossible\n"

PO_ERR_MSG1 "Impossible de cr‚er une entr‚e imprimante (%1).\n"

INST_USER_STARTUP "Mise … jour des fichiers de d‚marrage utilisateur UNIX...\n"
INST_PRINTER "Installation de l'interface de l'imprimante DOS...\n"
INST_CONFIG_SYS "Mise … jour de l'int‚gralit‚ de config.sys ...\n"
INST_AUTOEXEC "Mise … jour de l'int‚gralit‚ d'autoexec.bat ...\n"
INST_DRV_MSG "Installation des pilotes %1...\n"
INST_MOD_MSG "\tmodule : %1\n"
INST_MERGE "Mise sous tension du pilote MERGE...\n"
INST_DOSX "Activation du pilote DOSX...\n"
INST_SEG_WRAP "Activation du pilote SEG WRAP...\n"
INST_DONE "Installation de %1 termin‚e.\n"
INST_CANCEL "Installation annul‚e par l'utilisateur.\n"
INST_DRV_SH "Erreur d'installation des pilotes Merge.\n"
INST_MRG_SH "Incidents lors de l'installation des fichiers Merge.\n"
INST_X_SH "Incidents lors de l'installation des fichiers Merge X.\n"

REM_USER_STARTUP "Mise … jour des fichiers de d‚marrage utilisateur UNIX...\n"
REM_DOS_FILES "Suppression des fichiers DOS (de dosinstall)...\n"
REM_PRINTER "Suppression de l'interface de l'imprimante DOS...\n"
REM_PRINTER_NOT "Pas de suppression de l'interface de l'imprimante DOS.\n"
MISSING_PROG "Programme %1 manquant.\n"
REM_CONFIG_SYS "Mise … jour de l'int‚gralit‚ de config.sys ...\n"
REM_AUTOEXEC "Mise … jour de l'int‚gralit‚ d'autoexec.bat ...\n"
REM_DRIVERS "Suppression de %1 pilotes...\n"
REM_MERGE "Mise hors tension du pilote MERGE...\n"
REM_DOSX "Mise hors tension du pilote DOSX...\n"
REM_SEG_WRAP "Mise hors tension du pilote SEG WRAP...\n"
REM_DONE "La suppression de %1 est termin‚e.\n"
REM_CANCEL "Suppression annul‚e par l'utilisateur.\n"

REM_PROB_MODLIST "Impossible de trouver %1/modlist, aucun module n'a ‚t‚ supprim‚.\n"
REM_PROB_MODULE "Impossible de supprimer %1.\n"

# request script
REQ_PREV_INSTALL "\n\
$MERGENAME est d‚j… install‚.\n\
Il doit ˆtre supprim‚ avant de pouvoir ˆtre r‚install‚.\n\n"

$ END-PKGADD
$ SCCSID(@(#)scripts.msg	1.2 LCC) Merge 3.2.0 Branch Modified 12:38:24 10/14/94
$ SCCSID(@(#)scripts.msg	1.40 LCC) Modified 23:55:19 10/10/94
$ English message file for the Merge scripts.
$quote "

$domain LCC.MERGE.UNIX.SET_NLS
$ Messages for the "set_nls" script.
WARN_PREFIX "AVERTISSEMENT : "
ERR_PREFIX "ERREUR : "

$domain LCC.MERGE.UNIX.MKVFLP
$ Messages for the "mkvfloppy" script.
USAGE "SYNTAXE : %1 chemin_d'accŠs_complet-nom_de_fichier [-s]\n"

$domain LCC.MERGE.UNIX.DOSBOOT
$ Messages for the "dosboot" script.
ERR1 "ERREUR : impossible d'utiliser l'option \"+l\" avec %1.\n"

$domain LCC.MERGE.UNIX.INITDPART
$ Messages for the "initdospart" script.

INSERT "Ins‚rez une disquette haute densit‚ dans l'unit‚ A: puis appuyez sur <ENTREE> : "
FORMAT "Formatez la partition DOS.\n"
ERR_1  "ERREUR : pas de partition DOS.\n"
ERR_2  "ERREUR : seul l'utilisateur racine est autoris‚ … formater la partition DOS.\n"
ERR_3  "ERREUR : impossible de formater la disquette.\n\
\n\
V‚rifiez qu'elle n'est pas prot‚g‚e en ‚criture.\n"

ERR_4  "ERREUR : disquette apparemment d‚fectueuse.\n"

TEXT_1 "Pour utiliser la partition DOS, vous devez pr‚alablement la formater.\n\
\n\
Si une partition DOS a d‚j… ‚t‚\n\
format‚e, il est inutile de renouveler l'op‚ration, … moins que la\n\
taille ou l'emplacement de cette partition DOS n'ait ‚t‚ modifi‚ depuis le formatage.\n\
\n\
Avertissement : si vous formatez la partition DOS, toutes les donn‚es\n\
\                de cette derniŠre seront effac‚es.\n\
Avertissement : pour formater une partition DOS, vous devez d'abord\n\
\                arrˆter le systŠme, puis le red‚marrer … partir d'une\n\
\               disquette.\n\
\n\
Vous devez utiliser une disquette haute densit‚. V‚rifiez\n\
qu'elle n'est pas prot‚g‚e en ‚criture. La disquette\n\
sera format‚e et deviendra une disquette d‚marrable.\n"

$ NOTE: the CONTINUE, YESES, NOES and Y_OR_N string are closely related.
$ The CONTINUE and Y_OR_N are prompts for a yes or no answer.
$ They don't have newline chars, so the cursor will stay at the end
$ of the printed line.
$ The YESSES and NOES strings define the recognized single character
$ yes or no answers.
$ The YESES  string is a list of single characters that mean "yes".
$ The NOES  string is a list of single characters that mean "no".
CONTINUE "Souhaitez-vous continuer (o/n)? "
Y_OR_N "Veuillez r‚pondre o ou n : "
YESES "oO"
NOES  "nN"

RETRY "V‚rifiez que vous utilisez une disquette haute densit‚ non d‚fectueuse.\n\
\n\
Souhaitez-vous renouveler l'op‚ration (o/n) ? "

REBOOT "\n\
Pour formater la partition DOS, vous devez d‚marrer le\n\
systŠme … partir de la disquette ins‚r‚e dans l'unit‚.\n\
\n\
Ne retirez EN AUCUN CAS la disquette\n\
\n\
Appuyez sur <ENTREE> pour arrˆter le systŠme avant de le red‚marrer. "

EXIT   "Pas de formatage de la partition DOS.\n"
EXIT_2 "Pour pouvoir utiliser la partition DOS, vous devez pr‚alablement la formater.\n"


$domain LCC.MERGE.UNIX.DOSINSTALL
$ Messages for the dosinstall and dosremove scripts.
$    (sourcefiles: unix/janus/dosinst.sh, unix/janus/dosremove.sh)
EXITING "Sortie...\n"
ERR_0 "Impossible d'installer DOS.\n"
ERR_1 "Seul l'utilisateur racine ou le superutilisateur est autoris‚ … installer DOS.\n"
ERR_2 "DOS est d‚j… install‚. Vous devez supprimer la version\n\
de DOS actuellement install‚e pour pouvoir proc‚der … la r‚installation.\n"
CANNOT_CONTINUE "Impossible de continuer.\n"
$ The BAD_FLOP_NAME error can happen when the user used the "DRIVE_DEV"
$ variable to specify which floppy device to use, and the device does
$ not exist.  If the user did not set DRIVE_DEV, then the internal
$ logic that determines the device filename did not work.  To work around
$ this problem, the use should set DRIVE_DEV to the device name to use.
BAD_FLOP_NAME "Nom du p‚riph‚rique de l'unit‚ de disquette '%1' erron‚\n"

RM_MSG "Suppression de DOS\n"
ERR_RM "ProblŠmes survenus lors de la suppression de DOS.\n\
L'installation de DOS risque d'‚chouer.\n"
RE_INST "R‚installation de DOS\n"
INSTALLING_DOS "Installation de DOS\n"
INSTALLING_DOSVER "Installation de %1\n"
DOS_INST_DONE "Installation de DOS termin‚e.\n"

DSK_DONE "Vous pouvez … pr‚sent retirer la disquette.\n"

DOSINST_MKIMG_FAILED "Tous les fichiers images DOS n'ont pas pu ˆtre cr‚‚s.\n\
\     Une partie de l'installation de DOS n'a pas pu ˆtre effectu‚e.\n"

DOSINST_MKIMG_MONO_FAILED "Impossible de cr‚er des fichiers images DOS monochromes.\n\
\     Abandon de l'installation de DOS.\n"

NO_SPACE    "     Espace insuffisant.\n"
NO_SPACE_IN "     Espace insuffisant dans %1\n"
NEED_SPACE  "     %1 blocs requis alors que %2 blocs sont disponibles.\n"

DOSINST_CANT "Impossible d'installer %1\n"

DOSINST_DOSBRAND_MENU1 "Choisissez la version de DOS … installer.\n"

DOSINST_DOSBRAND_MENU2 "\
\      0 : aucune des versions ci-dessus.  Abandonnez l'installation de DOS.\n\
\n\
Entrez le num‚ro de l'option, puis appuyez sur Entr‚e : "

DOSINST_01 "Entrez la valeur 0 ou 1\n"
DOSINST_012 "Entrez la valeur 0, 1 ou 2\n"
DOSINST_0123 "Entrez la valeur 0, 1, 2, ou 3\n"
DOSINST_01234 "Entrez la valeur 0, 1, 2, 3, ou 4\n"
DOSINST_012345 "Entrez la valeur 0, 1, 2, 3, 4, ou 5\n"
DOSINST_0123456 "Entrez la valeur 0, 1, 2, 3, 4, 5 ou 6\n"
DOSINST_01234567 "Veuillez entrer les valeurs 0, 1, 2, 3, 4, 5, 6 ou 7\n"

DOSINST_SCO_DISKSIZE "\
Veuillez entrer la taille des disques DOS dont vous disposez.\n\
\n\
\      1 : 3,5\" disquettes faible densit‚.\n\
\      2 : 3,5\" disquettes haute densit‚.\n\
\      3 : 3,5\" disquettes de densit‚ inconnue.\n\
\      4 : 5,25\" disquettes faible densit‚.\n\
\      5 : 5,25\" disquettes haute densit‚.\n\
\      6 : 5,25\" disquettes de densit‚ inconnue.\n\
\      0 : aucun des types ci-dessus. Abandon de l'installation de DOS.\n\
\n\
Tapez le num‚ro d'option correspondant, puis appuyez sur Entr‚e : "

DOSINST_FLOP_P "\
A partir de quelle unit‚ de disquette effectuez-vous l'installation?\n\
\n\
\      1 : la premiŠre unit‚.  (unit‚ '%1')\n\
\      2 : la deuxiŠme unit‚. (unit‚ '%2')\n\
\      0 : arrˆt. Abandon de l'installation de DOS.\n\
\n\
Entrez le num‚ro d'option correspondant, puis appuyez sur Entr‚e : "

DOSINST_MSG1 "Installation de %1 … partir de %2 disquettes.\n"

DOSINST_FROM "Installation … partir de l'unit‚ %1.\n"

WRONG_DISK_1 "\
\      Cette disquette semble incorrecte.\n\
\      V‚rifiez que vous avez plac‚ la disquette appropri‚e dans l'unit‚ qui convient.\n"

WRONG_DISK_N "\
\      Cette disquette a d‚j… ‚t‚ lue en tant que disquette nø%1.\n\
\      V‚rifiez que vous avez ins‚r‚ la disquette appropri‚e dans l'unit‚ qui convient.\n"

DOSINST_DW_P "\
Que souhaitez-vous faire … pr‚sent :\n\
\      0 : abandonner l'installation de DOS.\n\
\      1 : renouveler la tentative d'installation. \n\
Entrez 0 ou 1 et appuyez sur Entr‚e : "

DOSINST_DW_R "Entrez la valeur 0 ou 1\n"

DOSINST_NOTINST "Pas d'installation de DOS.\n"
DOSINST_AUTO_NA "\
Impossible d'installer DOS selon une proc‚dure automatique.\n\
Pour installer DOS … partir de vos disquettes DOS\nlancez le\n\
programme 'dosinstall'.\n"

CHECK_DISK  "V‚rification du disque."
INSERT_S "Ins‚rez la disquette DOS nø%1 dans l'unit‚ et appuyez sur ENTREE. "
INSERT_D "Ins‚rez la disquette DOS nø%1 dans l'unit‚ %2 et appuyez sur ENTREE. "
INSERT_NEXT "Remplacez la disquette nø%1 par la disquette nø%2, puis appuyez sur ENTREE. "
INST_CANCEL "Installation annul‚e par l'utilisateur.\n"
INST_SYS "Installation des fichiers systŠme DOS.\n"
READING_MESSAGE "Lecture de la disquette en cours."
CANNOT_CREATE "Erreur : impossible de cr‚er %1\n"
NO_FILE "Erreur : %1 manquant\n"
MISSING_DIR "Erreur : r‚pertoire %1 manquant\n"
CREATING_DIR "Attention : R‚pertoire manquant %1\n\
Cr‚ation du r‚pertoire.\n"

$ Note: Q_REMOVE, Q_CONTINUE, and Q_ANYWAY don't end in a newline.
$ Also, the answer to them must be either the string in ANS_Y or ANS_N or ANS_Q.
Q_REMOVE "Souhaitez-vous supprimer maintenant la version de DOS actuellement install‚e (o/n/q)? "
Q_CONTINUE "Souhaitez-vous poursuivre l'installation de DOS (o/n/q)? "
Q_ANYWAY "Souhaitez-vous malgr‚ tout tenter de supprimer DOS (o/n/q) ? "
Q_PROMPT "Veuillez r‚pondre o, n ou q\n"
ANS_Y "y"
ANS_N "n"
$ Note: ANS_Q is what users can type at all prompts to "quit" or abort
$ the installation, (also entering 0 does the same thing as ANS_Q).
ANS_Q "q"
DOSINST_0Q_MSG "\
Si vous entrez 0 ou q au niveau d'une invite, l'installation de DOS sera abandonn‚e.\n"
CANCEL_MSG "Annulation de la r‚installation de DOS.\n"
IMPROPER "DOS n'est pas install‚ correctement.\n"
RM_ABORT "Abandon de la suppression de DOS. Aucun fichier n'a ‚t‚ supprim‚.\n"
RM_PROB "Erreur : problŠmes survenus lors de la suppression de DOS.\n"
RM_PROB_1 "Liste de fichiers manquante. Impossible de d‚sinstaller DOS int‚gralement.\n"
RM_ALMOST "La tentative de suppression de DOS a abouti.\n"
RM_DONE "Suppression de DOS termin‚e.\n"
BAD_DISK "\
\      Impossible de lire la disquette.\n\
\      V‚rifiez qu'elle est ins‚r‚e dans l'unit‚ appropri‚e.\n"
CLEAN_UP "\
Pour effectuer l'installation complŠte de DOS, lib‚rez de l'espace disque et red‚marrez le systŠme.\n"
VDISK_ERR "Erreur au niveau du disque virtuel. Impossible d'installer DOS.\n"
MISSING_FILE "Erreur : la recherche de %1 a ‚chou‚\n"
INTERNAL_ERR "Erreur interne dosinstall %1\n"
EXPANDING "Extension des fichiers DOS.\n"
CONFIGURING "Configuration des fichiers DOS.\n"
DRIVEA_NAME "A"
DRIVEB_NAME "B"
DOSINST_NDISKS_Q "Nombre de disquettes %1 figurant dans votre jeu : "
DOSINST_0_9 "Veuillez entrer un chiffre compris entre 0 et 9\n"
DOSINST_0_ABORT "Si vous entrez 0, l'installation de DOS sera abandonn‚e.\n"
FROM_BUILTIN "(… partir des fichiers int‚gr‚s)"
FROM_FLOP "(… partir des disquettes)"
MIN_SYSTEM "systŠme minimum"
DOSINST_PLS_ENTER "Veuillez entrer %1\n"
MISSING_SET "Fichiers 'set' manquants\n"
CREATE_BOOT_FAIL "\
Avertissement : la cr‚ation du nouveau fichier boot.dsk a ‚chou‚.\n\
\                Utilisation du fichier boot.dsk existant.\n"
BOOT_TOOSMALL "\
Avertissement : impossible de cr‚er un disque virtuel d'une capacit‚\n\
\                suffisante pour initialiser toutes les fonctions DOS\n\
\                NLS ; par cons‚quent, d‚sactivation des fonctions\n\
\                DOS NLS automatiques.\n"

$domain LCC.MERGE.UNIX.CHECKMERGE
$ Messages for the "checkmerge" script.
USAGE "SYNTAXE : %1\n"
MSG_1 "Comparaison des contr“les de coh‚rence de fichiers pour\n"
MSG_2 "avec les valeurs contenues dans %1...\n"
MSG_3 "modif‚ depuis votre derniŠre installation de Merge.\n"
MSG_4 "Impossible de calculer le contr“le de coh‚rence de %1.\n\
Poursuite de l'op‚ration...\n"
MSG_5 "V‚rification des fichiers termin‚e.\n"

$domain LCC.MERGE.UNIX.LPINSTALL
$ Messages for the lpinstall script.
MAIN_MENU "\n\
\n\
\n\
\         Programme d'installation de l'imprimante Merge\n\
\         ----------------------------------------------\n\
\n\
\         1)  Installer une imprimante\n\
\         2)  Supprimer une imprimante\n\
\         3)  Dresser la liste des imprimantes\n\
\n\
\         %1)  Quitter\n\
\n\
\         ----------------------------------\n\
\         Entrer l'option : "
$ Note: QUIT_CHAR must be a single character that mean quit.  It is used
$ as the %1 in the MAIN_MENU message and is compared with what the user
$ typed as the response to the "enter option" question.
QUIT_CHAR "q"
INSTALL "\n\
\         INSTALLER\n\
\         ---------------------\n"
PRINTER "        Nom de l'imprimante [%1] : "
MODEL   "        ModŠle de l'imprimante [%1] : "
DEVICE  "        P‚riph‚rique [%1] : "
$ Note: YES_CHAR must be a single character that means yes.  It is used
$ as the %1 in the OK_INST and  %2 in OK_REMOVE messages and is compared
$ with what the user typed as the response to those messages.
YES_CHAR "y"
OK_INST  "        D'accord pour installer ? [%1] : "
OK_REMOVE "        D'accord pour supprimer %1? [%2] : "
CONTINUE "        Appuyez sur [ENTREE] pour continuer"
REMOVE "\n\
\         SUPPRIMER\n\
\         ---------------------\n\
\         Nom de l'imprimante : "
LIST "\n\
Nombre d'imprimantes install‚es : %1\n\
---------------------------------------------\n"
MISSING_PROG	"Programme %1 manquant.\n"
NO_LP		"Le service d'impression LP n'est peut-ˆtre pas install‚.\n"
CANNOT_CONTINUE	"Impossible de continuer.\n"

$domain LCC.MERGE.UNIX.MKIMG
$ Messages for the "mkimg" script.

CANNOT_CONTINUE	"Impossible de continuer.\n"
CANNOT_COPY	"Avertissement : impossible de copier le fichier %1.\n"
CANNOT_MAKE	"Impossible de g‚n‚rer l'image %1.\n"
CANNOT_MKDIR	"Impossible de cr‚er le r‚pertoire : %1\n"
CANNOT_READ	"Impossible de lire le fichier %1\n"
CANNOT_WRITE	"Impossible d'‚crire dans %1\n"
CONFIG_MSG	"Configuration des fichiers images DOS.\n"
COPY_PROB	"ProblŠmes survenus lors de la copie de fichiers sur le disque virtuel %1\n"
CREATE_NOROOM	"Espace insuffisant pour cr‚er le fichier %1\n"
DOS_NI		"DOS n'est pas install‚\n"
FORMAT_PROB	"ProblŠmes survenus lors du formatage du disque virtuel %1\n"
IMG_MADE	"L'image %1 a ‚t‚ g‚n‚r‚e.\n"
INCOMPATABLE	"(Carte %1 incompatible ?)\n"
MAKING		"Cr‚ation de l'image %1.\n"
MISSING		"Fichier %1 manquant\n"
MUST_SU		"Seul l'utilisateur racine ou le superutilisateur est autoris‚ … g‚n‚rer l'image %1.\n"
NATIVE_MUST_SU	"Seul l'utilisateur racine ou le superutilisateur est autoris‚ … g‚n‚rer l'image native %1.\n"
NATIVE_TOKEN	"natif"
NOT_MADE	"L'image %1 n'a pas ‚t‚ g‚n‚r‚e.\n"
NO_DIRECTORY	"Le r‚pertoire %1 n'existe pas.\n"
NO_IMG_MADE	"Aucun fichier image n'a ‚t‚ g‚n‚r‚.\n"
NO_PERMIS_TMP	"Cr‚ation du fichier temporaire %1 non autoris‚e\n"
NO_PERMIS_WRITE	"Ecriture dans %1 non autoris‚e\n"
NO_ROOM		"(Plus d'espace disponible ?)\n"
NO_ROOM_TMP	"Espace insuffisant pour cr‚er le fichier temporaire %1\n"
OPT_NA		"Option %1 non autoris‚e.\n"
REMOVE_FAIL	"Impossible de supprimer le fichier %1\n"
SOME_NOT_MADE	"Certains fichiers images n'ont pas ‚t‚ g‚n‚r‚s.\n"
USAGE_LINE	"SYNTAXE : %1 [cga] [mda] [ d r‚pertoire ] [devicelow] [+aXXX]\n"
USING_INSTEAD	"Utilisation de %1.\n"
USING_ONCARD	"Utilisation de ROM %1 sur la carte d'affichage.\n"
WRONG_DISP	"Impossible de g‚n‚rer l'image %1 sur l'‚cran de type %2.\n"

$domain LCC.MERGE.UNIX.S55MERGE
$ Messages for "S55merge" script.
NOT_DOSINST	"Avertissement : l'installation de DOS n'est pas automatique.\n"
NOT_MKIMG	"Avertissement : la cr‚ation des fichiers images DOS n'est pas automatique.\n"
DOING_MKIMG	"Cr‚ation automatique d'images DOS en arriŠre-plan.\n"

$domain LCC.MERGE.UNIX.PART_SET
$ Messages for the "part_set" script.
BAD_DOSDEV	"Fichier '%1' alt‚r‚.\n"
MISSING_LINE	"Ligne '%1' manquante.\n"

$domain LCC.MERGE.UNIX.MERGEFONTMAKE
$ Messages for the "mergefontmake" script (part of X-Clients package).
MFM_USAGE	"\
SYNTAXE : mergefontmake  0|1|2|3|4  [s|ML]\n\
Recompilation des polices Merge.\n\
\n\
Les polices peuvent ˆtre compil‚es de quatre fa‡ons diff‚rentes et\n\
pour ˆtre utilisables, elles doivent ˆtre compil‚es selon la m‚thode\n\
X-Server. Le premier paramŠtre indique la m‚thode utilis‚e pour la\n\
compilation des polices : 1, 2, 3 ou 4. Le paramŠtre '0' correspond …\n\
l'utilisation des m‚thodes par d‚faut, quelles qu'elles soient.\n\
\n\
'mergefontmake' tente de d‚terminer la version du compilateur de polices\n\
que vous utilisez, car chaque version X utilise sa propre option de\n\
compilation des polices. (Une des versions utilise l'option 's' au\n\
lieu de '-M' et '-L'). Pour d‚finir le type d'options utilis‚,\n\
sp‚cifiez 's' ou 'ML' comme second paramŠtre.\n"
MFM_MISSING	"Erreur : %1 manquant\n"
MFM_BAD_SECONDP	"Erreur : le second paramŠtre doit ˆtre 's' ou 'ML', ou ne doit pas ˆtre pr‚cis‚.\n"
MFM_NO_XBIN	"Erreur : r‚pertoire X bin introuvable.\n"
MFM_NO_FONTDIR	"Erreur : impossible de d‚terminer le r‚pertoire des polices.\n"
MFM_I_NOPERMIS	"Erreur : l'installation des polices de caractŠres dans %1 n'est pas autoris‚e\n"
MFM_01234	"Erreur : utilisez  0, 1, 2, 3, ou 4.\n"
MFM_U_NOPERMIS	"Erreur : mise … jour de %1 non autoris‚e\n"
MFM_FC_ERR	"Erreur : %1 a g‚n‚r‚ %2\n"
MFM_COMP_ERR	"Erreur survenue lors de la compilation de %1\n"

$domain LCC.MERGE.UNIX.WINSETUP
$ Messages for the "winsetup" script.  This scripts runs when the
$ "Win_Setup" icon is used, or the user types "winsetup".
$ There are a maximum of 24 lines of message that can be printed out,
$ altough all don't have to be used.
$ Also, a single line message can be put into "SPCL_MSG", which is printed
$ out before the batch scripts are run.
$ All these messages are printed on the DOS screen, so these messages
$ must be in the appropriate DOS code page.
LINE01 "ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿"
LINE02 "³    Utilisez cette session DOS pour installer et reconfigurer Windows 3.1    ³"
LINE03 "ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´"
LINE04 "³                                                                             ³"
LINE05 "³ - Pour installer Windows, ins‚rez disquette 1 de votre set de dist. de      ³"
LINE06 "³  Windows et tapez la commande suivante si vous effectuez l'inst. d'unit‚ A: ³"
LINE07 "³                                                                             ³"
LINE08 "³               a:\\install                                                    ³"
LINE09 "³                                                                             ³"
LINE10 "³   (ou b:\\install si vous effectuez l'installation … partir de l'unit‚ B:)   ³"
LINE11 "³                                                                             ³"
LINE12 "³   Pour une desc. d‚taill‚e de la proc‚dure d'installation, consultez votre  ³"
LINE13 "³   manuel. Ce dernier pr‚sente ‚galement d'autres m‚thodes d'installation.   ³"
LINE14 "³                                                                             ³"
LINE15 "³ - Pour reconfigurer Windows, acc‚dez d'abord … l'unit‚ et au r‚pertoire o—  ³"
LINE16 "³   Windows est install‚, puis tapez :                                        ³"
LINE17 "³                                                                             ³"
LINE18 "³                install                                                      ³"
LINE19 "³                                                                             ³"
LINE20 "ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ"
$quote "
$domain LCC.PCI.UNIX.LSET_ESTR
$ The following messages are from lset_estr.c
LSET_ESTR0 "il ne s'agit pas d'une erreur"
LSET_ESTR1 "erreur systŠme"
LSET_ESTR2 "paramŠtre erron‚"
LSET_ESTR3 "la table est verrouill‚e/d‚verrouill‚e"
LSET_ESTR4 "impossible de verrouiller/d‚verrouiller la table"
LSET_ESTR5 "impossible d'obtenir l'identificateur IPC"
LSET_ESTR6 "impossible de cr‚er le jeu de verrous ou d'acc‚der … ce dernier"
LSET_ESTR7 "impossible d'attacher le segment de m‚moire partag‚e"
LSET_ESTR8 "espace m‚moire/table insuffisant"
LSET_ESTR9 "le connecteur indiqu‚ est d‚j… affect‚"
LSET_ESTR10 "le connecteur indiqu‚ est disponible"
LSET_ESTR11 "le jeu de verrous existe d‚j…"
LSET_ESTR12 "autorisation refus‚e"
LSET_ESTR13 "erreur LSERR inconnue (%d)"

$ @(#)messages	4.5 9/6/94 14:20:37

$quote "
$domain LCC.PCI.UNIX.RLOCKSHM

$ The following messages are from rlockshm.c
RLOCKSHM6 "La configuration est correcte uniquement avec le nouveau type de m‚moire partag‚e.\n"
RLOCKSHM7 "Impossible d'ex‚cuter %1 sur la m‚moire partag‚e, "
RLOCKSHM8 "(erreur systŠme : %1).\n"
RLOCKSHM9 "%1.\n"
RLOCKSHM10 "\nInformations de configuration par d‚faut :\n\n"
RLOCKSHM11 "\tadresse correspondant … l'attachement du segment\t\t(programme s‚lectionn‚)\n"
RLOCKSHM12 "\tadresse correspondant … l'attachement du segment\t\t%1\n"
RLOCKSHM14 "\tcl‚ d'accŠs … la m‚moire partag‚e\t\t%1\n"
RLOCKSHM15 "\tcl‚ d'accŠs au jeu de verrous\t\t\t%1\n"
RLOCKSHM17 "\tentr‚es de la table des fichiers ouverts\t\t%1\n"
RLOCKSHM18 "\tentr‚es de la table des en-tˆtes de fichiers\t%1\n"
RLOCKSHM19 "\tentr‚es de la table des fichiers hash\t%1\n"
RLOCKSHM20 "\tentr‚es de la table des verrous d'enregistrements\t%1\n"
RLOCKSHM22 "\tverrous d'enregistrements individuels\t\t%1\n"
RLOCKSHM24 "Ces donn‚es peuvent ˆtre remplac‚es par ces noms"
RLOCKSHM25 " de paramŠtres configurables : \n"
RLOCKSHM27 "\t%1 - %2\n"
RLOCKSHM28 "\nDiverses informations m‚moire existantes : \n\n"
RLOCKSHM29 "\tentr‚es de la table des fichiers ouverts\t\t%1 @ %2 octets\n"
RLOCKSHM30 "\tentr‚es de la table des en-tˆtes de fichiers\t%1 @ %2 octets\n"
RLOCKSHM31 "\tentr‚es de la table des fichiers hash\t%1 @ %2 octets\n"
RLOCKSHM32 "\tentr‚es de la table des verrous d'enregistrements\t%1 @ %2 octets\n"
RLOCKSHM34 "\tbase des segments ouverts\t\t%1\n"
RLOCKSHM35 "\tbase des segments de fichiers\t\t%1\n"
RLOCKSHM36 "\tbase de segments de type verrou\t\t%1\n"
RLOCKSHM38 "\ttable des fichiers ouverts\t\t\t%1\n"
RLOCKSHM39 "\ttable des en-tˆtes de fichiers\t\t%1\n"
RLOCKSHM40 "\ttable des fichiers hash\t\t%1\n"
RLOCKSHM41 "\ttable des verrous d'enregistrements\t\t%1\n"
RLOCKSHM43 "\tliste des tables ouvertes disponibles\t\t%1\n"
RLOCKSHM44 "\tliste des tables de fichiers disponibles\t\t%1\n"
RLOCKSHM45 "\tliste des tables de verrous disponibles\t\t%1\n"
RLOCKSHM46 "\tentr‚es de la table des fichiers ouverts\t\t%1 @ %2 octets\n"
RLOCKSHM47 "\tentr‚es de la table des en-tˆtes de fichiers\t%1 @ %2 octets\n"
RLOCKSHM48 "\tentr‚es de la table des fichiers hash\t%1 @ %2 octets\n"
RLOCKSHM49 "\tentr‚es de la table des verrous d'enregistrements\t%1 @ %2 octets\n"
RLOCKSHM50 "\ttaille totale des segments\t\t%1 octets\n"
RLOCKSHM52 "\tverrous d'enregistrements individuels\t\t%1\n"
RLOCKSHM54 "\tbase des attachements\t\t%1"
RLOCKSHM55 " (programme s‚lectionn‚)"
RLOCKSHM57 "\ttable des fichiers ouverts\t\t\t%1\n"
RLOCKSHM58 "\ttable des en-tˆtes de fichiers\t\t%1\n"
RLOCKSHM59 "\ttable des fichiers hash\t\t%1\n"
RLOCKSHM60 "\ttable des verrous d'enregistrements\t\t%1\n"
RLOCKSHM62 "\tindex de la liste des tables ouvertes disponibles\t%1\n"
RLOCKSHM63 "\tindex de la liste des tables de fichiers disponibles\t%1\n"
RLOCKSHM64 "\tindex de la liste des tables de verrous disponibles\t%1\n"
RLOCKSHM65 "\tfacteur de restriction d'alignement\t%1\n"
RLOCKSHM66 "\nEntr‚es de la table des en-tˆtes de fichiers :\n"
RLOCKSHM100 "entr‚e"
RLOCKSHM101 "liste des ‚l‚ments ouverts"
RLOCKSHM102 "liste des ‚l‚ments verrouill‚s"
RLOCKSHM103 "hash associ‚ … la liaison"
RLOCKSHM104 "ID sp‚cifique"
RLOCKSHM105 "index des ‚l‚ments ouverts"
RLOCKSHM106 "index des ‚l‚ments verrouill‚s"
RLOCKSHM70 "\nEntr‚es de la table des en-tˆtes de fichiers hash :\n"
RLOCKSHM200 "entr‚e"
RLOCKSHM201 "en-tˆte du fichier"
RLOCKSHM75 "\nEntr‚es de la table de verrouillage des enregistrements :\n"
RLOCKSHM111 "verrou suivant"
RLOCKSHM112 "verrou bas"
RLOCKSHM113 "verrou haut"
RLOCKSHM114 "ID session"
RLOCKSHM115 "PID dos"
RLOCKSHM116 "index suivant"
RLOCKSHM80 "\nEntr‚es de la table des fichiers ouverts :\n"
RLOCKSHM120 "ouvrir suivant"
RLOCKSHM121 "en-tˆte"
RLOCKSHM122 "acc"
RLOCKSHM123 "refuser"
RLOCKSHM130 "index des en-tˆtes de fichiers"
RLOCKSHM131 "descripteur de fichier"
RLOCKSHM_OLDST	"AVERTISSEMENT : utilisation d'une m‚moire d'un type ancien -- maintenant obsolŠte\n"
RLOCKSHM_USAGE	"\nsyntaxe : %1 [-cdhmrAFHLOV] [nom=donn‚es] ...\n"
RLOCKSHM_DETAIL "\
\  -c  Cr‚e le segment de m‚moire partag‚e.\n\
\  -d  Affiche la configuration par d‚faut (uniquement).\n\
\  -h  Affiche ces informations (uniquement).\n\
\  -m  Affiche les diff‚rentes informations concernant le segment existant.\n\
\  -r  Supprime le segment de m‚moire partag‚e.\n\
\  -A  Affiche toutes les entr‚es (y compris les entr‚es non utilis‚es).\n\
\  -F  Affiche la table des en-tˆtes de fichiers.\n\
\  -H  Affiche la table hashed des en-tˆtes de fichiers.\n\
\  -L  Affiche la table des verrouillages d'enregistrement.\n\
\  -O  Affiche la table des fichiers ouverts.\n\
\  -V  Imprime la version/le copyright (uniquement).\n\
\  nom=donn‚es - Nombre de paramŠtres de configuration sup‚rieur ou ‚gal … z‚ro.\n\n"

$ The following messages are from set_cfg.c
$quote "
SET_CFG1 "ChaŒne de configuration non autoris‚e : '%1'\n"
SET_CFG2 "Aucun nom indiqu‚ pour '%1'.\n"
SET_CFG3 "Le nom figurant dans '%1' est trop long.\n"
SET_CFG4 "Aucune donn‚e indiqu‚e pour '%1'.\n"
SET_CFG5 "Nom inconnu : '%1'\n"
SET_CFG6 "Les donn‚es associ‚es … '%1' doivent ˆtre positives.\n"
SET_CFG7 "Les donn‚es associ‚es … '%1' sont incorrectes.\n"
SET_CFG8 "Les donn‚es associ‚es … '%1' doivent ˆtre inf‚rieures … %2.\n"

$ The following are handcrafted from set_cfg.c
SET_CFG100 "base"
SET_CFG101 "adresse correspondant … l'attachement du segment (0 = programme s‚lectionn‚)"
SET_CFG102 "cl‚s"
SET_CFG103 "param‚trer LSW pour les codes d'accŠs … la m‚moire partag‚e et aux jeux de verrous"
SET_CFG104 "table_ouverte"
SET_CFG105 "nombre maximal d'entr‚es dans la table des fichiers ouverts"
SET_CFG106 "table_des_fichiers"
SET_CFG107 "nombre maximal d'entr‚es dans la table des en-tˆtes de fichiers"
SET_CFG108 "table hash"
SET_CFG109 "nombre maximal d'entr‚es dans la table des fichiers hash"
SET_CFG110 "table des verrous"
SET_CFG111 "nombre maximal d'entr‚es dans la table des verrous d'enregistrements"
SET_CFG112 "verrous d'enregistrements"
SET_CFG113 "nombre maximal de verrous d'enregistrements individuels"
$ SCCSID(@(#)messages	7.11	LCC)	/* Modified: 10:33:25 10/20/93 */

$quote "
$domain LCC.PCI.UNIX

$ In general, the names below start with some indication of the file in
$ which the string is actually used.  Usually, this is the base name of
$ the file, although it may be some "shorter" version of the name.

LICM_VIOLATION	"Violation de licence, %1 (arrˆt) vs. %2\n"
LICM_TERMINATE	"Violation de licence -- arrˆt du serveur\n"
LICM_BAD_KEY	"License incorrecte (code de licence erron‚)\n"
LICM_ALTERED	"Licence incorrecte (texte modifi‚)\n"
LICM_NO_ID	"Licence incorrecte (pas d'ID de licence)\n"
LICM_NO_MEMORY	"Licence inutilisable (pas de m‚moire)\n"
LICM_EXPIRED	"Licence inutilisable (p‚rim‚e)\n"
LICM_NO_SERIAL	"Aucun num‚ro de s‚rie client n'a ‚t‚ pr‚cis‚\n"
LICM_BAD_SERIAL	"le num‚ro de s‚rie client indiqu‚ est incorrect\n"

LICU_DUPLICATE	"Num‚ro de s‚rie client en double, connexion non autoris‚e."
LICU_INVALID	"Num‚ro de s‚rie client incorrect, connexion non autoris‚e."
LICU_RESOURCE	"La ressource h“te n'est pas disponible, prenez contact avec votre administrateur systŠme."

$ %1 is the maximum number of clients
LICU_LIMIT	"Le nombre maximal de clients autoris‚s sur ce serveur a ‚t‚ d‚livr‚ (%1), connexion impossible."

$ %1 is the log file name, %2 is the error string, %3 and %4 (where used) are
$ the user and group IDs, respectively
LOG_OPEN	"Impossible d'ouvrir le fichier-journal '%1', %2\n"
LOG_CHMODE	"Impossible de changer le mode de '%1', %2\n"
LOG_CHOWN	"Impossible de remplacer le propri‚taire de '%1' par %3/%4, %2\n"
LOG_REOPEN	"Impossible de rouvrir '%1' aprŠs la troncature, %2\n"

$ %1 is a host file descriptor
LOG_OPEND	"Impossible de rouvrir le fichier-journal … partir du descripteur %1\n"

$ %1 is the program name, %2 is the process id
LOG_SERIOUS	"Erreur grave dans %1 (PID %2), tentative de poursuite du processus\n"
LOG_FATAL	"Erreur bloquante dans %1 (PID %2), impossible de continuer\n"

$ %1 is a number of bytes
MEM_NONE	"Impossible d'affecter %1 octets de m‚moire\n"
MEM_RESIZE	"Impossible de red‚finir la capacit‚ m‚moire sur %1 octets\n"

$ %1 is the host name
NETAD_NOHOST	"Impossible de trouver les donn‚es relatives … l'h“te '%1'\n"

$ %1 is the error string
NETIF_DEVACC	"Impossible d'acc‚der au p‚riph‚rique r‚seau, %1\n"
NETIF_CONFIG	"Impossible de d‚terminer la configuration r‚seau, %1\n"

NETIO_DESC_ARR	"Impossible d'affecter la matrice des descripteurs r‚seau\n"
NETIO_RETRY	"Impossible de recevoir un paquet … partir du r‚seau\n"
NETIO_CORRUPT	"La table des r‚seaux internes a ‚t‚ alt‚r‚e\n"
NETIO_MUXERR	"Erreur de multiplexage r‚seau\n"

$ %1 is the size of a network packet
NETIO_PACKET	"La taille maximale des paquets r‚seau (%1) est insuffisante\n"

$ %1 is a network address
NETIO_IFERR	"Erreur irr‚m‚diable au niveau de l'interface %1\n"

DOSSVR_NO_CSVR	"Impossible de trouver l'adresse du serveur de connexion\n"
DOSSVR_SETIF	"Impossible d'ouvrir l'interface du r‚seau local\n"
DOSSVR_CURDIR	"Impossible de d‚terminer le r‚pertoire de travail actuel\n"

$ %1 is the error string
DOSSVR_R_CPIPE	"Impossible de lire le canal de configuration, %1\n"
DOSSVR_G_NETA	"Impossible d'obtenir les attributs du r‚seau, %1\n"
DOSSVR_S_NETA	"Impossible de d‚finir les attributs du r‚seau, %1\n"
DOSSVR_G_TERMA	"Impossible d'obtenir les attributs de la ligne du terminal, %1\n"
DOSSVR_S_TERMA	"Impossible de d‚finir les attributs de la ligne du terminal, %1\n"
DOSSVR_C_CPIPE	"Impossible de cr‚er le canal de configuration, %1\n"
DOSSVR_NOFORK	"Impossible de cr‚er un nouveau processus, %1\n"

$ %1 is an RLOCK package error, %1 is a system error
DOSSVR_RLINIT	"Impossible d'initialiser les donn‚es relatives aux verrous d'enregistrement, %1, %2\n"

$ %1 is a program name, %2 is an error string (if used)
DOSSVR_NOEXEC	"Impossible de lancer '%1', %2\n"
DOSSVR_NOSHELL	"Impossible de lancer le shell utilisateur '%1'\n"
DOSSVR_ACC_SVR	"Impossible d'acc‚der au serveur DOS '%1'\n"
DOSSVR_RUN_SVR	"Impossible d'ex‚cuter le serveur DOS '%1', %2\n"

$ %1 is an luid, %2 is the error string
DOSSVR_LUID	"Impossible de d‚finir le luid sur %1, %2\n"

$ %1 is the written count, %2 is the expected count, %3 is the error string
DOSSVR_W_CPIPE	"Impossible d'‚crire dans le canal de configuration (%1 octets de %2), %3\n"

CONSVR_NOMEM	"Impossible d'affecter de la m‚moire pour la chaŒne de fonctions\n"
CONSVR_NO_NET	"Impossible d'ouvrir le(s) interface(s) r‚seau\n"

$ %1 is the luid that started the consvr process
CONSVR_LUID	"Le luid est d‚j… param‚tr‚ sur %1\n"

$ %1 is file or program name, %2 is error string (where used)
CONSVR_RUN_SVR	"Impossible d'ex‚cuter le serveur DOS '%1', %2\n"
CONSVR_NO_FF	"Impossible d'ouvrir le fichier de fonctions '%1'\n"
CONSVR_ERR_FF	"Fichier de fonctions '%1' erron‚\n"

$ %1, %2, %3 and %4 are the major, minor, sub-minor and special version ID
$ values, respectively
CONSVR_BANNER		"Interface PC pour DOS, version %1.%2.%3 %4\n"
CONSVR_BANNER_AADU	"Serveur DOS pour AIX, version %1.%2\n"

$ %1 is error string
IPC_NO_MSGQ	"Impossible de cr‚er une file d'attente de messages, %1\n"
IPC_NO_SEM	"Impossible de cr‚er un s‚maphore, %1\n"

MAPSVR_NO_NET	"Impossible d'ouvrir le(s) interface(s) r‚seau\n"

DOSOUT_SEGMENT	"Impossible d'acc‚der au segment de m‚moire partag‚e RD\n"
DOSOUT_REXMIT	"Retransmissions trop nombreuses\n"

$ %1 is an error string
DOSOUT_NO_SHM	"Pas de m‚moire partag‚e, %1\n"
DOSOUT_S_NETA	"Impossible de d‚finir les attributs du r‚seau, %1\n"
DOSOUT_PIPE_ACK	"Erreur d'E/S au niveau du canal (attente ACK), %1\n"
DOSOUT_PIPE_CNT	"Erreur au niveau du canal d'E/S (contr“le ACK), %1\n"
DOSOUT_ERR6	"Erreur de lecture PTY 6 : %1\n"
DOSOUT_ERR7	"Erreur de lecture PTY 7 : %1\n"
DOSOUT_ERR8	"Erreur de lecture PTY 8 : %1\n"
DOSOUT_ERR13	"Erreur de lecture PTY 13 : %1\n"
DOSOUT_ERR14	"Erreur de lecture PTY 14 : %1\n"
DOSOUT_ERR19	"Erreur de lecture PTY 19 : %1\n"
DOSOUT_ERR9	"Erreur d'‚criture TTY 9 : %1\n"
DOSOUT_ERR10	"Erreur d'‚criture TTY 10 : %1\n"
DOSOUT_ERR15	"Erreur d'‚criture TTY 15 : %1\n"
DOSOUT_ERR16	"Erreur d'‚criture TTY 16 : %1\n"
DOSOUT_ERR17	"Erreur d'‚criture TTY 17 : %1\n"

$ %1 is an error string
SEMFUNC_NOSEM	"Impossible de cr‚er un s‚maphore RD, %1\n"
SEMFUNC_NOSHM	"Impossible de cr‚er un segment de m‚moire partag‚e RD, %1\n"

$ %1 is the number of objects in the caches, %2 is the size of each object
VFILE_NOMEM	"Impossible d'affecter de la m‚moire pour %1 objets, %2 octets chacun\n"

$ %1 is the lcs error number
NLSTAB_HOST	"Impossible d'acc‚der … la table LCS des h“tes, erreur %1\n"
NLSTAB_CLIENT	"Impossible d'acc‚der … la table LCS des clients, erreur %1\n"
NLSTAB_SET	"Impossible de param‚trer les tables LCS, erreur %1\n"

DEBUG_CHILD	"enfant"
DEBUG_OFF	"off"
DEBUG_ON	"activ‚"

$ %1 is the program name
DEBUG_USAGE	"Syntaxe : %1 <PID> [[op]canaux] [enfant] [on] [off]\n"

$ %1 is the program name, %2 is the faulty command line argument
DEBUG_ARG	"%1 : argument incorrect : '%2'\n"

$ %1 is the program name, %2 is the channel file, %3 is an error string
DEBUG_NOFILE	"%1 : impossible de cr‚er un fichier de canaux '%2', %3\n"

$ %1 is the program name, %2 is a PID, %3 is an error string
DEBUG_INVAL	"%1 : le processus %2 ne peut ˆtre signal‚, %3\n"
DEBUG_NO_SIG	"%1 : impossible de signaler le processus %2, %3\n"

$ %1 is the program name, %2 is a channel number specified in the argument list
DEBUG_BADBIT	"%1 : le canal nø%2 est hors limites\n"
$ SCCSID("@(#)messages	7.2	LCC")	/* Modified: 1/19/93 20:08:07 */
$domain LCC.PCI.DOS.CONVERT
$quote "
$ NOTE: '\n' indicates that a new line will be printed
$ The following messages are from convert.c
CONVERT1 "Majuscules ET minuscules sp‚cifi‚es\n"
CONVERT1A "Des options incompatibles on ‚t‚ sp‚cifi‚es\n"
CONVERT2 "Les traductions sp‚cifi‚es sont incompatibles\n"

$ %1 is the file name on which a read error occured
CONVERT3 "Erreur survenue lors de la lecture de %1\n"

$ %1 is the fiel name which cannot be opened
CONVERT4 "Impossible d'ouvrir %1\n"

$ input file %1 and output file %2 are identical
CONVERT5 "Les fichiers %1 et %2 sont identiques\n"

$ an error was encountered writing file %1
CONVERT6 "Erreur survenue lors de l'‚criture dans %1\n"

CONVERT7 "Table de traduction des r‚sultats non fournie\n"

CONVERT8 "Majuscules/minuscules sp‚cifi‚es sans indication d'une option de traduction\n"

$ translation table %1 cannot be opened
CONVERT10 "Impossible d'ouvrir la table de traduction %1\n"

CONVERT15 "Table de traduction incorrecte !\n"
CONVERT17 "Table de traduction des donn‚es d'entr‚e non fournie\n"
CONVERT21 "Echec de l'‚criture dans le fichier de sortie !\n"
CONVERT30 "Impossible d'affecter de l'espace pour le tampon de traduction !\n"
CONVERT31 "Tables de traduction non d‚finies !\n"

$ character %1 was untranslatable with the options used
CONVERT32 "\nCaractŠre intraduisible … la ligne nø %1\n"

$ unknown error %1 occurred
CONVERT42 "Erreur de traduction inconnue : %1\n"

CONVERT45 "Table(s) de traduction introuvable(s) !\n"
CONVERT46 "Table(s) de traduction incorrecte(s) !\n"
CONVERT60 "Variable d'environnement COUNTRY non d‚finie, utilisation de 1\n"

$ code page %1 will be used as no other was specified
CONVERT61 "Variable d'environnement CODEPAGE non d‚finie, utilisation de %1\n"

CONVERT77 "Tables de traduction des donn‚es d'entr‚e et des r‚sultats non fournies\n"
CONVERT80 "Avertissement ! CaractŠre … 8 bits\n"
CONVERT86 "M‚moire insuffisante pour ex‚cuter CONVOPT !\n"
CONVERT90 "Impossible d'affecter davantage de m‚moire !\n"
CONVERT_B1 "Impossible d'ex‚cuter la meilleur conversion possible en un caractŠre unique avec le jeu de caractŠres par d‚faut !\n"
CONVERT_S1 "\nInformations relatives … la traduction :\n"

$ %1 is the number of glyphs
CONVERT_S2 "Traduction exacte des signes :\t\t%1\n"
CONVERT_S3 "Traduction des signes en plusieurs octets :\t%1\n"
CONVERT_S4 "Traduction des signes en valeurs par d‚faut utilisateur :\t%1\n"
CONVERT_S5 "Meilleure conversion possible des signes en un signe unique :\t%1\n"
CONVERT_S6 "\nNombre total de signes trait‚s :\t%1\n"

$ %1 is the number of bytes, %2 is the number of lines in the text file
CONVERT_S7 "\n%1 octets ont ‚t‚ trait‚s sur %2 lignes\n"

$ %1 is the name of the program
CONVERT_M1_D "syntaxe : %1   [/options] ... [entr‚e [sortie]]\n"
CONVERT_M3_D "Les options comprennent : /u      Fichier de majuscules.\n\
\                         /l      Fichier de minuscules.\n\
\                         /f      Forcer (ancienne option).\n\
\                         /b      Binaire (ancienne option).\n"
CONVERT_M4_D "\                /7      G‚n‚rer un avertissement si un caractŠre utilise le 8Šme bit.\n\
\                /x      Transmission directe.  Ne pas traduire.\n\n\
\                /i tbl  Traduire les donn‚es d'entr‚e au moyen de la table tbl.\n\
\                /o tbl  Traduire les r‚sultats au moyen de la table tbl.\n\
\n"
CONVERT_M5_D "\                /c c    Utilisation de c comme ‚chec de traduction user_char.\n\
\                /m      Permettre la trduction de plusieurs caractŠres.\n\
\                /a      Abandonner lors de l'‚chec de la traduction.\n\
\                /s      Utiliser la meilleure traduction du caractŠre simple.\n\
\                /z      Utiliser CTL-Z correctement.\n\
\n"
CONVERT_M6_D "\                /d      Introduire un retour chariot pour ex‚cuter un changement de ligne avant la traduction.\n\
\                /p      Ex‚cuter un changement de ligne pour introduire un retour chariot avant la traduction.\n\
\n"
CONVERT_M7_D "\                /q      Supprimer l'affichage des messages d'avertissement.\n\
\                /v      Afficher les messages d'avertissement et les statistiques de traduction.\n\
\                /h ou /? Imprimer ce message.\n"


CONVERT_M1_U "syntaxe : %1   [-options] ... [entr‚e [sortie]]\n"
CONVERT_M3_U "Les options comprennent : -u      Fichier de majuscules.\n\
\                         -l      Fichier de minuscules.\n\
\                         -f      Forcer (ancienne option).\n\
\                         -b      Binaire (ancienne option).\n"
CONVERT_M4_U "\                -7      G‚n‚rer un avertissement si un caractŠre utilise le 8Šme bit.\n\
\                -x      Transmission directe.  Ne pas traduire.\n\n\
\                -i tbl  Traduire les donn‚es d'entr‚e au moyen de la table tbl.\n\
\                -o tbl  Traduire les r‚sultats au moyen de la table tbl.\n\
\n"
CONVERT_M5_U "\                -c c    Utilisation de c comme ‚chec de traduction user_char.\n\
\                -m      Permettre la trduction de plusieurs caractŠres.\n\
\                -a      Abandonner lors de l'‚chec de la traduction.\n\
\                -s      Utiliser la meilleure traduction du caractŠre simple.\n\
\                -z      Utiliser CTL-Z correctement.\n\
\n"
CONVERT_M6_U "\                -d      Introduire un retour chariot pour ex‚cuter un changement de ligne avant la traduction.\n\
\                -p      Ex‚cuter un changement de ligne pour introduire un retour chariot avant la traduction.\n\
\n"
CONVERT_M7_U "\                -q      Supprimer l'affichage des messages d'avertissement.\n\
\                -v      Afficher les messages d'avertissement et les statistiques de traduction.\n\
\                -h ou -? Imprimer ce message.\n\"

$ %1 is the filename, %2 is the options
CONVERT_HELP00_D "syntaxe : %1 [/%2] [fichier_entr‚e [fichier_sortie]]\n"
CONVERT_HELP00_U "syntaxe : %1 [-%2] [fichier_entr‚e [fichier_sortie]]\n"
CONVERT_HELP01_D "       %1 -h ou /? pour obtenir des informations d‚taill‚es\n"
CONVERT_HELP01_U "       %1 -h ou -? pour obtenir des informations d‚taill‚es\n"
CONVERT_HELP1_D "syntaxe : %1 /%2 [fichier_entr‚e [fichier_sortie]]\n"
CONVERT_HELP1_U "syntaxe : %1 -%2 [fichier_entr‚e [fichier_sortie]]\n"

$ The following messages are from getopt.c
$ Do NOT change of the order of %1 and %2!
GETOPT1 "Option %1%2 inconnue\n"
GETOPT2 "Argument manquant pour l'option %1%2\n"

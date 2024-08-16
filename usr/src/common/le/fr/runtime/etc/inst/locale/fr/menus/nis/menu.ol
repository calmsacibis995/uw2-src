.ul
Installation et configuration de NIS (Network Information Service)
.lr
F1=Aide
.top
`echo  "La configuration NIS suivante existe déjà sur cette machine."`
`echo "\n \n \n"`
`[ "$TYPE" = "1" ] && echo "    Cette machine est un serveur maître NIS.\n \n"`
`[ "$TYPE" = "2" ] && echo "    Cette machine est un serveur esclave NIS.\n \n"`
`[ "$TYPE" = "3" ] && echo "    Cette machine est un client NIS.\n \n"`
`echo "    Domaine NIS : $def_dom"` 
`echo "\n \n   Serveurs NIS du domaine $def_dom: \c" && cat /tmp/nis.overlay | xargs echo`
.button
Appliquer
Réinitialiser
.bottom
Appuyez sur "TAB" pour faire passer le curseur d'un champ à un autre.
Ensuite, placez le curseur sur "Appliquer" et appuyez sur "ENTREE" pour
continuer.
.form
2 2//Yes::Oui//No::Non//Utiliser configuration NIS actuelle ?//USE_CURRENT//
//Flèches gauche/droite pour sélectionner Oui et la conf. NIS actuelle//
.help
Cette machine a déjà été configurée avec NIS. Vous pouvez procéder à
l'installation en utilisant la configuration NIS actuelle (entrez Oui)
ou créer une nouvelle configuration NIS au moyen du script
d'installation NIS (entrez Non).

Remarque : ce menu n'affiche que les trois premiers serveurs NIS
           configurés pour ce domaine.
.helpinst
Suppr=Annuler F1=Aide ECHAP=Quitter l'aide 1=Page suiv. 2=Page préc.

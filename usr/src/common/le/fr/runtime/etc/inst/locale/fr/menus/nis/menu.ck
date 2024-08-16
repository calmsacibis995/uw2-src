.ul
Installation et configuration de NIS (Network Information Service)
.lr
F1=Aide
.top
`echo  "\n \nVous avez choisi la configuration NIS suivante : "`
`echo  "\n \n  $host est un $host_type NIS"` 
`echo  "\n \n  Domaine NIS : $def_dom"`
`[ "$slavep" = "F" ] && echo  "\n \n  Serveurs NIS du domaine $def_dom: $SERV1"`
`[ "$slavep" = "F" ] && echo  "                                       $SERV2"`
`[ "$slavep" = "F" ] && echo  "                                       $SERV3"`
`[ "$slavep" = "T" ] && echo  "\n\n  Serveur ma�tre NIS du domaine $def_dom: $master"`
`echo   "\n \nPour reconfigurer NIS ou annuler la configuration NIS, entrez Non."`
.button
Appliquer
R�initialiser
.bottom
Appuyez sur "TAB" pour faire passer le curseur d'un champ � un autre.
Ensuite, placez le curseur sur "Appliquer" et appuyez sur "ENTREE" pour
continuer.
.form
2 2//No::Non//Yes::Oui//Accepter configuration NIS ? ://ACCEPT//
//Fl�ches gauche/droite pour s�l. Oui et accepter la configuration//
.help
Si cette configuration NIS n'est pas acceptable, entrez "Non" et vous
pourrez reconfigurer NIS ou annuler la configuration dans le menu
suivant. Que la configuration NIS soit annul�e ou non, NIS sera
install�.

Remarque : ce menu n'affiche que les trois premiers serveurs NIS
           configur�s pour ce domaine.
.helpinst
Suppr=Annuler F1=Aide ECHAP=Quitter l'aide 1=Page suiv. 2=Page pr�c.

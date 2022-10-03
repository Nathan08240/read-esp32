# Installation et initialisation sur VSCODE

Il faut installer le plugin Platform.io sur VSCODE

![image](https://user-images.githubusercontent.com/50341252/193598403-fb33c198-600b-4fc9-b323-02b86f142caf.png)

Une fois installé, on va cliquer sur la tête d'Alien et séléctionner l'option Clone Git Project
dans lequel on va venir clône ce dépôt :

```
https://github.com/Nathan08240
```

![image](https://user-images.githubusercontent.com/50341252/193599671-94a157e0-3714-4b9a-a173-de0eba0596f5.png)


Une fois le dépôt cloner et ouvert sur votre VSCODE, on va se rendre dans le dossier /lib/Credentials/src.
Il y à a l'intérieur un fichier Credentials.h.ex, on va enlever l'extension .ex afin d'avoir uniquement Credentials.h

Il faut dedans remplir toutes les infos personnelles afin que le programme fonctionne correctement.

Une fois celà fait, on peut build pour la première fois le programme.


![image](https://user-images.githubusercontent.com/50341252/193599772-6f440e47-de9a-4a48-8640-d17bcfba2917.png)

![image](https://user-images.githubusercontent.com/50341252/193599818-bae58cf5-9371-4fae-a28a-c24833e0d306.png)


On appuie sur ce dernier bouton et maintenant on laisse la magie opérer ! 

Maintenant que c'est build, bien configurer i faut l'upload sur l'esp32, pour cela, rien de plus simple, on appuie sur le bouton à côté de build

![image](https://user-images.githubusercontent.com/50341252/193600150-09037f2f-e44e-48c3-96b5-2a0bd443d697.png)

L'upload peut démarrer.
A un moment donné, sur le termianl va s'afficher
```connecting...```
Il suffit de rester appuyer sur le bouton boot de l'esp32 et cela devrait se débloquer.

Maintenant tout est opérationel !

# CODE COULEUR ! 

## Au demarrage 

La led va clignoter rouge -> Recherche de Wifi
Une fois connecter au wifi, si elle clignote bleu -> COnnection au MQQT
Si bleu fixe -> opérationnel, tout fonctionne correctemement

##  Durant le fonctionnement !
La led doit de base être en bleu fixe

### lorsque l'on passe un badge 
Si led rouge -> Erreur !
Si led verte -> Badge correct ! 

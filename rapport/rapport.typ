#import "./template.typ": *
#import "@preview/codelst:2.0.1": sourcecode

#show: project.with(
  subject: "Systèmes d'exploitation centralisés",
  title: "Rapport : minishell",
  authors: ("Louis Thevenet",),
  teachers: ("Emannuel Chaput",),
  date: "4 Mai 2024",
  subtitle: "1SN-F",
  toc: true,
)

// #figure(
//   caption: [Entremêlement des sorties des processus],
// )[#image("./assets/q2_output.png")]

= Gestion des processus
== Enchaînement séquentiel des commandes
#figure(
  caption: "On attend la fin de l'éxecution du fils pour passer à la prochaine commande",
)[
#sourcecode[```rust
> ls
flake.lock  flake.nix  projet  rapport  result  sujets  tp
> echo test
test
> cat ./projet/test
exemple
> exit
Au revoir ...
```]

]
== Exécution en arrière-plan
#figure(caption: "On exécute la commande en arrière-plan")[
#sourcecode(highlighted: (10,), highlight-color: lime)[
```rs
> cat ./projet/test
#!/usr/bin/env bash

sleep 5
echo "Done!"
> ./projet/test &
>
> ls
flake.lock  flake.nix  projet  rapport  result  sujets  tp
> Done!
>

    ```
]
]

On peut également vérifier la bonne terminaison du fils après éxecution via
`watch ps -sf` :
#figure(
  caption: "Durant l'exécution",
)[
#sourcecode(
  highlighted: (10,),
  highlight-color: lime,
)[
```rs
S+   pts/2      0:00      \_ /nix/store/cci0aml5v6xdvkqrvg-minishell/bin/minishell
S+   pts/2      0:00          \_ bash ./projet/test
S+   pts/2      0:00              \_ sleep 5

    ```
]
]

#figure(
  caption: "Après exécution",
)[
#sourcecode(
  highlighted: (10,),
  highlight-color: lime,
)[
```rs
S+   pts/2      0:00      \_ /nix/store/cci0aml5v6xdvkqrvg-minishell/bin/minishell

    ```
]
]

= Gestion des tâches
Comme proposé dans l'énoncé du projet, on ajoute des commandes internes au
minishell :
/ lj: (list jobs) Affiche les tâches en cours
/ sj \<id>: (stop job) Arrête la tâche d'identifiant `id` (envoie `SIGSTOP`)
/ fg \<id>: (foreground) Met la tâche d'identifiant `id` en avant-plan
/ bg \<id>: (background) Met la tâche d'identifiant `id` en arrière-plan

Ces commandes seront utilisées dans la suite pour illustrer l'état du minishell.

De plus, un mode `debug` a été ajouté au projet afin d'afficher des informations
sur les signaux reçus. Il fournit des messages de logs durant le foncitonnement.

Pour l'activer, il faut compiler en définissant `DEBUG` qui change le comportant
de la macro suivante :

#sourcecode(
  highlighted: (1,),
  highlight-color: yellow,
)[
```c
// #define DEBUG // A DECOMMENTER

#ifdef DEBUG
#define LIGHT_GRAY "\033[1;30m"
#define NC "\033[0m"
#define DEBUG_PRINT(x)                                                         \
  printf("%s", LIGHT_GRAY);                                                    \
  printf x;                                                                    \
  printf("%s", NC);
#else
#define DEBUG_PRINT(x)                                                         \
  do {                                                                         \
  } while (0)
#endif

    ```
]

= Signaux
== Signal `SIGCHLD`
Dans la @sigchild, on :
- attend normalement la fin d'éxecution de la commande en avant-plan
- attend normalement la fin d'éxecution de la commande en arrière-plan
- envoie le signal `SIGCHLD` au processus fils
- envoie le signal `SIGSTOP` au processus fils
- envoie le signal `SIGCONT` au processus fils
- affiche les jobs en cours pour constater que le fils est continué en
  arrière-plan
- On teste ensuite les signaux `SIGSTOP` et `SIGCONT` sur un job en arrière-plan.

#figure(caption: [Démonstration `SIGCHLD`])[
#sourcecode()[
```rs
> sleep 2
[Child 294501 exited]
> sleep 2&
> [Child 294709 exited]

> sleep 9999
[Child 296018 signaled]
> sleep 9999
[Child 299625 stopped]
> [Child 299625 continued]

> lj
Job 0
  PID: 299625
STDOUT -> 4207432  State: Active  Command: sleep 9999
>
>
>
>
>
> sleep 999&
> [Child 335139 stopped]
[Child 335139 continued]

> lj
Job 0
  PID: 299625
STDOUT -> 4207432  State: Active  Command: sleep 9999

Job 1
  PID: 335139
STDOUT -> 4207432  State: Active  Command: sleep 999
>


    ```
]
]<sigchild>

== Signaux `SIGINT`, `SIGTSTP`

On voit dans cet exemple que le programme père reçoit le signal `SIGINT`, qu'il
décide de tuer le fils en avant-plan, finalement le message informant la
terminaison du processus fils est affiché.

#figure(caption: "Interruption au clavier")[
#sourcecode()[
```rs
> sleep 10
^C[SIGINT received]
[Killing 343628]
> [Child 343628 signaled]

>
    ```
]
]

#figure(caption: [Envoie du signal `SIGTSTP`])[
#sourcecode()[
```rs
> sleep 10
^Z[SIGTSTP received]
[Stopping 348897]
> [Child 348897 stopped]

> lj
Job 0
  PID: 348897
STDOUT -> 4207432  State: Suspended  Command: sleep 10
>
    ```
]
]

Les processus fils quant à eux ne reçoivent pas les signaux `SIGINT` et
`SIGTSTP`, c'est le père qui gère les interruptions.

#figure(
  caption: [Les processus fils masquent les signaux `SIGINT` et `SIGTSTP`],
)[
#sourcecode()[
```rs
> sleep 999&
> ^C[SIGINT received]
^C[SIGINT received]
^Z[SIGTSTP received]
^Z[SIGTSTP received]

> lj
Job 0
  PID: 355672
STDOUT -> 4207432  State: Active  Command: sleep 999
>
    ```
]
]

= Fichiers et redirections

Le mode debug affiche les nombres d'octets lus et écrits.

#figure(
  caption: [Démonstration des redirections vers des fichiers en avant plan],
)[
#sourcecode()[
```rs
> cat < ./projet/test
[read 42, wrote 42]
[read 0, wrote 0]
[Child 137265 exited]
#!/usr/bin/env bash

sleep 5
echo "Done!"
[Child 137264 exited]
> cat < ./projet/test > test
[read 42, wrote 42]
[read 0, wrote 0]
[Child 137384 exited]
[read 42, wrote 42]
[read 0, wrote 0]
[Child 137383 exited]
> [Child 137385 exited]

> cat test
#!/usr/bin/env bash

sleep 5
echo "Done!"
[Child 137434 exited]
```
]
]

L'exemple suivant montre que les redirections continuent de fonctionner en
arrière-plan et respectent l'état du processus qui les envoit ou reçoit. On
lance en arrière-plan un script dont la sortie est redirigée vers un fichier, on
peut le mettre en pause et le reprendre sans que la redirection ne s'arrête.
#figure(
  caption: [Démonstration des redirections vers des fichiers en arrière-plan (sans affichage
    debug)],
)[
#sourcecode()[
```rs
> cat ./projet/test_boucle
#!/usr/bin/env bash

for i in {1..50}; do
  echo "Iteration $i"
  sleep 3
done
echo "Done!"
> ./projet/test_boucle > sortie &
> cat sortie
Iteration 1
Iteration 2
> #on attend un peu
Error: command failed to execute
> cat sortie
Iteration 1
Iteration 2
Iteration 3
Iteration 4
> lj
Job 0
  PID: 153453
STDOUT -> 4  State: Active  Command: ./projet/test_boucle
> sj 0
> lj
Job 0
  PID: 153453
STDOUT -> 4  State: Suspended  Command: ./projet/test_boucle
> cat sortie
Iteration 1
Iteration 2
Iteration 3
Iteration 4
Iteration 5
Iteration 6
Iteration 7
> cat sortie
Iteration 1
Iteration 2
Iteration 3
Iteration 4
Iteration 5
Iteration 6
Iteration 7
> bg 0
> cat sortie
Iteration 1
Iteration 2
Iteration 3
Iteration 4
Iteration 5
Iteration 6
Iteration 7
Iteration 8
Iteration 9
```
]
]

= Tubes
#figure(caption: [Démonstration de tubes en avant-plan])[
#sourcecode()[
```rs
> ls | wc
[Child 176273 exited]
      9       9      62
[Child 176274 exited]
>
> cat flake.nix | grep pkgs | wc -l
[Child 178832 exited]
[Child 178833 exited]
7
[Child 178834 exited]
>
```
]
]

L'exemple suivant montre que l'information du descripteur de fichier vers lequel
la sortie est redirigée est stockée dans la liste des jobs.
#figure(caption: [Démonstration de tubes en arrière-plan])[
#sourcecode()[
```rs
>  ./projet/test_boucle | wc &
> lj
Job 0
  PID: 179824
STDOUT -> 4207432  State: Active  Command: ./projet/test_boucle
Job 1
  PID: 179825
STDOUT -> 12800864  State: Active  Command: wc
>      11      11      27
[Child 179824 exited]
[Child 179825 exited]
```
]
]
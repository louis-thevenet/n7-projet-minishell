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
#figure(caption: "On attend la fin de l'éxecution du fils pour passer à la prochaine commande")[
#sourcecode[```rust
> ls
flake.lock  flake.nix  projet  rapport	result	sujets	tp
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
#sourcecode(highlighted:(10,), highlight-color: lime)[
    ```rs
> cat ./projet/test
#!/usr/bin/env bash

sleep 5
echo "Done!"
> ./projet/test &
>
> ls
flake.lock  flake.nix  projet  rapport	result	sujets	tp
> Done!
>

    ```
]]

On peut également vérifier la bonne terminaison du fils après éxecution via `watch ps -sf` :
#figure(caption: "Durant l'exécution")[
#sourcecode(highlighted:(10,), highlight-color: lime)[
    ```rs
S+   pts/2      0:00      \_ /nix/store/cci0aml5v6xdvkqrvg-minishell/bin/minishell
S+   pts/2      0:00          \_ bash ./projet/test
S+   pts/2      0:00              \_ sleep 5

    ```
]]

#figure(caption: "Après exécution")[
#sourcecode(highlighted:(10,), highlight-color: lime)[
    ```rs
S+   pts/2      0:00      \_ /nix/store/cci0aml5v6xdvkqrvg-minishell/bin/minishell

    ```
]]


= Signaux
== Signal `SIGCHLD`
Un mode `debug` a été ajouté au projet afin d'afficher des informations sur les signaux reçus.

Dans la @sigchild, on :
- attend normalement la fin d'éxecution de la commande en avant-plan
- attend normalement la fin d'éxecution de la commande en arrière-plan
- envoie le signal `SIGCHLD` au processus fils
- envoie le signal `SIGSTOP` au processus fils
- envoie le signal `SIGCONT` au processus fils
- affiche les jobs en cours pour constater que le fils est continué en arrière-plan
- On teste ensuite les signaux `SIGSTOP` et `SIGCONT` sur un job en arrière-plan.

#figure(caption: "Démonstration `SIGCHLD`")[
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
STDOUT -> 4207432	State: Active	Command: sleep 9999
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
STDOUT -> 4207432	State: Active	Command: sleep 9999

Job 1
	PID: 335139
STDOUT -> 4207432	State: Active	Command: sleep 999
>


    ```
]  ]<sigchild>

== Signaux `SIGINT`, `SIGTSTP`

On voit dans cet exemple que le programme père reçoit le signal `SIGINT`, qu'il décide de tuer le fils en avant-plan, finalement le message informant la terminaison du processus fils est affiché.



#figure(caption: "Interruption au clavier")[
#sourcecode()[
    ```rs
> sleep 10
^C[SIGINT received]
[Killing 343628]
> [Child 343628 signaled]

>
    ```
]]


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
STDOUT -> 4207432	State: Suspended	Command: sleep 10
>
    ```
]]

Les processus fils quant à eux ne reçoivent pas les signaux `SIGINT` et `SIGTSTP`, c'est le père qui gère les interruptions.

#figure(caption: [Les processus fils masquent les signaux `SIGINT` et `SIGTSTP`])[
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
STDOUT -> 4207432	State: Active	Command: sleep 999
>
    ```
]]

= Fichiers et redirections

= Tubes
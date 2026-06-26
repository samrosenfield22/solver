# SWG-SOLVER

SWG-Solver is a general-purpose game solver engine!
My goal is to implement a fast, flexible game solver framework that
makes it easy to integrate new games.

## Features

* Flexible - can play multiple games (Nim, Tic-Tac-Toe, Connect 4,
	Quoridor is in progress)
* Interactive - terminal-based user interface with arrow-key control, chess-style clocks, evaluation and principal variation(s) console
* Powerful - uses a variety of solver-accelerating algorithms to
speed up the analysis, with many more in progress:
	* Alpha-beta pruning
	* Transposition table with zobrist hashing and two-big replacement policy
	* Iterative deepening
	* Move sorting with heuristics (PV move, forcing move, static move ordering)
	* Aspiration window
	* Early losing move avoidance

## Use
Compile with the given makefile, then run with the following syntax

No arguments - play an interactive game against the computer:
```bash
solver
```
Computer difficulty can be adjusted by setting the time handicap macro TIME_ODDS (in src/solver/play.c) - the higher the value, the less time the computer gets to think!

Solve a game from the initial position to a fixed depth:
```bash
solver 32
```

Solve a game from a given position, specified by a sequence of
moves, to a fixed depth:
```bash
solver 40 3,3,3,3,3,3
```



* /src/solver is a game solver engine. at the moment, it has
3 working games (nim, tic-tac-toe, and connect 4) and one game
in progress (quoridor)
to play, just compile and run the executable!
difficulty can be set in play.c by changing the value of
TIME_ODDS	(computer's clock time is divided by this value)

* /src/utils is a general-purpose utilities library, which
contains:
	/ds (data structures)
	/memory (memory allocators, memory function wrappers)
	/misc (various cool things)

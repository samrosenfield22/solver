README
-------

this folder contains two sub-projects:

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

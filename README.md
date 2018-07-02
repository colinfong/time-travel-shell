# Time Travel Shell
#### UCLA CS 111 Lab 1 Spring 14
###### Contributors: Stanway Liau, Colin Fong

### Overview
Part A deals with parsing a shell script within a small subset of the standard POSIX shell grammar.
Refer to the specification listed below for more details. Part B concerns building the execution
model for the shell. In Part C we parallelized the execution of shell commands with no dependencies.
The goal was to maintain 100% accuracy but not necessarily 100% parallelization.

### Features
- 1A) Shell script parser
  * Detects invalid grammar
  * Ignores comments
  * Supports detection for subshells
  * Print out a diagnostic with the '-p'flag

- 1B) Execution model
  - I/O redirections handled (within the specification)
  - Subshells executed appropriately

- 1C) Parallelization model
  - Turn on parallelization with the '-t' flag

### Limitations
- 1A) Shell script parser
  - Cannot handle append operator '>>'
  - Doesn't support the tokens '((' or '))'
  - Memory leaks galore

- 1B) Execution model
  - Doesn't support control flow statements such as 'if', 'while', etc.
  - Possibly not closing all opened file descriptors
  - Doesn't support the shell directive 'exec' properly
  - Will not properly handle commands like 'cd' or 'rm'

- 1C) Parallelization model
  - Doesn't parallelize within each command tree, only amongst command trees
  - Shell prompt may return before all shell commands print out their results
  - More memory leaks introduced

### Installation
1. Clone this repository using 'git clone https://github.com/stanwayl/time-travel-shell'
1. Create an executable 'timetrash' by running 'make'
1. Run shell using './timetrash [OPTION] script.sh' within the directory

### Ideas for future improvements
If we don't care too much about memory, unify all the different
node structures into a single one with all needed fields. Then
don't need to worry about type conversions and data structures
can just hold a single type of node. Also easier to just go
through them all and free them when done. 

### Specification
http://cs.ucla.edu/classes/spring14/cs111/assign/lab1.html

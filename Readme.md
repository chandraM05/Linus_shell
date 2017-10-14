The file shell.cpp is an application program used to implement simple command interpreter "shell" for unix.

Fuctionalities:
 -  Execution of simple commands.
 -  Shell builtins (cd ,pwd,export).
 -  Environment variables and echo command with some corner cases.
 -  Implementation of multiple pipes.
 -  Input and output redirection.
 -  Support for history and '!' operator.
 -  Handle Interrupt Signal.


Details:

 	Parsing :Detection of valid commands. Handling certain characters like "|" ,"<" and ">" whether they are part of input or shell 	operators.Executing valid commands.

	Shell builtins are handled separately with few cases. implementation of cd with pipes.

	Environment variables are handled using setenv and getenv system calls with few corner cases.

	implementation of multiple pipes using pipe(),dup1(),dup2(). 

	For input and output redirection ,creating new files,writing and appending to already existing files creating new if does't 		exist using open(), dup1() and dup2() system calls . 

	For History history_file.txt is created and a vector is used.All the commands executed will be stored in history_file.

	Implementation of bang operator.
	
	Handling ctrl-c Interrupt.

	Handling few error cases.
	

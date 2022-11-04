
/*
 * CS354: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>



#include "command.h"

void handler_kill(int sig){}
void handler_chld(int sig){
	std::fstream fs;
	fs.open("log.txt", std::fstream::app);
	fs << "Child terminated: ";
	fs << sig;
	fs << "\n";
	fs.close();
}

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char *argument)
{
	if (_numberOfAvailableArguments == _numberOfArguments + 1)
	{
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **)realloc(_arguments,
									  _numberOfAvailableArguments * sizeof(char *));
	}

	_arguments[_numberOfArguments] = argument;

	// Add NULL argument at the end
	_arguments[_numberOfArguments + 1] = NULL;

	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc(_numberOfSimpleCommands * sizeof(SimpleCommand *));

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	_doubleFile = 0;
}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{
	if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands)
	{
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **)realloc(_simpleCommands,
													_numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
	}

	_simpleCommands[_numberOfSimpleCommands] = simpleCommand;
	_numberOfSimpleCommands++;
}

void Command::clear()
{
	for (int i = 0; i < _numberOfSimpleCommands; i++)
	{
		for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
		{
			free(_simpleCommands[i]->_arguments[j]);
		}

		free(_simpleCommands[i]->_arguments);
		free(_simpleCommands[i]);
	}

	if (_outFile)
	{
		free(_outFile);
	}

	if (_inputFile)
	{
		free(_inputFile);
	}

	if (_errFile)
	{
		free(_errFile);
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	_doubleFile = 0;
	_isPipe = 0;
}

void Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");

	for (int i = 0; i < _numberOfSimpleCommands; i++)
	{
		printf("  %-3d ", i);
		for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
		{
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
		}
	}

	printf("\n\n");
	printf("  Output       Input        Error        Background\n");
	printf("  ------------ ------------ ------------ ------------\n");
	printf("  %-12s %-12s %-12s %-12s\n", _outFile ? _outFile : "default",
		   _inputFile ? _inputFile : "default", _errFile ? _errFile : "default",
		   _background ? "YES" : "NO");
	printf("_isPipe = %d",_isPipe);
	printf("\n\n");
}

void Command::execute()
{
	// Don't do anything if there are no simple commands
	if (_numberOfSimpleCommands == 0)
	{
		prompt();
		return;
	}

	// Print contents of Command data structure
	print();

	commandExecute();
	// Clear to prepare for next command
	clear();

	// Print new prompt
	prompt();
}

// Shell implementation

void Command::prompt()
{
	printf("myshell>");
	fflush(stdout);
}

int Command::commandExecute(void)
{
	pid_t pid;
	int j;
	std::string fun = _simpleCommands[0]->_arguments[0];
	if ( fun.compare("cd") == 0)
	{
		chdir(_simpleCommands[0]->_arguments[1]);
	}
	else if (_outFile && !_isPipe)
	{
		// Save default input, output, and error because we will
		// change them during redirection and we will need to restore them
		// at the end.
		// The dup() system call creates a copy of a file descriptor.
		int defaultin = dup(0);
		int defaultout = dup(1);
		int defaulterr = dup(2);

		//////////////////  ls  //////////////////////////

		// Input:    defaultin
		// Output:   file
		// Error:    defaulterr

		// Create file descriptor
		int outfd;
		if(_append == 1){
			outfd = open(_outFile,O_WRONLY | O_APPEND);
		}else {
			outfd = creat(_outFile, 0666);
		}
		if (outfd < 0)
		{
			perror("ls : create outfile");
			// exit( 2 );
		}
		// Redirect output to the created utfile instead off printing to stdout
		dup2(outfd, 1);
		close(outfd);

		// Redirect input
		dup2(defaultin, 0);

		// Redirect output to file
		dup2(outfd, 1);

		// Redirect err
		dup2(defaulterr, 2);
		char *argv[_simpleCommands[0]->_numberOfArguments + _doubleFile?2:1];
		for (int i = 0; i < _numberOfSimpleCommands; i++)
		{
			for (j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
			{
				argv[j] = _simpleCommands[i]->_arguments[j];
			}
		}
		if(_doubleFile){
			argv[j] = _inputFile;
			argv[j+1] = NULL;
		}else {
			argv[j] = NULL;
		}
		
		// Create new process for "ls"
		int pid = fork();
		if (pid == -1)
		{
			perror("ls: fork\n");
			// exit( 2 );
		}

		if (pid == 0)
		{
			// Child

			// close file descriptors that are not needed
			close(outfd);
			close(defaultin);
			close(defaultout);
			close(defaulterr);

			// You can use execvp() instead if the arguments are stored in an array
			int val = execvp(argv[0], argv);
			if (val == -1)
			{
				perror("ERROR");
			}
			exit(0);
			// exit( 2 );
		}

		// Restore input, output, and error

		dup2(defaultin, 0);
		dup2(defaultout, 1);
		dup2(defaulterr, 2);

		// Close file descriptors that are not needed
		close(outfd);
		close(defaultin);
		close(defaultout);
		close(defaulterr);

		// Wait for last process in the pipe line
		if(_background == 0){
			waitpid(pid, 0, 0);
		}
		
		// exit( 2 );
	}
	else if (_inputFile && !_isPipe )
	{
		
		// Save default input, output, and error because we will
		// change them during redirection and we will need to restore them
		// at the end.
		// The dup() system call creates a copy of a file descriptor.
		int defaultin = dup(0);
		int defaultout = dup(1);
		int defaulterr = dup(2);

		//////////////////  ls  //////////////////////////

		// Input:    defaultin
		// Output:   file
		// Error:    defaulterr

		// Create file descriptor
		int infd = open(_inputFile,O_RDONLY);

		if (infd < 0)
		{
			perror("ls : create outfile");
			// exit( 2 );
		}
		// Redirect output to the created utfile instead off printing to stdout
		dup2(infd, 0);

		// Redirect input
		dup2(defaultout, 1);

		// Redirect err
		dup2(defaulterr, 2);
		char *argv[_simpleCommands[0]->_numberOfArguments + 1];
		for (int i = 0; i < _numberOfSimpleCommands; i++)
		{
			for (j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
			{
				argv[j] = _simpleCommands[i]->_arguments[j];
			}
		}
		argv[j] = NULL;
		// Create new process for "ls"
		int pid = fork();
		if (pid == -1)
		{
			perror("ls: fork\n");
			// exit( 2 );
		}

		if (pid == 0)
		{
			// Child

			// close file descriptors that are not needed
			close(infd);
			close(defaultin);
			close(defaultout);
			close(defaulterr);

			// You can use execvp() instead if the arguments are stored in an array
			int val = execvp(argv[0], argv);
			if (val == -1)
			{
				perror("ERROR");
			}
			exit(0);
			// exit( 2 );
		}

		// Restore input, output, and error

		dup2(defaultin, 0);
		dup2(defaultout, 1);
		dup2(defaulterr, 2);

		// Close file descriptors that are not needed
		close(infd);
		close(defaultin);
		close(defaultout);
		close(defaulterr);

		// Wait for last process in the pipe line
		if(_background == 0){
			waitpid(pid, 0, 0);
		}

		// exit( 2 );
	}
	else if (_isPipe == 1)
	{
		// Save default input, output, and error because we will
		// change them during redirection and we will need to restore them
		// at the end.
		// The dup() system call creates a copy of a file descriptor.
		int defaultin = dup( 0 ); // Default file Descriptor for stdin
		int defaultout = dup( 1 ); // Default file Descriptor for stdout
		int defaulterr = dup( 2 ); // Default file Descriptor for stderr
		// Create new pipe 
		// Conceptually, a pipe is a connection between two processes, 
		// such that the standard output from one process becomes the standard input of the other process.
		// so if a process writes to fdpipe[1] process be can read from fdpipe[0]
		int fdpipe[(_numberOfSimpleCommands-1)*2];
		for (size_t i = 0; i < _numberOfSimpleCommands-1; i++)
		{
			if ( pipe(fdpipe + i*2) == -1) {
			perror( "cat_grep: pipe");
			exit( 2 );
			}
		}
		
					// Redirect input (use sdtin)
			dup2( defaultin, 0 );
				
				// Redirect output to pipe (write the output to pipefile[1] instead of stdout)
			dup2( fdpipe[1], 1 );

				// Redirect err (use stderr)
			dup2( defaulterr, 2 );

				// Redirect input for grep feed it with the output of cat which is writen by the child in fdpipe[1] 
				// so parent should read it from fdpipe[0]
			for (int i = 0; i < _numberOfSimpleCommands; i++){

				char *argv[_simpleCommands[i]->_numberOfArguments + 1];
				for (j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
				{
					argv[j] = _simpleCommands[i]->_arguments[j];
				}
				argv[j] = NULL;

				int pid = fork();
				if ( pid == -1 ) {
					perror( "cat_grep: fork\n");
					exit( 2 );
				}

				if (pid == 0) {
					for (size_t i = 0; i < _numberOfSimpleCommands-1; i++)
					{
						close(fdpipe[i*2]);
						close(fdpipe[i*2+1]);
					}
					close( defaultin );
					close( defaultout );
					close( defaulterr );

					execvp(argv[0], argv);
					perror( "cat_grep: exec cat");
					exit( 2 );
				}

				if (i == 0)
				{
					dup2( fdpipe[0] ,0);
				}

				if (i == _numberOfSimpleCommands - 2)
				{
					if (_outFile)
					{
						int outfd;
						if(_append == 1){
							outfd = open(_outFile,O_WRONLY | O_APPEND);
						}else {
							outfd = creat(_outFile, 0666);
						}
						if (outfd < 0)
						{
							perror("ls : create outfile");
							// exit( 2 );
						}
						// Redirect output to the created utfile instead off printing to stdout
						dup2(outfd, 1);
						close(outfd);
					}
					else
					{
						dup2( defaultout, 1 );
					}
				}

				if (i != 0)
				{
					dup2( fdpipe[i*2] ,0);
				}

				if (i != _numberOfSimpleCommands - 2)
				{
					dup2( fdpipe[(i+1)*2+1], 1 );
				}
				dup2( defaulterr, 2 );
			}
			// Restore input, output, and error
			dup2( defaultin, 0 );
			dup2( defaultout, 1 );
			dup2( defaulterr, 2 );
			// Close file descriptors that are not needed
			for (size_t i = 0; i < _numberOfSimpleCommands-1; i++)
			{
				close(fdpipe[i*2]);
				close(fdpipe[i*2+1]);
			}
			
			close( defaultin );
			close( defaultout );
			close( defaulterr );
			
			if(_background == 0){
			waitpid(pid, 0, 0);
		}
	}
	// int outputFile = _outFile?open(_outFile,O_CREAT | O_TRUNC | O_WRONLY):-1;
	// int inputFile = _inputFile?open(_inputFile, O_TRUNC | O_RDONLY):-1;
	// if(outputFile > 0){
	// 	dup2(outputFile,STDOUT_FILENO);
	// 	close(outputFile);
	// }
	// if(inputFile > 0){
	// 	dup2(inputFile,STDIN_FILENO);
	// 	close(inputFile);
	// }
	else
	{
		char *argv[_simpleCommands[0]->_numberOfArguments + 1];
		for (int i = 0; i < _numberOfSimpleCommands; i++)
		{
			for (j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
			{
				argv[j] = _simpleCommands[i]->_arguments[j];
			}
		}
		argv[j] = NULL;

		pid = fork();
		// printf("  %-3d ", pid );
		if (pid == -1)
		{
			return -1;
		}
		if (pid == 0)
		{
			int val = execvp(argv[0], argv);
			if (val == -1)
			{
				perror("ERROR");
			}
			exit(0);
		}
		else
		{
			if(_background == 0){
			waitpid(pid, 0, 0);
		}
			printf("Done with execvp\n");
		}
	}
	return 0;
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

int yyparse(void);

int main()
{
	signal(SIGINT, handler_kill);
	signal(SIGCHLD, handler_chld);
	Command::_currentCommand.prompt();
	yyparse();
	return 0;
}

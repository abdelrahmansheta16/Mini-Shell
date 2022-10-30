
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

#include "command.h"

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
	if (_outFile)
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
		int outfd = creat(_outFile, 0666);

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
	else if (_inputFile)
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
	Command::_currentCommand.prompt();
	yyparse();
	return 0;
}

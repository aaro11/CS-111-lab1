// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

int
command_status (command_t c)
{
  return c->status;
}

void
execute_command (command_t cmd, int time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
//  error (1, 0, "command execution not yet implemented");
	int status;
	pid_t child;
	int fd[2];
	switch (cmd->status)
	{
		case SIMPLE_COMMAND:
			child = fork();
			if (!child)
			{
				//handle redirects then execution
				int fd_in, fd_out;
				if (cmd->input)
				{
					if ((fd_in = open(cmd->input, O_RDONLY, 0666)) == -1)
						error(1, 0, "cannot open input file!");
					if (dup2(fd_in, 0) == -1)
						error(1, 0, "cannot do input redirect!");
				}
				if (cmd->output)
				{
					if ((fd_out = open(cmd->output, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1)
						error(1, 0, "cannot open output file!");
					if (dup2(fd_out, 0) == -1)
						error(1, 0, "cannot do output redirect!");
				}
				execvp(cmd->u.word[0], cmd->u.word);
				error(1,0,"cannot execute command");
			}
			else if (child > 0)
			{
				waitpid(child, &status, 0);
				cmd->status = status;
			}
			else
				error(1, 0, "cannot create child process!");
		case AND_COMMAND:
			//run left child command first, on its success, run right child command
			execute_command(cmd->u.command[0], time_travel);
			if (cmd->u.command[0]->status == 0)
			{
				//run second command cmd2
				execute_command(cmd->u.command[1], time_travel);
				cmd->status = cmd->u.command[1]->status;
			}
			else
				cmd->status = cmd->u.command[0]->status;
		case PIPE_COMMAND:
			//create 2 processes for each, redirect output of cmd1 to input of cmd2
			if (pipe(fd) == -1)
				error(1, 0, "cannot create pipe");
			child = fork();
			if (!child)
			{
				//child writes to pipe
				close(fd[0]);
				if (dup2(fd[1], STDOUT_FILENO) == -1)
					error(1, 0, "cannot redirect output");
				execute_command(cmd->u.command[0], time_travel);
				close(fd[1]);
				exit(cmd->u.command[0]->status);
			}
			else if (child > 0)
			{
				//parent reads the pipe
				waitpid(child, &status, 0);
				cmd->u.command[0]->status = status;
				close(fd[1]);
				if (dup2(fd[0], STDIN_FILENO) == -1)
					error(1, 0, "cannot redirect input");
				execute_command(cmd->u.command[1], time_travel);
				close(fd[0]);
			}
			else
				error(1, 0, "could not create child process!");
	}
}

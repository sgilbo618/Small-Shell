/*******************************************************************************
 * Name: Samantha Guilbeault
 * Date: February 13, 2020
 * Description: A small shell program that runs command line instructions and 
 * 				returns results simailar to bash. It allows for redirection of
 * 				stdin and stdout, supports foreground and background processes,
 * 				built in commands (for exit, cd, and status), comments, and uses
 * 				signal handling for SIGINT and SIGTSTP.
 * 				SIGINT - will terminate only the foreground command if one is 
 * 						running.
 * 				SIGTSTP - will toggle foreground only mode and normal mode.
*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "dynArray.h"

// Constants
#define MAX_ARGS 513
#define MAX_INPUT 2048
#define NUM_BLT_INS 3

// Globals
char *BUILT_INS[NUM_BLT_INS] = {"exit", "cd", "status"};
bool IS_FOREGROUND_ONLY = false;

// Struct for holding exit/termination status
struct Status
{
	int exitStatus;
	int termStatus;
};

// Prototypes
bool is_empty(char *command);
void parse_args_to_arr(char *line, char *arguments[], int *numArgs);
void free_args_memory(char *arguments[], int numArgs);
bool is_built_in(char *command);
void execute_built_in(char *arguments[], DynArr *cpids, struct Status lastStatus);
void my_exit(DynArr *cpids);
void my_cd(char *path);
void my_status(struct Status lastStatus);
void check_exit_status(struct Status *lastStatus, int childExitMethod);
void execute(char *arguments[], int *numArgs, bool isBackground);
void check_for_redirect(char *arguments[], int *numArgs, bool isBackground);
int find_symbol(char *arguments[], char *symbol);
void expand_variable(char *lookIn, char *lookFor);
bool check_for_background_command(char *arguments[], int *numArgs);
void check_for_background_complete(DynArr *cipds);
void catch_SIGTSTP(int signo);


/*******************************************************************************
 * Function: main()
 * Description: Driving function for the shell program. It sets up the signal
 * 				handling, tracking child PIDs, tracking exit status, getting
 * 				user input, executing built in commands, executing other commands,
 * 				and tracking foreground-only vs normal mode.
*******************************************************************************/
void main()
{
	// Source: 3.3 Advanced User Input with getline()
	// SIGINT Default - used by foreground child processes
	struct sigaction SIGINT_action = {0}; // init struct to 0
	SIGINT_action.sa_handler = SIG_DFL; // set function
	sigfillset(&SIGINT_action.sa_mask); // fill mask with all signals
	SIGINT_action.sa_flags = 0;

	// SIGINT Ignore - used by shell and background processes
	struct sigaction ignore_action = {0};
	ignore_action.sa_handler = SIG_IGN;

	// SIGTSTP - toggle foreground-only mode
	struct sigaction SIGTSTP_action = {0};
	SIGTSTP_action.sa_handler = catch_SIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = 0;
	
	// Set sigactions
	sigaction(SIGINT, &ignore_action, NULL);
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	// Set up signal set for blocking in foreground child
	sigset_t signal_set;
	sigemptyset(&signal_set);
	sigaddset(&signal_set, SIGTSTP);

	// Create dynamic array to store child PIDs
	DynArr *cpids;
	cpids = newDynArr(10);


	// Store status of last foreground process
	struct Status lastStatus = {0, -5}; // init with exit=0


	// Buffer setup
	// source: 3.3 Advanced User Input with getline()
	int numCharsEntered = -5;
	int currChar = -5;
	size_t bufferSize = 0;
	char *lineEntered = NULL;
	
	// Main shell loop
	while(1)
	{
		// Will display PIDs of background processes completed since last loop
		check_for_background_complete(cpids);

		// Get input
		do
		{
			printf(": ");
			fflush(stdout);
			numCharsEntered = getline(&lineEntered, &bufferSize, stdin);
			
			if (numCharsEntered == -1) // getline returns -1 if interrupted
			{	
				lineEntered = NULL;
				clearerr(stdin);
			}
			else if (numCharsEntered > MAX_INPUT) // only allow input up to 2048 chars
			{
				free(lineEntered);
				lineEntered = NULL;
				printf("Error - exceeded max input\n");
				fflush(stdout);
			}
		// Keep getting input if line is empty or comment
		} while (lineEntered == NULL || is_empty(lineEntered));
		

		// Expand $$ to PID
		expand_variable(lineEntered, "$$");
		

		// Handle args
		char *arguments[MAX_ARGS]; // Array to store arguments
		int numArgs = 0;
		parse_args_to_arr(lineEntered, arguments, &numArgs);


		// Check for built in commands
		if(is_built_in(arguments[0]))
		{
			execute_built_in(arguments, cpids, lastStatus);
		}

		// Otherwise use command execution
		else
		{
			pid_t spawnPid = -5;
			int childExitMethod = -5;
			bool isBackground = false;
			
			// Check global state to see if background needs to be ignored
			if (IS_FOREGROUND_ONLY)
				check_for_background_command(arguments, &numArgs); // To remove &
			else
				isBackground = check_for_background_command(arguments, &numArgs);


			// Create fork
			spawnPid = fork();
			switch (spawnPid)
			{
				// ERROR in fork
				case -1:
					perror("Unable to create fork");
					exit(1);
					break;

				// Child
				case 0:
					// Foreground process only
					if (!isBackground)
					{
						sigaction(SIGINT, &SIGINT_action, NULL); // Set SIGINT to default
					}

					// All child ignore SIGTSP
					sigaction(SIGTSTP, &ignore_action, NULL);

					execute(arguments, &numArgs, isBackground);
					break;
				
				// Parent
				default:

					// Run in background
					if (isBackground)
					{
						// Track child pid and don't wait
						addDynArr(cpids, spawnPid);
						printf("background pid is %d\n", spawnPid);
						fflush(stdout);
					}

					// Run in foreground
					else
					{
						
						sigprocmask(SIG_BLOCK, &signal_set, NULL); // Block SIGTSTP
						
						// Wait for child and then update status
						int result;
						do
						{
							result = waitpid(spawnPid, &childExitMethod, 0);
						// Reset waitpid if interrupted by system call
						} while (result == -1 && errno == EINTR);

						sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
					
						// Update status
						check_exit_status(&lastStatus, childExitMethod);
						
						// Let user know if foreground process was terminated
						if (WIFSIGNALED(childExitMethod) != 0)
						{
							printf("terminated by signal %d\n", lastStatus.termStatus);
							fflush(stdout);
						}
					}
			}
		}
			
		// Free memory for input
		free_args_memory(arguments, numArgs);
		free(lineEntered);
		lineEntered = NULL;
	}
	// Free memory for pids
	deleteDynArr(cpids);
}


/*******************************************************************************
 * Function: catchSIGTSTP(int signo)
 * Description: Signal handler function for SIGTSTP when user enters CTRL-Z. It
 * 				toggles the global variable IS_FOREGROUND_ONLY to change the 
 * 				mode of the shell. Also displays a message to the user.
*******************************************************************************/
void catch_SIGTSTP(int signo)
{
	// In foreground only mode
	if (IS_FOREGROUND_ONLY)
	{
		char *message = "Exiting foreground-only mode\n";
		write(STDOUT_FILENO, message, 29);
		IS_FOREGROUND_ONLY = false;
	}
	// In normal mode
	else
	{
		char *message = "Entering foreground-only mode (& is now ignored)\n";
		write(STDOUT_FILENO, message, 49);
		IS_FOREGROUND_ONLY = true;
	}
}


/*******************************************************************************
 * Function: check_for_background_complete(DynArr *cpids)
 * Description: Takes in an array of the background child process pids. Loops
 * 				through them to see if any of them have terminated. If they have
 * 				they are removed from the array and a message is displayed to 
 * 				the user.
********************************************************************************/
void check_for_background_complete(DynArr *cpids)
{
	int size = sizeDynArr(cpids); // Max times to run loop
	int count = 0; // To track number of times loop has run
	int i = 0; // To track index of cpids array

	// Loop while cpids is not empty and up until size
	while (!isEmptyDynArr(cpids) && count < size)
	{
		// Check if each process has completed
		int childExitMethod = -5;
		int result;
		result = waitpid(getDynArr(cpids, i), &childExitMethod, WNOHANG);
		
		// If it has completed
		if (result > 0)
		{
			// Remove it from cpids
			removeAtDynArr(cpids, i);
			i--; // To offset losing an item

			// Print either exit status or termination signal
			printf("background pid %d is done: ", result);
			if (WIFEXITED(childExitMethod) != 0)
				printf("exit value %d\n", WEXITSTATUS(childExitMethod));
			else if (WIFSIGNALED(childExitMethod) != 0)
				printf("terminated by signal %d\n", WTERMSIG(childExitMethod));
			fflush(stdout);
		}
		count++;
		i++;
	}
}


/*******************************************************************************
 * Function: check_for_background_command(char *arguments[], int *numArgs)
 * Description: Takes in an array of strings that represent the user entered
 * 				args and a pointer to the number of args in the array. It looks
 * 				up the last position of the array and compartes it to "&" to 
 * 				check if this command is meant to be a background command. Will
 * 				NULL out the & and return true if found, otherwise returns false.
********************************************************************************/
bool check_for_background_command(char *arguments[], int *numArgs)
{
	// Look at last arg in list to see if it is &
	if (strcmp(arguments[(*numArgs)-1], "&") == 0)
	{
		// Free the memory for &
		free(arguments[(*numArgs)-1]);
		arguments[(*numArgs)-1] = NULL;
		(*numArgs)--; // Update numArgs

		return true;
	}
	// Not background command
	return false;
}


/*******************************************************************************
 * Function: expand_variables(char *lookIn, char *lookFor)
 * Source: stackoverflow.com/questions/32413667
 * Description: Takes in a string to look in and a string to look for. Uses 
 * 				strstr to look for occurances of the lookFor sting inside of the
 * 				lookIn string. If found, the loofFor portion is removed and 
 * 				replaces by the PID of the process.
********************************************************************************/
void expand_variable(char *lookIn, char *lookFor)
{	
	// Get PID and convert to string
	int pid = getpid();
	char strPID[8];
	snprintf(strPID, sizeof(strPID), "%d", pid);
		
	char buffer[2100] = { 0 }; // For piecing string back together
	char *tmpLookIn = lookIn; // For traversing lookIn string

	// Keep going until all lookFors are converted
	while((tmpLookIn = strstr(tmpLookIn, lookFor))) // Will be NULL if no matches
	{
		// Copy up until found lookin
		strncpy(buffer, lookIn, tmpLookIn-lookIn);
		buffer[tmpLookIn-lookIn] = '\0';
		
		// Add on the PID
		strcat(buffer, strPID);

		// Add on the rest of string, after lookFor
		strcat(buffer, tmpLookIn+strlen(lookFor));	
		
		// Copy re-built string into lookIn
		strcpy(lookIn, buffer);
		
		tmpLookIn++;
	}
}


/*******************************************************************************
 * Function: find_symbol(char *arguments[], char *symbol)
 * Description: Takes in an array of strings and a string to search for. Loops
 * 				throught the array and if the symbol is found, its position in
 * 				the array is returned. Other wise -10 is returned.
********************************************************************************/
int find_symbol(char *arguments[], char *symbol)
{
	int position = -10;
	int i = 0;
	while (arguments[i] != NULL) // Arguments last position is NULL
	{
		// Found the symbol in args, so return the position it is at
		if (strcmp(arguments[i], symbol) == 0)
		{
			position = i;
			break;
		}
		i++;
	}
	return position;
}


/*******************************************************************************
 * Function: check_for_redirect(char *arguments[], int* numArgs, bool isBackground)
 * Description: Takes in an array of arguments from the user and a Boolean to 
 * 				represent whether or not this is for a background process. It
 * 				searches the args to determine is a redirection is commanded and
 * 				sets up the redirction to the designated files if it is. If no
 * 				redirection is implicated for a background process, then it sets
 * 				up redirection to dev/null.
********************************************************************************/
void check_for_redirect(char *arguments[], int *numArgs, bool isBackground)
{
	int pos1 = find_symbol(arguments, "<");	// STDIN
	int pos2 = find_symbol(arguments, ">"); // STDOUT

	// Found redirection for stdin or background
	if (pos1 >= 0 || isBackground)
	{
		int sourceFile;

		// Open file for input with next arg
		if (pos1 >= 0)
			sourceFile = open(arguments[pos1+1], O_RDONLY);
		// Open dev/null for background
		else
			sourceFile = open("/dev/null", O_RDONLY);
		
		if (sourceFile == -1)
		{
			printf("cannont open %s for input\n", arguments[pos1+1]);
			fflush(stdout);
			exit(1);
		}
		
		// Close on exec
		fcntl(sourceFile, F_SETFD, FD_CLOEXEC);
		
		// Redirect input to given file
		int inResult = dup2(sourceFile, 0);
		if (inResult == -1)
		{
			printf("error in dup2() redirect for input\n");
			fflush(stdout);
			exit(2);
		}

		// NULL out pos so arguments will end with a NULL for exec()
		if (pos1 >= 0)
		{	
			free(arguments[pos1]);
			arguments[pos1] = NULL;
			(*numArgs)--;

			free(arguments[pos1+1]);
			arguments[pos1+1] = NULL;
			(*numArgs)--;
		}
	}

	// Found redirection for stdout or background
	if (pos2 >= 0 || isBackground)
	{
		int targetFile;

		// Open file for output with next arg
		if (pos2 >= 0)
			 targetFile = open(arguments[pos2+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
		// Open dev/null for background
		else
			targetFile = open("/dev/null", O_WRONLY);

		if (targetFile == -1)
		{
			printf("cannot open %s for output\n", arguments[pos2+1]);
			fflush(stdout);
			exit(1);
		}

		// Close on exec
		fcntl(targetFile, F_SETFD, FD_CLOEXEC);

		// Redirect output to given file
		int outResult = dup2(targetFile, 1);
		if (outResult == -1)
		{
			printf("error in dup2() redirect for output\n");
			fflush(stdout);
			exit(2);
		}

		// NULL out pos	
		if (pos2 >= 0)
		{
			free(arguments[pos2]);
			arguments[pos2] = NULL;
			(*numArgs)--;

			free(arguments[pos2+1]);
			arguments[pos2+1] = NULL;
			(*numArgs)--;
		}
	}
}


/*******************************************************************************
 * Function: execute(char *arguments[], int *numArgs, bool isBackground)
 * Description: Takes in an array of user entered arguments and a Boolean that is
 * 				a flag for background processes. Passes the args to set up any
 * 				redirection and calls execvp() to execute the desired process.
********************************************************************************/
void execute(char *arguments[], int *numArgs, bool isBackground)
{
	check_for_redirect(arguments, numArgs, isBackground);

	// Create the new process
	if (execvp(arguments[0], arguments) < 0)
	{
		printf("%s: command not found\n", arguments[0]);
		fflush(stdout);
		exit(1);
	}
}


/*******************************************************************************
 * Function: check_exit_status(struct Status *lastStatus, int childExitMethod)
 * Description: Takes in the struct Status that holds either the exit status or
 * 				termination signal number of the last foreground process and the
 * 				int returned from the last child process executed. The int is
 * 				used to determine whether the child process exited or was 
 * 				terminated with a signal. The values held by the struct are set
 * 				to reflect the findings.
********************************************************************************/
void check_exit_status(struct Status *lastStatus, int childExitMethod)
{
	// Process terminated normally, so set the exit status
	if (WIFEXITED(childExitMethod) != 0)
	{
		lastStatus->exitStatus = WEXITSTATUS(childExitMethod);
		lastStatus->termStatus = -100; // Reset termination status
	}
	// Process terminated with signal termination, so set the termination status
	else if (WIFSIGNALED(childExitMethod) != 0)
	{
		lastStatus->termStatus = WTERMSIG(childExitMethod);
		lastStatus->exitStatus = -100; // Reset exit status
	}
}


/*******************************************************************************
 * Function: my_status(struct Status lastStatus)
 * Description: Takes in a struct status that holds either the last exit status
 * 				or the last terminating signal number. The exit status being set
 * 				to -100 indicates that the last process was terminated, otherwise
 * 				the last process was exited. The corresponding message will be
 * 				displayed to the user.
********************************************************************************/
void my_status(struct Status lastStatus)
{
	// The last process exited
	if (lastStatus.exitStatus != -100)
	{
		printf("exit value %d\n", lastStatus.exitStatus);
		fflush(stdout);
	}
	// The last process was terminated
	else
	{
		printf("terminated by signal %d\n", lastStatus.termStatus);
		fflush(stdout);
	}
}


/*******************************************************************************
 * Function: my_cd(char *path)
 * Description: Takes in a string for the path attempting to cd to. If the path
 * 				is not specified, the directory is changed to the HOME directory,
 * 				otherwise the directory is changed to the passed in path.
********************************************************************************/
void my_cd(char *path)
{
	// No path specified, so move to home directory
	if (path == NULL)
	{
		char *home = getenv("HOME");
		chdir(home);
	}
	// Move to specified path
	else
	{
		chdir(path);
	}
}


/*******************************************************************************
 * Function: my_exit(DynArr *cpids)
 * Description: Takes in an array of child pids. Loops through the pids to send
 * 				SIGTERM signal to each one before exiting the program.
********************************************************************************/
void my_exit(DynArr *cpids)
{
	// Loop through child pids to kill all child process
	int size = sizeDynArr(cpids);

	for(int i = 0; i < size; i++)
	{
		kill(getDynArr(cpids, i), SIGKILL);
	}
	
	exit(0);
}


/*******************************************************************************
 * Function: execute_built_in(char *arguemtns[], DynArr *cpids)
 * Description: Takes in an array of user inputted arguments in which the first 
 * 				argument is a built in command, an array of child pids, and an
 *				int that represents the either the exit status or terminating
 *				signal number of the last ran foreground process.
 * 				Uses the first argument from the user to determine which built
 * 				in command to run. 
 * Currently supports EXIT, CD, and STATUS
********************************************************************************/
void execute_built_in(char *arguments[], DynArr *cpids, struct Status lastStatus)
{
	// EXIT
	if(strcmp(arguments[0], "exit") == 0)
		my_exit(cpids);

	// CD
	else if(strcmp(arguments[0], "cd") == 0)
		my_cd(arguments[1]); // Pass in next arg
	
	// STATUS
	else if(strcmp(arguments[0], "status") == 0)
		my_status(lastStatus);
}


/*******************************************************************************
 * Function: is_built_in(char *command)
 * Description: Takes in the first argument from the user input then checks to
 * 				see if it is one of the built-in commands. If it is found in
 * 				the built in list it returns true, otherwise it returns false.
********************************************************************************/
bool is_built_in(char *command)
{
	for(int i = 0; i < NUM_BLT_INS; i++)
	{
		if(strcmp(BUILT_INS[i], command) == 0)
			return true;
	}
	return false;
}


/*******************************************************************************
 * Function: free_args_memory(char *arguments[], int numArgs)
 * Description: Takes in the arguments array and its size. Loops through to free
 * 				all the allocated memory.
********************************************************************************/
void free_args_memory(char *arguments[], int numArgs)
{
	for(int i = 0; i < numArgs; i++)
	{
		free(arguments[i]);
		arguments[i] = NULL;
	}
}


/*******************************************************************************
 * Function: parse_args_to_arr(char *line, char *arguments[], int *numArgs)
 * Description: Takes in the user entered string of arguments, an empty array of
 * 				pointers to chars, and a pointer to the number of elements in the
 * 				array.
 * 				Uses strtok() to parse the string by spaces, then copies each
 * 				string into the arguments array.
********************************************************************************/
 void parse_args_to_arr(char *line, char *arguments[], int *numArgs)
 {
	// Remove trailing new line from getline and add null terminator
	line[strcspn(line, "\n")] = '\0';

	// Use strtok to parse line at spaces
	char *token = strtok(line, " ");
	while (token != NULL) // Keep parsing until there are no strings left
	{
		// Allocate memory and copy into args array
		arguments[*numArgs] = malloc((strlen(token)+1) * sizeof(char));
		strcpy(arguments[*numArgs], token);
		(*numArgs)++;

		// Get next string
		token = strtok(NULL, " ");
	}

	// Set last spot in array to NULL
	arguments[*numArgs] = NULL;
 }


/********************************************************************************
 * Function: is_empty(char *command)
 * Description: Takes in the string command from getline and checks if it is an
 * 				empty string or a comment. Returns true if it is, otherwise it
 * 				returns false.
********************************************************************************/
bool is_empty(char *command)
{
	if (command[0] == '\n' || command[0] == '#')
		return true;
	else
		return false;
}

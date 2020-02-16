/*******************************************************************************
 * Name: Samantha Guilbeault
 * Date: February 13, 2020
 * Assignment: Program 3 - smallsh
 * Description:
*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>


// Constants
#define MAX_ARGS 513
#define NUM_BLT_INS 3
char *BUILT_INS[NUM_BLT_INS] = {"exit", "cd", "status"};

// Prototypes
bool is_empty(char *command);
void parse_args_to_arr(char *line, char *arguments[], int *numArgs);
void free_args_memory(char *arguments[], int numArgs);
bool is_built_in(char *command);
void execute_built_in(char *arguments[]);
void my_exit();
void my_cd(char *path);
void my_status();


/*******************************************************************************
 * Function: catchSIGINT(int signo)
 * Source: 3.3 Advanced User Input with getline()
 * Description:
*******************************************************************************/
void catch_SIGINT(int signo)
{
	char *message = "SIGIN. Use CTRL-Z to Stop.\n";
	write(STDOUT_FILENO, message, 28);
}


/*******************************************************************************
 * Function: main()
 * Description:
*******************************************************************************/
void main()
{
	// Create and set sigaction
	// Source: 3.3 Advanced User Input with getline()
	struct sigaction SIGINT_action = {0}; // init struct to 0
	SIGINT_action.sa_handler = catch_SIGINT; // set function
	sigfillset(&SIGINT_action.sa_mask); // fill mask with all signals
	sigaction(SIGINT, &SIGINT_action, NULL); // set sigaction

	// Buffer setup
	// source: 3.3 Advanced User Input with getline()
	int numCharsEntered = -5;
	int currChar = -5;
	size_t bufferSize = 0;
	char *lineEntered = NULL;
	
	// Set up input loop
	// Source: 3.3 Advanced User Input with getline()
	while(1)
	{
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
		// Keep getting input if line is empty or comment
		} while (lineEntered == NULL || is_empty(lineEntered));

		// Create array and fill with parsed arguments
		char *arguments[MAX_ARGS]; // Array to store arguments
		int numArgs = 0;
		parse_args_to_arr(lineEntered, arguments, &numArgs);

		if(is_built_in(arguments[0]))
		{
			execute_built_in(arguments);
		}
			

		// Free memory
		free_args_memory(arguments, numArgs);
		free(lineEntered);
		lineEntered = NULL;
	}
}


/*******************************************************************************
 * Function:
 * Description:
********************************************************************************/

/*******************************************************************************
 * Function:
 * Description:
********************************************************************************/
void my_status()
{
	printf("in status\n");
}


/*******************************************************************************
 * Function:
 * Description:
********************************************************************************/
void my_cd(char *path)
{
	printf("in cd\n");
}


/*******************************************************************************
 * Function:
 * Description:
********************************************************************************/
void my_exit()
{
	printf("in exit\n");
}


/*******************************************************************************
 * Function:
 * Description:
********************************************************************************/
void execute_built_in(char *arguments[])
{
	// EXIT
	if(strcmp(arguments[0], "exit") == 0)
		my_exit();

	// CD
	else if(strcmp(arguments[0], "cd") == 0)
		my_cd(arguments[1]); // Pass in next arg
	
	// STATUS
	else if(strcmp(arguments[0], "status") == 0)
		my_status();
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





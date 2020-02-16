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

// Prototypes
bool isEmpty(char *command);
void parseArgstoArr(char *line, char *arguments[], int *numArgs);
void freeArgsMemory(char *arguments[], int numArgs);


/*******************************************************************************
 * Function: catchSIGINT(int signo)
 * Source: 3.3 Advanced User Input with getline()
 * Description:
*******************************************************************************/
void catchSIGINT(int signo)
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
	SIGINT_action.sa_handler = catchSIGINT; // set function
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
				lineEntered = NULL;
				clearerr(stdin);

		// Keep getting input if line is empty or comment
		} while (lineEntered == NULL || isEmpty(lineEntered));

		// Create array and fill with parsed arguments
		char *arguments[MAX_ARGS]; // Array to store arguments
		int numArgs = 0;
		parseArgstoArr(lineEntered, arguments, &numArgs);



		// Free memory
		freeArgsMemory(arguments, numArgs);
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
void freeArgsMemory(char *arguments[], int numArgs)
{
	for(int i = 0; i < numArgs; i++)
	{
		free(arguments[i]);
		arguments[i] = NULL;
	}
}


/*******************************************************************************
 * Function: parseArgstoArr(char *line, char *arguments[], int *numArgs)
 * Description: Takes in the user entered string of arguments, an empty array of
 * 				pointers to chars, and a pointer to the number of elements in the
 * 				array.
 * 				Uses strtok() to parse the string by spaces, then copies each
 * 				string into the arguments array.
********************************************************************************/
 void parseArgstoArr(char *line, char *arguments[], int *numArgs)
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
 * Function: isEmpty(char *command)
 * Description: Takes in the string command from getline and checks if it is an
 * 				empty string or a comment. Returns true if it is, otherwise it
 * 				returns false.
********************************************************************************/
bool isEmpty(char *command)
{
	if (command[0] == '\n' || command[0] == '#')
		return true;
	else
		return false;
}





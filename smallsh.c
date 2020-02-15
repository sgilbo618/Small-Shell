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


// Prototypes
bool isEmpty(char *command);



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
				clearerr(stdin);

		// Keep getting input if line is empty or comment
		} while (isEmpty(lineEntered));

		// Remove trailing new line from getline and add null terminator
		lineEntered[strcspn(lineEntered, "\n")] = '\0';
		printf("Here is the cleaned line: \"%s\"\n", lineEntered);

		char *token = strtok(lineEntered, " ");
		while (token != NULL)
		{
			printf("%s\n", token);
			token = strtok(NULL, " ");
		}

		free(lineEntered);
		lineEntered = NULL;
	}
}


/*******************************************************************************
 * Function:
 * Description:
********************************************************************************/


/*******************************************************************************
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





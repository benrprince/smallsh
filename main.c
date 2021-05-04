/************************************************************************
    Author: Ben Prince
    Assignment 3: Smallsh
    Date: 05/03/2021
Sources:
https://www.tutorialspoint.com
https://www.youtube.com/channel/UC6qj_bPq6tQ6hLwOBpBQ42Q
https://www.youtube.com/user/jms36086
*************************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

// Global Variables
#define MAX_CHARS 2048
#define MAX_ARGS 512
int exitStat = 0;
int next = 1;
int childStatus = -5;
// Variables to manage background status
int background = 0;
int fgOnlyMode = 0;
// Variables to manage background PIDs
int bgProc[100];
int bgCount = 0;
// Structs for catching signals ^C and ^Z
struct sigaction SIGINT_action = {0};
struct sigaction SIGTSTP_action = {0};



void clearArray(char** arguments, int argCount) {
    /***********************************************
    *   Params: char** argument array, int arg count
    *   Returns: NA
    *   Desc: Clears argument array for next command
    ***********************************************/

    for (int i = 0; i < argCount; i++) {
        arguments[i] = NULL;
    }

}

void exitStatus(int stat) {
    /***********************************************
    *   Params: int child status
    *   Returns: NA
    *   Desc: Prints out the exit status
    ***********************************************/

    if (WIFEXITED(stat)) {
		printf("exit value %d\n", WEXITSTATUS(stat));
        fflush(stdout);
	} 
    
    else {

        printf("terminated by signal %d\n", WTERMSIG(stat));
        fflush(stdout);
    }
}

void redirectCheck(char** arguments, int argCount) {
    /***********************************************
    *   Params: char** argument array, int arg count
    *   Returns: NA
    *   Desc: Checks for redirect "<" or ">" and handles
    *         whatever redirection is specified
    ***********************************************/

    int targetFD;
    char* sourceFD;
    int redirected = 0;

    for (int i = 1; i < argCount; i++) {
        // if < redirect stdout
        if (strcmp(arguments[i], "<") == 0) {
            redirected = 1;
            sourceFD = strdup(arguments[i+1]);

            targetFD = open(sourceFD, O_RDONLY);
            if (targetFD == -1) {
                printf("Cannot open file\n");
                fflush(stdout);
                exit(1);
            }
            if (dup2(targetFD, STDIN_FILENO) == -1) {
                printf("Error");
                fflush(stdout);
                exit(1);
            }
            close(targetFD);
            free(sourceFD);

        }
        // if > redirect stdin
        else if (strcmp(arguments[i], ">") == 0) {
            redirected = 1;
            sourceFD = strdup(arguments[i+1]);

            targetFD = open(sourceFD, O_CREAT | O_RDWR | O_TRUNC, 0666);
            if (targetFD == -1) {
                printf("Cannot open file\n");
                fflush(stdout);
                exit(1);
            }
            if (dup2(targetFD, STDOUT_FILENO) == -1) {
                printf("Error");
                fflush(stdout);
                exit(1);
            }
            close(targetFD);
            free(sourceFD);

        }
    }
    
    // if there was redirection remove commands except for the first command
    if (redirected != 0) {

        for(int i = 1; i < argCount; i++) {
			arguments[i] = NULL;
        }

    }

}


void newProcess(char** arguments, int argCount) {
    /***********************************************
    *   Params: char** argument array, int arg count
    *   Returns: NA
    *   Desc: Forks into child process.
    ***********************************************/

    pid_t spawnpid = fork();

    if (spawnpid < 0) {

        printf("Fork Failed\n");
        fflush(stdout);
        exit(1);
    
    }

    else if (spawnpid == 0) {

        // Reset Ctrl-C as the default for child process
        SIGINT_action.sa_handler = SIG_DFL;
		sigaction(SIGINT, &SIGINT_action, NULL);

        // Ignore Ctrl-Z
        SIGTSTP_action.sa_handler = SIG_IGN;
		sigaction(SIGTSTP, &SIGTSTP_action, NULL);
        printf("%s", arguments[0]);

        // Check for redirect and handle
        redirectCheck(arguments, argCount);

        execvp(arguments[0], arguments);
        printf("Error executing command\n");
        fflush(stdout);
        exit(0);

    }

    else {
        // Not in background mode so parent waits
        if (background == 0) {

            waitpid(spawnpid, &childStatus, 0);

        }

        else {
            // In background mode so parent doesn't wait
            // and spawnpid is saved in the background array
            bgProc[bgCount] = spawnpid;
            bgCount++;
            waitpid(spawnpid, &childStatus, WNOHANG);
            printf("Background pid: %d\n", spawnpid);
            fflush(stdout);

        }
    }

    fflush(stdout);

}


void builtInCommands(char** arguments, int flag, int argCount) {
    /***********************************************
    *   Params: char** argument array, int cd/status flag, int arg count
    *   Returns: NA
    *   Desc: Runs the built in commands status and cd 
    ***********************************************/

    
    switch (flag) {

        case 1: // cd

            // cd by itself
            if (argCount == 1) {
                chdir(getenv("HOME"));
            }
            // cd with arguments
            else {

                chdir(arguments[1]);

            }

            break;

        default: // status
            
            exitStatus(exitStat);
        
    }

    clearArray(arguments, argCount);

}

int getArguments(char** arguments, char input[]) {
    /***********************************************
    *   Params: char** argument array, char [] input string
    *   Returns: int, number of arguments parsed
    *   Desc: Uses strtok_r to parse each argument into
    *         arguments array.    
    ***********************************************/

    char *originalInput = strdup(input);
    char *savePTR = originalInput;
    char *token;
    int count = 0;

    // Parse input into separate arguments 
    while((token = strtok_r(savePTR, " \n", &savePTR))) {

        arguments[count] = token;
        count++;
    }

    return count;

}


void userInput() {
    /***********************************************
    *   Params: NA
    *   Returns: NA
    *   Desc: Gets input from the user and calls getArguments()
    *         to put them into an array. Checks for Built in
    *         commands. If those aren't found sends to exec()   
    ***********************************************/

    char *arguments[MAX_ARGS];
    char input[MAX_CHARS];
    int argCount;

    // Get input from user and remove new line char
    printf(": ");
    fflush(stdout);
    fgets(input, sizeof(input), stdin);
    // Source: https://www.tutorialspoint.com/c_standard_library/c_function_strcspn.htm
    input[strcspn(input, "\n")] = '\0';

    // Check for $$ for variable expansion
	for (int i = 0; i < strlen(input); i ++) {
		// Remove $$ and add pid
		if ( (input[i] == '$') && (i + 1 < strlen(input)) && (input[i + 1] == '$') ) {

            char buffer[100];
            input[i] = '\0';

            strcpy(buffer, input);
            strcat(buffer, "%d");
			sprintf(input, buffer, getpid());

		}
	}
    
    // Check for empty entry
    if (strcmp(input, "") == 0) {
        
        return;

    }

    // Set argument array and get number of arguments
    argCount = getArguments(arguments, input);

    // Check for background process, if foreground mode is not on
    if (strncmp(arguments[argCount - 1], "&", 1) == 0) {
        
        // If foreground mode is not on
        if (fgOnlyMode == 0) {
            argCount = argCount - 1;
            arguments[argCount] = NULL;
            background = 1;

        }

        // Foreground mode is on
        else {

            argCount = argCount - 1;
            arguments[argCount] = NULL;

        }
    } 

        // Check for built in command 'exit'
    if (strncmp(arguments[0], "exit", 4) == 0) {

        // This will break out of the while loop in main
        next = 0;
        return;

    }
        // Check if the input starts with #
    else if (strncmp(arguments[0], "#", 1) == 0) {

        clearArray(arguments, argCount);
        return;

    } 
        // Check for other built in commands, excluding exit
    else if ((strncmp(arguments[0], "cd", 2) == 0) || (strncmp(arguments[0], "status", 6) == 0)) {

        // Set flag to which built in, and send to builtInCommands() for processing
        int flag = 0;
        if (strncmp(arguments[0], "cd", 2) == 0) {

            flag = 1;

        }
        
        builtInCommands(arguments, flag, argCount);

    }
        // Else send to newProcess() function
    else {

        newProcess(arguments, argCount);

    }

    // Reset for next command
    background = 0;
    clearArray(arguments, argCount);
    return;

}

void checkBackground() {
    /***********************************************
    *   Params: NA
    *   Returns: NA
    *   Desc: Checks the background for child processes and 
    *         prints out how they were terminated.
    ***********************************************/

    for(int i = 0; i < bgCount; i++) {

        if(waitpid(bgProc[i], &childStatus, WNOHANG) > 0) {

            if (WIFSIGNALED(childStatus)) {
				printf("background child %d terminated: ", bgProc[i]);
                fflush(stdout);
				printf("terminated by signal %d\n", WTERMSIG(childStatus));
                fflush(stdout);

			}

			if (WIFEXITED(childStatus)) {
			
                printf("background child %d is done: ", bgProc[i]);
                fflush(stdout);
				printf("exit value %d\n", WEXITSTATUS(childStatus));
                fflush(stdout);

			}

        }
    }
}

void handle_SIGTSTP() {
    /***********************************************
    *   Params: NA
    *   Returns: NA
    *   Desc: Handler function for ctrl-Z or TSTP. Turns 
    *         on/off foreground only mode   
    ***********************************************/

    // Foreground mode is currently not turned on, turn it on
    if (fgOnlyMode == 0) {

        printf("\nEntering foreground-only mode (& is now ignored)\n");
        fflush(stdout);
        fgOnlyMode = 1;

    }

    // Foreground mode is currently turned on, turn it off
    else {

        printf("\nExiting foreground-only mode\n");
        fflush(stdout);
        fgOnlyMode = 0;

    }
}


int main(int argc, char *argv[]) {
    /***********************************************
    *   Params: NA
    *   Returns: NA
    *   Desc: Main function 
    ***********************************************/

    // Ignore Ctrl-C
	SIGINT_action.sa_handler = SIG_IGN;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = 0;
	sigaction(SIGINT, &SIGINT_action, NULL);

    // Catch Ctrl-Z and send to handler function
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = 0;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    // next == 1 keeps the everything running until exit command
    while (next == 1) {

        checkBackground();
        userInput();

    }

    return EXIT_SUCCESS;

}
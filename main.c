// Author: Ben Prince
// Assignment 3: Smallsh
// Date: 05/03/2021

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
int pid;
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

    for (int i = 0; i < argCount; i++) {
        arguments[i] = NULL;
    }

}

void exitStatus(int stat) {

    // If exited by status command
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

    int targetFD;
    char* sourceFD;
    int redirected = 0;

    for (int i = 1; i < argCount; i++) {
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

        else if (strcmp(arguments[i], ">") == 0) {
            redirected = 1;
            sourceFD = strdup(arguments[i+1]);

            targetFD = open(sourceFD, O_CREAT | O_RDWR | O_TRUNC, 0644);
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
    
    if (redirected != 0) {

        for(int i = 1; i < argCount; i++) {
			arguments[i] = NULL;
        }

    }

}


void newProcess(char** arguments, int argCount) {

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

        if (background == 0) {

            waitpid(spawnpid, &childStatus, 0);

        }

        else {

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

    char *arguments[MAX_ARGS];
    char input[MAX_CHARS];
    int argCount;

    // Get input from user and remove new line char
    printf(": ");
    fflush(stdout);
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;

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

    if (strcmp(input, "") == 0) {

        return;

    }

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

    for(int i = 0; i < bgCount; i++) {

        if(waitpid(bgProc[i], &childStatus, WNOHANG) > 0) {

            if (WIFSIGNALED(childStatus)) {
				printf("background child %d terminated: ", bgProc[i]);			//Child PID	
                fflush(stdout);
				printf("terminated by signal %d\n", WTERMSIG(childStatus));		//Signal number
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

    // Set pid to the pid of smallsh program
    pid = getpid();

    while (next == 1) {

        checkBackground();
        userInput();

    }

    return EXIT_SUCCESS;

}
// Author: Ben Prince
// Assignment 3: Smallsh
// Date: 05/03/2021

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> // pid_t
#include <unistd.h> // fork
#include <sys/wait.h> // waitpid

#define MAX_CHARS 2048
#define MAX_ARGS 512
int exitStat = 0;


int next = 1;

void clearArray(char** arguments, int argCount) {

    for (int i = 0; i < argCount; i++) {
        arguments[i] = NULL;
    }

}

void exitStatus(int stat) {

    // If exited by status command
    if (WIFEXITED(stat)) {
		printf("exit value: %d\n", WEXITSTATUS(stat));
	} 
    
    else {

        printf("terminated by signal %d\n", WTERMSIG(stat));
    }
}


void newProcess(char** arguments, int argCount) {

    int childStatus = -5;

    pid_t spawnpid = fork();

    if (spawnpid < 0) {

        printf("Fork Failed\n");       // TODO: Remove test printf
        fflush(stdout);
        exit(1);
    
    }

    else if (spawnpid == 0) {

        execvp(arguments[0], arguments);
        perror("Error");
        exit(1);

    }

    else {

        pid_t childPid;
        childPid = waitpid(spawnpid, &childStatus, 0);
        //exit(0);

    }

    clearArray(arguments, argCount);

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

        // strcpy(arguments[count], token);
        arguments[count] = token;
        count++;

    }

    return count;

}

void userInput() {

    char *arguments[MAX_ARGS];
    char input[MAX_CHARS];
    int argCount;

    // Get input from user
    printf(": ");
    fflush(stdout);
    fgets(input, sizeof(input), stdin);

    argCount = getArguments(arguments, input);

        // Check for built in command 'exit'
    if (strncmp(arguments[0], "exit", 4) == 0) {

        next = 0;
        return;

    }
        // Check if the input starts with # or is a blank line
    else if ((strncmp(arguments[0], "#", 1) == 0) || (strlen(arguments[0]) == 1)) {

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

    clearArray(arguments, argCount);

    return;

}


int main(int argc, char *argv[]) {

    while (next == 1) {

        userInput();

    }

    return EXIT_SUCCESS;

}
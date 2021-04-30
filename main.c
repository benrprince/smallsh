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
                printf("Cannot open file");
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

            targetFD = open(sourceFD, O_CREAT | O_RDWR | O_TRUNC, 0644); //Possible Change
            if (targetFD == -1) {
                printf("Cannot open file");
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

    int childStatus = -5;
    pid_t spawnpid = fork();

    if (spawnpid < 0) {

        printf("Fork Failed\n");
        fflush(stdout);
        exit(1);
    
    }

    else if (spawnpid == 0) {

        // Check for redirect and handle
        redirectCheck(arguments, argCount);

        execvp(arguments[0], arguments);
        printf("Error executing command\n");
        fflush(stdout);
        exit(0);

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

    // Ignore SIGINT
    struct sigaction SIGINT_action = {0};
	SIGINT_action.sa_handler = SIG_IGN;
	sigaction(SIGINT, &SIGINT_action, NULL);


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
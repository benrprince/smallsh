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

int next = 1;

void childCmd(char input[]) {

    // Initialize an array of all commands
    char arguments[MAX_ARGS][100];

    // Set up for strtok_r
    char *originalInput = strdup(input);
    char *savePTR = originalInput;
    char *token;
    int count = 0;

    // Parse input into separate arguments 
    while((token = strtok_r(savePTR, " ", &savePTR))) {

        strcpy(arguments[count], token);
        count++;

    }

    for (int i = 0; i< 10; i++) {

        printf("%s", arguments[i]);

    }

}

void newProcess(char input[]) {

    int childStatus = -5;
    pid_t childPid;

    pid_t spawnpid = fork();

    switch (spawnpid) {
        
        case -1:

            printf("Fork Failed\n");       // TODO: Remove test printf
            fflush(stdout);
            exit(1);
            break;
        
        case 0:

            //printf("I am the child\n");       // TODO: Remove test printf
            childCmd(input);
            break;

        default:

            childPid = waitpid(spawnpid, &childStatus, WNOHANG);
            printf("I am the parent\n");       // TODO: Remove test printf
            break;
    }
}


void builtInCommands(char input[], int flag) {

    // Try to consolidate the multiple instances of this
    char arguments[MAX_ARGS][100];

    // Set up for strtok_r
    char *originalInput = strdup(input);
    char *savePTR = originalInput;
    char *token;
    int count = 0;

    // Parse input into separate arguments 
    while((token = strtok_r(savePTR, " ", &savePTR))) {

        strcpy(arguments[count], token);
        count++;

    }

    switch (flag) {

        case 1:

            // cd by itself
            if (count == 1) {
                chdir(getenv("HOME"));
            }
            // cd with arguments
            else {
                int len = strlen(arguments[1]);
                char path[len];
                char nullChar = NULL;

                strcpy(path, arguments[1]);
                path[len-1] = nullChar;

                chdir(path);
            }

            break;

        default:

            printf("You typed status\n");       // TODO: Remove test printf
        
    }


}

void userInput() {


    char input[MAX_CHARS];

    // Get input from user
    printf(": ");
    fflush(stdout);
    fgets(input, sizeof(input), stdin);

        // Check for built in command 'exit'
    if (strncmp(input, "exit", 4) == 0) {

        next = 0;
        return;

    }
        // Check if the input starts with # or is a blank line
    else if ((strncmp(input, "#", 1) == 0) || (strlen(input) == 1)) {

        return;

    } 
        // Check for other built in commands, excluding exit
    else if ((strncmp(input, "cd", 2) == 0) || (strncmp(input, "status", 6) == 0)) {

        // Set flag to which built in, and send to builtInCommands() for processing
        int flag = 0;
        if (strncmp(input, "cd", 2) == 0) {

            flag = 1;

        }
        
        builtInCommands(input, flag);

    }
        // Else send to newProcess() function
    else {

        newProcess(input);

    }

    return;

}


int main(int argc, char *argv[]) {

    char input[MAX_CHARS];

    while (next == 1) {

        userInput();

    }

    return EXIT_SUCCESS;

}
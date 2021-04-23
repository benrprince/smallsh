// Author: Ben Prince
// Assignment 3: Smallsh
// Date: 05/03/2021

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHARS 2048
#define MAX_ARGS 512

int next = 1;

void userInput() {


    char input[MAX_CHARS];

    // Get input from user
    printf(": ");
    fflush(stdout);
    fgets(input, sizeof(input), stdin);

    // Check if input is exit
    if (strncmp(input, "exit", 4) == 0) {

        next = 0;
        return;

    } else {

    printf("%s", input);

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
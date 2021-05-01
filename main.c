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
int childStatus = -5;
int background = 0;
int bgProc[100];
int bgCount = 0;


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

        if (background == 0) {

            //pid_t childPid = waitpid(spawnpid, &childStatus, 0);     See if this works then remove one
            waitpid(spawnpid, &childStatus, 0);

        }

        else {

            bgProc[bgCount] = spawnpid;
            bgCount++;
            waitpid(spawnpid, &childStatus, WNOHANG);
            printf("Background pid: %d\n", spawnpid);
            fflush(stdout);
            
            // pid_t childPid = waitpid(spawnpid, &childStatus, WNOHANG);
            // bgProc[0] = spawnpid;
            // bgCount = bgCount + 1;

            // printf("%d\n", bgProc[bgCount]);

            // printf("Background pid: %d\n", spawnpid);
            // fflush(stdout);

        }
    }

    // while (waitpid(-1, &childStatus, WNOHANG) > 0) {

    //     spawnpid = waitpid(-1, &childStatus, WNOHANG);
    //     printf("%d\n", bgProc[bgCount]);

    //     printf("child %d terminated: ", bgProc[0]);
    //     exitStatus(childStatus);
    //     fflush(stdout);

    // }

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

    // // Ignore ^C
    // struct sigaction SIGINT_action = {0};
	// SIGINT_action.sa_handler = SIG_IGN;
	// sigaction(SIGINT, &SIGINT_action, NULL);

    char *arguments[MAX_ARGS];
    char input[MAX_CHARS];
    int argCount;

    // Get input from user and remove new line char
    printf(": ");
    fflush(stdout);
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;

    if (strcmp(input, "") == 0) {

        // Possibly put something here to look for be processes
        return;

    }

    argCount = getArguments(arguments, input);

    // Check for background process
    if (strncmp(arguments[argCount - 1], "&", 1) == 0) {

        argCount = argCount - 1;
        arguments[argCount] = NULL;
        background = 1;

    }

        // Check for built in command 'exit'
    if (strncmp(arguments[0], "exit", 4) == 0) {
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

// void catchSIGTSTP(int signo) {

// 	// If it's 1, set it to 0 and display a message reentrantly
// 	if (background == 1) {
// 		char* message = "Entering foreground-only mode (& is now ignored)\n";
// 		write(1, message, 49);
// 		fflush(stdout);
// 		background = 0;
// 	}

// 	// If it's 0, set it to 1 and display a message reentrantly
// 	else {
// 		char* message = "Exiting foreground-only mode\n";
// 		write (1, message, 29);
// 		fflush(stdout);
// 		background = 1;
// 	}
// }

void checkBackground() {

    for(int i = 0; i < bgCount; i++) {

        if(waitpid(bgProc[i], &childStatus, WNOHANG) > 0) {

            if (WIFSIGNALED(childStatus)) {
				printf("background child %d terminated: ", bgProc[i]);			//Child PID	
				printf("terminated by signal %d\n", WTERMSIG(childStatus));		//Signal number

			}

			if (WIFEXITED(childStatus)) {
			
                printf("background child %d is done: ", bgProc[i]);
				printf("exit value %d\n", WEXITSTATUS(childStatus));

			}

        }



    }


}


int main(int argc, char *argv[]) {

    // // Ignore ^C
	// struct sigaction sa_sigint = {0};
	// sa_sigint.sa_handler = SIG_IGN;
	// sigfillset(&sa_sigint.sa_mask);
	// sa_sigint.sa_flags = 0;
	// sigaction(SIGINT, &sa_sigint, NULL);

	// // Redirect ^Z to catchSIGTSTP()
	// struct sigaction sa_sigtstp = {0};
	// sa_sigtstp.sa_handler = catchSIGTSTP;
	// sigfillset(&sa_sigtstp.sa_mask);
	// sa_sigtstp.sa_flags = 0;
	// sigaction(SIGTSTP, &sa_sigtstp, NULL);

    while (next == 1) {

        checkBackground();
        userInput();

    }

    return EXIT_SUCCESS;

}
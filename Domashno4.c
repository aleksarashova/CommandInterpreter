#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <string.h>
#include <sys/wait.h>

#define BLOCK_SIZE 32

char* readline(int fd) {
    int bytesRead = 0;
    char buffer[BLOCK_SIZE];
    int lineLen = 0;

    char* nextLine = (char*) calloc(1, sizeof(char));

    if (NULL == nextLine) {
        close(fd);
        err(6, "ERROR: Allocating memory for the line was not successful.");
    }

    while (0 < (bytesRead = read(fd, buffer, BLOCK_SIZE))) {
            for (int i = 0; i < bytesRead; ++i) {
                nextLine = realloc(nextLine, lineLen + 1);
                if (NULL == nextLine) {
                    close(fd);
                    err(7, "ERROR: Reallocating memory for the line was not successful.");
                }

                if ('\n' == buffer[i]) {
                    nextLine[lineLen] = '\0';

                    if (i < (bytesRead - 1)) {
                        off_t offset = i - bytesRead + 1;
                        int status = lseek(fd, offset, SEEK_CUR);
                        if(-1 == status){
                            free(nextLine);
                            close(fd);
                            err(8, "ERROR: Setting the cursor at the right position was not successful.");

                        }
                    }

                    return nextLine;
                }

                nextLine[lineLen] = buffer[i];
                lineLen++;
            }
    }

    if (-1 == bytesRead) {
        free(nextLine);
        close(fd);
        err(9, "ERROR: Reading from file was not successful.");
    }


    if (lineLen > 0) {
        nextLine[lineLen] = '\0';
        return nextLine;
    } else {
        free(nextLine);
        return NULL;
    }
}

char** tokenize(char* line) {
    char* lineCopy = strdup(line);
    if(NULL == lineCopy) {
        return NULL;
    }
    char** tokens = NULL;
    char* nextToken = strtok(lineCopy, " ");

    int index = 0;
    int size = 0;

    while(NULL != nextToken) {
        size++;
        tokens = realloc(tokens, size * sizeof(char*));

        if(NULL == tokens) {
            free(lineCopy);
            for(int i = 0; i < index; ++i) {
                free(tokens[i]);
            }
            return NULL;
        }

        tokens[index] = strdup(nextToken);

        if(NULL == tokens[index]) {
            free(lineCopy);
            for(int i = 0; i < index; ++i) {
                free(tokens[i]);
            }
            free(tokens);
            return NULL;
        }
        index++;

        nextToken = strtok(NULL, " ");
    }

    size++;
    tokens = realloc(tokens, size * sizeof(char*));

    if(NULL == tokens) {
        free(lineCopy);
        for(int i = 0; i < index; ++i) {
            free(tokens[i]);
        }
        return NULL;
    }

    tokens[index] = NULL;

    free(lineCopy);

    return tokens;
}

int main(int argc, char* argv[]) {
    int fd = open(argv[1], O_RDONLY);

    if(-1 == fd) {
        err(1, "ERROR: Opening the file was not successful.");
    }

    char* line = NULL;

    while(NULL != (line = readline(fd))) {

        char** tokens = tokenize(line);
        if(NULL == tokens) {
            free(line);
            close(fd);
            err(2, "ERROR: Tokenizing was not successful.");
        }

        int childPid = fork();

        if(-1 == childPid) {
            free(line);
            for(int i = 0; NULL != tokens[i]; ++i) {
                free(tokens[i]);
            }
            free(tokens);
            close(fd);
            err(3, "ERROR: Forking was not successful.");
        }

        if(0 == childPid) {
            if(-1 == execvp(tokens[0], tokens)) {
                free(line);
                for(int i = 0; NULL != tokens[i]; ++i) {
                    free(tokens[i]);
                }
                free(tokens);
                close(fd);
                err(4, "ERROR: Execvp was not successful.");
            }
            exit(0);
        }

        int status;

        if(-1 == wait(&status)) {
            free(line);
            for(int i = 0; NULL != tokens[i]; ++i) {
                free(tokens[i]);
            }
            free(tokens);
            close(fd);
            err(5, "ERROR: Waiting was not successful.");
        }

        if(WIFEXITED(status)) {
            int exitStatus = WEXITSTATUS(status);
            printf("%s exited with exit status: %d\n", tokens[0], exitStatus);
        } else {
            printf("%s was terminated by another process.\n", tokens[0]);
        }

        free(line);
        for(int i = 0; NULL != tokens[i]; ++i) {
            free(tokens[i]);
        }
        free(tokens);
    }

    close(fd);
    exit(0);
}


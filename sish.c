#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

char *path[1000];
int pathlen = 2;
char error_message[30] = "An error has occurred\n";
int has_err = 0;

void error()
{
    ssize_t bytes_written =
        write(STDERR_FILENO, error_message, strlen(error_message));
    if (bytes_written < 0)
    {
        exit(1);
    }
    has_err = 1;
}

void stripws(char *str)
{
    char *start = str;
    while (*start && isspace(*start))
    {
        start++;
    }
    char *end = str + strlen(str) - 1;
    while (*end && isspace(*end))
    {
        end--;
    }
    *(end + 1) = '\0';
    if (start != str)
    {
        memmove(str, start, end - start + 2);
    }
}

char *concat(char *str1, char *str2)
{
    int i = strlen(str1) + strlen(str2);
    char *result = (char *)malloc(i * sizeof(char));
    strcpy(result, str1);
    strcat(result, str2);
    return result;
}

void run(char *cmd, char *args[], int i)
{
    int has_redir = 0;
    int redir_pos = -1;
    for (int j = 0; j < i; j++)
    {
        if (strcmp(args[j], ">") == 0)
        {
            if (has_redir == 1)
            {
                error();
                return;
            }
            has_redir = 1;
            redir_pos = j;
        }
    }
    if (has_redir)
    {
        if ((i > 2) && (i - 2 == redir_pos) && (args[i - 1] != NULL) &&
            (strcmp(args[i - 1], ">") != 0))
        {
            char *output = args[i - 1];
            int fdout = open(output, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (fdout == -1)
            {
                error();
                return;
            }

            int stdout_copy = dup(STDOUT_FILENO);
            if (stdout_copy == -1)
            {
                error();
                return;
            }
            if (dup2(fdout, STDOUT_FILENO) == -1)
            {
                error();
                return;
            }
            args[i - 1] = NULL;
            args[i - 2] = NULL;

            // int stderr_copy = dup(STDERR_FILENO);
        }
        else
        {
            error();
            return;
        }
    }
    // check if path is valid
    for (int i = 0; i < pathlen; i++)
    {
        char *p = concat(concat(path[i], "/"), args[0]);
        if (access(p, X_OK) != -1)
        {
            int x = execvp(p, args);
            if (x == -1)
            {
                // execvp error
                error();
                return;
            }
        }
    }
    // throw error invalid path
    error();
    return;
}

int main(int argc, char *argv[])
{
    size_t len = 0;
    ssize_t nread;
    FILE *fp = stdin;
    path[0] = "/usr/bin";
    path[1] = "/bin";
    if (argc > 2)
    {
        printf("too many args\n");
        exit(1);
    }
    if (argc == 2)
    {
        // batch file
        fp = fopen(argv[1], "r");
        if (fp == NULL)
        {
            // file not found
            printf("Error opening file.\n");
            exit(1);
        }
    }
    printf("Singapore Shell - Made by Ye Xinkang\n");
    printf("This is a simple UNIX shell\n");
    printf("Builtin commands:\n");
    printf("cd [PATH] - change directory\n");
    printf("path [PATH1] [PATH2] - Set the path variable\n");
    printf("exit - exit shell\n");
    printf("command [ARG1] [ARG2] ... - Use command\n");
    while (1)
    {
        char *line = NULL;
        if (argc == 1)
        {
            printf("sish> ");
        }
        nread = getline(&line, &len, fp);
        (void)nread;
        if (feof(fp) || strlen(line) == 1)
        {
            exit(0);
        }
        char *cmd = strtok(line, "&");
        while (cmd != NULL)
        {
            has_err = 0;
            stripws(cmd);
            char *token;
            char *rest = cmd;
            char *args[1000];
            int i = 0;
            while ((token = strsep(&rest, " ")) != NULL)
            {
                stripws(token);
                if (strlen(token) > 0)
                {
                    args[i] = token;
                    i++;
                }
            }
            args[i] = NULL;
            // process for >
            if (!has_err)
            {
                if (strcmp(args[0], "exit") == 0)
                {
                    // exit command
                    if (i != 1)
                    {
                        error();
                    }
                    else
                    {
                        wait(NULL);
                        exit(0);
                    }
                }
                else if (strcmp(args[0], "cd") == 0)
                {
                    // chdir
                    char *path = args[1];
                    if (chdir(path) != 0)
                    {
                        error();
                    }
                }
                else if (strcmp(args[0], "path") == 0)
                {
                    // set path to args[1:i]
                    for (int x = 0; x < pathlen; x++)
                    {
                        path[x] = NULL;
                    }
                    pathlen = 0;
                    for (int j = 1; j < i; j++)
                    {
                        path[j - 1] = concat("/", args[j]);
                        // printf("%s\n", concat("/", args[j]));
                        pathlen++;
                    }
                }
                else
                {
                    // run command in new process
                    int rc = fork();
                    if (rc < 0)
                    {
                        error();
                        exit(1);
                    }
                    else if (rc == 0)
                    {
                        // child
                        run(cmd, args, i);
                    }
                    else
                    {
                        // parent
                        wait(NULL);
                    }
                }
            }
            cmd = strtok(NULL, "&");
        }
    }
}

// gcc -o sish sish.c -Wall -Werror
// ./sish
// ls -m  -a   & whoami   &   pwd -P    & sleep 3 &  path  /usr/bin  /bin
// /home/runner  /home & exit     & cd /usr/bin & exit
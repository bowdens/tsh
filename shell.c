#include "libtalaris.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>

// envp global to be set on main (and then not touched!)
char **genvp;

/*
 * inbuilt function definitions
 * all have the prototype int argc, char **argv, LT_Parser *parser
 */
int cd(int, char **, LT_Parser *);
int dirs(int, char **, LT_Parser *);
int exec(int, char **, LT_Parser *);

/*
 * exec binary replaces the current thread with the binary
 * char *path, char **argv, char **envp
 * (1)  full path to binary
 * (2)  list of arguments to be passed. last must be NULL
 * (3)  list of environment variables. last must be NULL
 */
void exec_binary(char *, char **, char **);
int exec_boilerplate(int, char **);
int unfound(int, char**, LT_Parser *);

int main(int argc, char **argv, char **envp) {
    genvp = envp;
    LT_Parser *parser = lt_create_parser();
    /*
     * Commands declarations in the form
     * {name, help description, usage, flags, function, NULL}
     * each entry must end with a NULL and the final entry must be NULL
     */
    LT_Command commands[] = {
        {"cd", "Change the shell working directory", "Usage: cd [dir]", LT_UNIV, cd, NULL},
        {"dirs", "Display directory stack", "Usage: dirs", LT_UNIV, dirs, NULL},
        {"exec", "Replace the shell with a given command", "Usage: exec binary [arguments]", LT_UNIV, exec, NULL},
        {NULL}
    };
    lt_add_commands(parser, commands);
    parser->unfound = unfound;
    parser->prompt = "tsh $ ";

    char *matches[] = {"cd", "dirs", "exec", NULL};

    int res = 0;
    do {
        res = lt_input(parser, matches);
    } while (res != LT_CALL_FAILED);
}

int cd(int argc, char **argv, LT_Parser *parser) {
    if (argc > 2) {
        printf("cd: too many arguments\n");
        return 1;
    }
    char *path = NULL;
    if (argc == 1) {
        path = getenv("HOME");
    } else {
        path = argv[1];
    }
    if(chdir(path) == -1) {
        perror("cd");
        return 1;
    }
    return 0;
}

int dirs(int argc, char **argv, LT_Parser *parser) {
    char buf[PATH_MAX];
    char *res = getcwd(buf, PATH_MAX);
    if (res == NULL) {
        perror("dirs");
        return 1;
    }
    printf("%s\n", buf);
    return 0;
}

int exec(int argc, char **argv, LT_Parser *parser) {
    return exec_boilerplate(argc-1, &argv[1]);
}

int exec_boilerplate(int argc, char **argv) {
    char *path = argv[0];
    char *fullpath = NULL;
    if (path[0] == '.' || path[0] == '/' || path[0] == '~') {
        // we've got a full path already
        fullpath = path;
    } else {
        char *envpath = getenv("PATH");
        char *pdir = NULL;
        // iterate through : separated elements in the path
        char testpath[PATH_MAX];
        while ((pdir = strtok((pdir == NULL ? envpath : NULL), ":")) != NULL) {
            strncpy(testpath, pdir, PATH_MAX);
            strncat(testpath, "/", PATH_MAX);
            strncat(testpath, path, PATH_MAX);
            if (access(testpath, X_OK) != -1) {
                fullpath = testpath;
                break;
            }
        }

    }
    if (fullpath == NULL) {
        printf("couldn't find any binary with name '%s' in your PATH\n", path);
        return 1;
    } else {
        char **eargv = malloc(sizeof(char*) * (argc + 1));
        for (int i = 0; i < argc; i++)
            eargv[i] = argv[i];
        eargv[argc] = NULL;
        exec_binary(fullpath, argv, genvp);
        printf("PANIC! we should never get here...\n");
        exit(2);
    }
}

void exec_binary(char *path, char **argv, char **envp) {
    int res = execve(path, argv, envp);
    if (res == -1) {
        perror("exec");
    }
    exit(errno);
}

int unfound(int argc, char **argv, LT_Parser *parser) {
    if (fork() == 0) {
        if (exec_boilerplate(argc, argv) != 0) {
            exit(1);
        }
    } else {
        wait(NULL);
    }
}

#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <locale.h>

#ifdef _WIN32
#include <Windows.h>
#include <direct.h>
#include "include/linuxsub/dirent.h"
#include "include/linuxsub/wait.h"
#define GetCurrentDir _getcwd
#elif defined __unix__
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#define GetCurrentDir getcwd
#endif

#include "include/VaderAPI.h"
#include "include/appfuncs.h"

using namespace VADER;

std::string API::MAJOR = "1";
std::string API::MINOR = "0";
std::string API::PATCH = "0";
std::string API::VERSION = "v" + API::MAJOR + "." + API::MINOR + "." + API::PATCH;

const std::string API::builtin_list[7] = {
    "cd",
    "ls",
    "echo",
    "cat",
    "help",
    "ruler",
    "clear"
};

const std::string API::builtin_man[7] = {
    "<path/directory...>",
    "[path/directory...]",
    "<string>",
    "<file>",
    "[command]",
    "",
    "",
};

const std::string API::builtin_desc[7] = {
    "Changes directory",
    "List files and subdirectories",
    "Prints input string",
    "Prints input file",
    "Prints this command",
    "Shows a screen ruler, used for resizing to 80x25",
    "Clears the screen"
};

int(* API::builtin_funcs[7])(std::vector<std::string>, std::string& cwd) = {
    &API::cd,
    &API::ls,
    &API::echo,
    &API::cat,
    &API::help,
    &API::ruler,
    &API::_clear,
};

API::API() {}

API::~API() {}

#ifdef _WIN32
LPWSTR ConvertString(const std::string& instr) {
    // Assumes std::string is encoded in the current Windows ANSI codepage
    int bufferlen = ::MultiByteToWideChar(CP_ACP, 0, instr.c_str(), instr.size(), NULL, 0);

    if (bufferlen == 0)
    {
        // Something went wrong. Perhaps, check GetLastError() and log.
        return 0;
    }

    // Allocate new LPWSTR - must deallocate it later
    LPWSTR widestr = new WCHAR[bufferlen + 1];

    ::MultiByteToWideChar(CP_ACP, 0, instr.c_str(), instr.size(), widestr, bufferlen);

    // Ensure wide string is null terminated
    widestr[bufferlen] = 0;

    // Do something with widestr
    return widestr;
    //delete[] widestr;
}
#endif

void API::cwd(std::string& cwd) {
    char buff[FILENAME_MAX]; //create string buffer to hold path
    GetCurrentDir(buff, FILENAME_MAX);
    cwd = std::string(buff);
}

int API::launch(std::vector<std::string> args, std::string& cwd) {
#ifdef __unix__ or __apple__
    char** pargs;
    for (size_t i = 0; i < args.size(); i++) {
        pargs[i] = (char*)args[i].c_str();
    }

    pid_t pid, wpid;
    int status;
    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(args[0].c_str(), pargs) == -1) {
            perror("lsh");
        }
        return EXIT_FAILURE;
}
    else if (pid < 0) {
        // Error forking
        perror("lsh");
    }
    else {
        // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return EXIT_SUCCESS;
#elif defined _WIN32
    std::string command;
    for (size_t i = 0; i < args.size(); i++) {
        command += args[i];
        if (i != args.size() - 1) command += ' ';
    }

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    bool b = false;

    // Start the child process. 
    if (!CreateProcessW(NULL,   // No module name (use command line)
        ConvertString(command),        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi)           // Pointer to PROCESS_INFORMATION structure
    ) {
#ifdef _DEBUG
        print("[Vader DEBUG] Using system(command.c_str())...", true);
#endif
        if (system(command.c_str()) != 0) {
            cprint(args[0] + " does not exist in " + cwd, red, true);
        } else b = true;
    } else {
        b = true;
        // Wait until child process exits.
        WaitForSingleObject(pi.hProcess, INFINITE);

        // Close process and thread handles. 
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    return (int)!b;
#endif
}

int API::cd(std::vector<std::string> args, std::string &cwd) {
    for (int i = 1; i < args.size(); i++) {
        if (strlen(args[1].c_str()) == 1) { args[i] += ":/"; API::cd(args, cwd); }
        else if (chdir(args[i].c_str()) != 0) {
            error(args[i] + " does not exist in " + cwd);
        }
        API::cwd(cwd);
    }
    return EXIT_SUCCESS;
}

int API::echo(std::vector<std::string> args, std::string& cwd) {
    for (int i = 1; i < args.size(); i++) {
        print(args[i] + " ");
    }
    print();
    return EXIT_SUCCESS;
}

static void list_directory(const char* dirname) {
    /* Open directory stream */
    DIR* dir = opendir(dirname);
    if (!dir) {
        /* Could not open directory */
        fprintf(stderr, "Cannot open %s (%s)\n", dirname, strerror(errno));
    }

    struct dirent* ent;
    int max_size = 0;
    while ((ent = readdir(dir)) != NULL) {
        int namelen;
#ifdef _WIN32
        namelen = ent->d_namlen;
#else
        while (namelen < PATH_MAX && ent->d_name[namelen] != 0) {
            namelen++;
        }
#endif
        if (namelen + 1 > max_size) max_size = namelen + 1;
    }

    closedir(dir);
    dir = opendir(dirname);

    /* Print all files and directories within the directory */
    int _ = 0;
    while ((ent = readdir(dir)) != NULL) {
        int color;
        switch (ent->d_type) {
        case DT_REG:
            color = blue;
            break;

        case DT_DIR:
            color = green;
            break;

        case DT_LNK:
            color = bright_blue;
            break;

        default:
            color = blue;
        }
        cprint(charfix(ent->d_name, max_size), color);
        if ((_ + max_size) >= 80) {
            print();
            _ = 0;
        } else _ += max_size;
    }
    print();

    closedir(dir);
}

int API::ls(std::vector<std::string> args, std::string& cwd) {
    /* For each directory in command line */
    int i = 1;
    while (i < args.size()) {

        list_directory(args[i].c_str());
        i++;
    }

    /* List current working directory if no arguments on command line */
    if (args.size() == 1)
        list_directory(".");
    return EXIT_SUCCESS;
}

static void output_file(const char* fn) {
    /* Open file */
    FILE* fp = fopen(fn, "r");
    if (!fp) {
        /* Could not open directory */
        fprintf(stderr, "Cannot open %s (%s)\n", fn, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Output file to screen */
    size_t n;
    do {
        /* Read some bytes from file */
        char buffer[4096];
        n = fread(buffer, 1, 4096, fp);

        /* Output bytes to screen */
        fwrite(buffer, 1, n, stdout);
    } while (n != 0);
    print();

    /* Close file */
    fclose(fp);
}

int API::cat(std::vector<std::string> args, std::string& cwd) {
    /* Require at least one file */
    if (args.size() == 1) {
        error("Usage: cat filename\n");
        return EXIT_FAILURE;
    }

    /* For each file name argument in command line */
    int i = 1;
    while (i < args.size()) {
        output_file(args[i].c_str());
        i++;
    }
    return EXIT_SUCCESS;
}

int API::help(std::vector<std::string> args, std::string& cwd) {
    if (args.size() > 1) {
        if (args.size() == 2) {
            for (int i = 0; i < sizeof(API::builtin_list) / sizeof(API::builtin_list[0]); i++) {
                if (args[1] == API::builtin_list[i]) {
                    print(args[1] + " " + API::builtin_man[i], true);
                    return EXIT_SUCCESS;
                };
            }
        } else {
            error("Usage: " + API::builtin_man[API::HELP]);
        }
        return EXIT_FAILURE;
    }
    // assuming help all functions
    cprint("Use `help [command]` too see a command\'s usage", bright_red, true);

    for (int i = 0; i < sizeof(API::builtin_list) / sizeof(API::builtin_list[0]); i++) {
        print(charfix(API::builtin_list[i], 8) + " " + API::builtin_desc[i], true);
    }
    return EXIT_SUCCESS;
}

int API::ruler(std::vector<std::string> args, std::string& cwd) {
    int i, j, w = 80, h = 25;
    for (i = 1; i <= h - 1; i++) {
        for (j = 1; j <= w; j++) {
            ANSI::foreground(white);
            if (j % 10 == 0 && i == 1) {
                ANSI::foreground(red);
                printf("%1d", (j - (j % 10)) / 10);
            }
            else if (i % 10 == 0 && j == 1) {
                ANSI::foreground(red);
                printf("%1d", (i - (i % 10)) / 10);
            }
            else if (i == 2) {
                if (j % 10 == 0) ANSI::foreground(red);
                printf("%1d", j % 10);
            }
            else if (j == 2) {
                if (i % 10 == 0) ANSI::foreground(red);
                printf("%1d", i % 10);
            }
            else if (j % 10 == 0 && i % 10 == 0) printf("+");
            else if (j % 10 == 0) printf("|");
            else if (i % 10 == 0) printf("-");
            else printf(" ");
        }
        printf("\n");
    }
    ANSI::foreground(red);
    printf("25 ");
    ANSI::reset();
    return EXIT_SUCCESS;
}

int API::_clear(std::vector<std::string> args, std::string& cwd) {
    return API::clear();
}

int API::clear() {
#ifdef _WIN32
    system("cls");
#elif defined __unix__ or __apple__
    system("clear");
#endif
    cprint("Vader " + VADER::API::VERSION, 5, true);
    cprint("Copyright Logan Houston 2022. All rights reserved.", 5, true);
    return EXIT_SUCCESS;
}
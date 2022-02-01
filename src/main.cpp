#include "include/VaderAPI.h"
#include "include/appfuncs.h"

#include <stdlib.h>

int main() {
    bool running = true;
    std::string input, cwd;

    VADER::API api;

    printf("\033]0;Vader\007");
    system("chcp 65001"); // shh

    api.clear();
    api.welcome();

    while (running) {
        api.cwd(cwd);
        cprint("  "+cwd, white, dark_gray);
        printcaret(dark_gray, black, true);

        std::getline(std::cin, input);

        if (input == "") continue;

        std::vector<std::string> args = parse(input);

        bool used_builtin = false;

        for (size_t i = 0; i < sizeof(api.builtin_list) / sizeof(std::string); i++) {
            if (args[0] == api.builtin_list[i]) {
                used_builtin = true;
                (*api.builtin_funcs[i])(args, cwd);
            }
        }

        if (!used_builtin) {
            if (args[0] == "exit") running = false;
            else api.launch(args, cwd);
        }
    }

    return 0;
}
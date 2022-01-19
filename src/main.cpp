#include "include/VaderAPI.h"
#include "include/appfuncs.h"

int main() {
    bool running = true;
    std::string input, cwd;

    VADER::API api;

    std::cout << "\033]0;Vader\007";

    api.clear();

    while (running) {
        api.cwd(cwd);
        cprint(cwd, white);
        cprint("$", bright_blue);
        std::getline(std::cin, input);

        if (input == "") continue;

        std::vector<std::string> args = parse(input);

        bool used_builtin = false;

        for (size_t i = 0; i < sizeof(api.builtin_list)/sizeof(std::string); i++) {
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
#include "include/VaderAPI.h"
#include "include/appfuncs.h"

#include <stdlib.h>
#include <time.h>
#include <ctime>
#include <sstream>

using namespace VADER;

int main(int argc, char** argv) {
    API* api = new API();
    bool running = true;
    std::string input, cwd = API::cwd();

    printf("\033]0;Vader\007"); // set title

#ifdef _WIN32
    api->launch({ "chcp", "65001" }); // set utf8
#endif
    api->clear();

    const int color_palettes[8][4] = { // light, dark, color on light, color on dark
        { dark_gray, black, white, white },
        { bright_red, red, white, white },
        { bright_green, green, white, white },
        { bright_yellow, yellow, black, white },
        { bright_blue, blue, white, white },
        { bright_magenta, purple, white, white },
        { bright_cyan, cyan, white, white },
        { gray, dark_gray, black, white }
    };
    int colors[4] = { -1,-1,-1,-1 };

    int c = -1;
    if (argc > 2) {
        error("Usage: vader <color (0-7)>");
    } else if (argc != 1) {
        try {
            c = std::stoi(argv[1]);
            if (c > -1 && c < 8) {
                colors[0] = color_palettes[c][0];
                colors[1] = color_palettes[c][1];
                colors[2] = color_palettes[c][2];
                colors[3] = color_palettes[c][3];
            } else error("Usage: color <int (0-7)>");
        } catch (const std::exception& e) {
            error("Usage: color <int (0-7)>");
        }
    }

    if (colors[0] == -1) {
        colors[0] = color_palettes[7][0];
        colors[1] = color_palettes[7][1];
        colors[2] = color_palettes[7][2];
        colors[3] = color_palettes[7][3];
    }

    api->welcome();

    time_t t; tm* tm; char tb[16]; // variables used in time defined outside of loop because unnecessary to redefine every frame

    while (running) {
        cwd = API::cwd();
        std::time(&t);
        tm = std::localtime(&t);
        strftime(tb, sizeof(tb), "%H:%M:%S", tm);

        cprint(" " + api->icon, black, white);
        printcaret(white, colors[0]);
        cprint(icons::CLOCK + ' ' + std::string(tb), colors[2], colors[0]);
        printcaret(colors[0], colors[1]);
        cprint(icons::FOLDER + ' ' + cwd, colors[3], colors[1]);
        printcaret(colors[1], reset, true);

        std::getline(std::cin, input);

        if (input == "") continue;

        std::vector<std::string> args = parse(input);

        bool used_builtin = false;

        for (size_t i = 0; i < sizeof(api->builtin_list) / sizeof(std::string); i++) {
            if (args[0] == api->builtin_list[i]) {
                used_builtin = true;
                (*api->builtin_funcs[i])(args);
            }
        }

        if (!used_builtin) {
            if (args[0] == "exit") running = false;
            else if (args[0] == "color") {
                if (args.size() != 2) {
                    error("Usage: color <int (0-7)>");
                } else {
                    try {
                        int c = std::stoi(args[1]);
                        if (c > -1 && c < 8) {
                            colors[0] = color_palettes[c][0];
                            colors[1] = color_palettes[c][1];
                            colors[2] = color_palettes[c][2];
                            colors[3] = color_palettes[c][3];
                        } else {
                            error("Usage: color <int (0-7)>");
                        }
                    } catch (const std::exception& e) {
                        error("Usage: color <int (0-7)>");
                    }
                }
            } else api->launch(args);
        }
    }

    return 0;
}
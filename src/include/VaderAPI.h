#ifndef PYRO_API
#define PYRO_API

#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <cstdlib>
#include <array>
#include "colors.h"

namespace VADER {
	namespace API {
		enum Commands { CD, LS, ECHO, CAT, HELP, RULER, CLEAR, WELCOME, INFO };
		extern std::string MAJOR, MINOR, PATCH, VERSION;
		extern int cd(std::vector<std::string> args);
		extern int echo(std::vector<std::string> args);
		extern int ls(std::vector<std::string> args);
		extern int cat(std::vector<std::string> args);
		extern int help(std::vector<std::string> args);
		extern int ruler(std::vector<std::string> args);
		extern int _clear(std::vector<std::string> args);
		extern int _welcome(std::vector<std::string> args);
		extern int info(std::vector<std::string> args);
		extern int version(std::vector<std::string> args);

		extern int clear();
		extern int welcome(std::array<int, 4> colors);

		extern int launch(std::vector<std::string> args);
		extern std::string cwd();

		constexpr int builtins = 10;
		extern const std::array<std::string, builtins> builtin_list;
		extern const std::array<std::string, builtins> builtin_desc;
		extern const std::array<std::string, builtins> builtin_man;
		extern int(*builtin_funcs[builtins])(std::vector<std::string>);
		extern std::string icon;
		extern std::array<int, 4> colors;
	};
}

#endif
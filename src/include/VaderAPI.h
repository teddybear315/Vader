#ifndef PYRO_API
#define PYRO_API

#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <cstdlib>
#include "colors.h"

namespace VADER {
	class API {
	public:
		enum Commands { CD, LS, ECHO, CAT, HELP, RULER };
		static std::string MAJOR, MINOR, PATCH, VERSION;
		static int cd(std::vector<std::string> args, std::string& cwd);
		static int echo(std::vector<std::string> args, std::string& cwd);
		static int ls(std::vector<std::string> args, std::string& cwd);
		static int cat(std::vector<std::string> args, std::string& cwd);
		static int help(std::vector<std::string> args, std::string& cwd);
		static int ruler(std::vector<std::string> args, std::string& cwd);
		static int _clear(std::vector<std::string> args, std::string& cwd);
		static int clear();

		static int launch(std::vector<std::string> args, std::string& cwd);
		static void cwd(std::string& cwd);

		API();
		~API();

		static const std::string builtin_list[7];
		static const std::string builtin_desc[7];
		static const std::string builtin_man[7];
		static int(*builtin_funcs[7])(std::vector<std::string>, std::string& cwd);
	};
}

#endif
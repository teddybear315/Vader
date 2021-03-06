#include <sstream>
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
#include <sys/sysinfo.h>
#define GetCurrentDir getcwd
#endif

#include "include/VaderAPI.h"
#include "include/appfuncs.h"

using namespace VADER;

std::string API::MAJOR = "0";
std::string API::MINOR = "1";
std::string API::PATCH = "0";
std::string API::VERSION = "v" + API::MAJOR + "." + API::MINOR + "." + API::PATCH;

std::array<int, 4> API::colors = { -1,-1,-1,-1 };
std::string API::icon = "☺";

const std::array<std::string, API::builtins> API::builtin_list = {
	"cd",
	"ls",
	"echo",
	"cat",
	"help",
	"ruler",
	"clear",
	"welcome",
	"info",
	"version" };

const std::array<std::string, API::builtins> API::builtin_man = {
	"<path/directory...>",
	"[path/directory...]",
	"<string>",
	"<file>",
	"[command]",
	"",
	"",
	"",
	"<type>",
	"" };

const std::array<std::string, API::builtins> API::builtin_desc = {
	"Changes directory",
	"List files and subdirectories",
	"Prints input string",
	"Prints input file",
	"Prints this command",
	"Shows a screen ruler, used for resizing to 80x24",
	"Clears the screen",
	"Shows the welcome message",
	"Gives information about the system",
	"Shows Vader\'s current version" };

int (*API::builtin_funcs[API::builtins])(std::vector<std::string>) = {
	&API::cd,
	&API::ls,
	&API::echo,
	&API::cat,
	&API::help,
	&API::ruler,
	&API::_clear,
	&API::_welcome,
	&API::info,
	&API::version,
};

std::string API::cwd() {
	char buff[FILENAME_MAX]; //create string buffer to hold path
	GetCurrentDir(buff, FILENAME_MAX);
	return std::string(buff);
}

int API::launch(std::vector<std::string> args) {
#ifdef __unix__
	char* pargs[args.size() + 1];
	if (args.size() > 1) {
		size_t i = 0;
		for (; i < args.size(); i++) {
			pargs[i] = (char*)args[i].c_str();
		}
		pargs[++i] = NULL;
	} else {
		pargs[0] = (char*)args[0].c_str();
		pargs[1] = NULL;
	}
	pid_t pid;
	int status;
	pid = fork();
	if (pid == 0) {
		// Child process
		if (execvp(args[0].c_str(), pargs) == -1) {
			perror("Vader");
		}
		return EXIT_FAILURE;
	} else if (pid < 0) {
		// Error forking
		perror("Vader");
	} else {
		// Parent process
		do {
			waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}

	return EXIT_SUCCESS;
#elif defined _WIN32
	std::string command;
	for (size_t i = 0; i < args.size(); i++) {
		command += args[i];
		if (i != args.size() - 1)
			command += ' ';
	}

	STARTUPINFOW si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	bool b = false;

	// Start the child process.
	if (!CreateProcessW(NULL,	// No module name (use command line)
		ConvertString(command), // Command line
		NULL,					// Process handle not inheritable
		NULL,					// Thread handle not inheritable
		FALSE,					// Set handle inheritance to FALSE
		0,						// No creation flags
		NULL,					// Use parent's environment block
		NULL,					// Use parent's starting directory
		&si,					// Pointer to STARTUPINFO structure
		&pi						// Pointer to PROCESS_INFORMATION structure
	))
	{
#ifdef _DEBUG
		print("[Vader DEBUG] Using system(command.c_str())...", true);
#endif
		if (system(command.c_str()) == 0)	b = true;
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

int API::cd(std::vector<std::string> args) {
	for (size_t i = 1; i < args.size(); i++) {
		if (strlen(args[1].c_str()) == 1) {
			args[i] += ":/";
			API::cd(args);
		} else if (chdir(args[i].c_str()) != 0) {
			error(args[i] + " does not exist in " + API::cwd());
		}
	}
	return EXIT_SUCCESS;
}

int API::echo(std::vector<std::string> args) {
	for (size_t i = 1; i < args.size(); i++) {
		print(args[i] + " ");
	}
	print();
	return EXIT_SUCCESS;
}

static void list_directory(std::string dirname, bool showHidden) {
	/* Open directory stream */
	DIR* dir;
	try {
		dir = opendir(dirname.c_str());
	} catch (const std::exception& e) {
		error("Cannot open " + dirname + " (" + e.what() + ")");
	}

	struct dirent* ent;
	int max_size = 0;
	while ((ent = readdir(dir)) != NULL) {
		int namelen = 0;
#ifdef _WIN32
		namelen = ent->d_namlen;
#else
		while (namelen < PATH_MAX && ent->d_name[namelen] != '\0') {
			namelen++;
		}
#endif
		if (namelen + 1 > max_size && ((ent->d_name[0] == '.' && showHidden) || (ent->d_name[0] != '.'))) max_size = namelen + 1;
	}

	closedir(dir);
	dir = opendir(dirname.c_str());

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
		if ((ent->d_name[0] == '.' && showHidden) || (ent->d_name[0] != '.')) {
			cprint(charfix(ent->d_name, max_size), color);
			if ((_ + max_size) >= 80) {
				print();
				_ = 0;
			} else _ += max_size;
		}
	}
	print();

	closedir(dir);
}

int API::ls(std::vector<std::string> args) {
	bool showHidden = false;
	for (size_t i = 0; i < args.size(); i++) {
		if (args[i] == "-h" || args[i] == "--hidden") {
			showHidden = true;
			args.erase(args.begin() + i);
		}
	}

	/* For each directory in command line */
	size_t i = 1;
	while (i < args.size()) {
		list_directory(args[i].c_str(), showHidden);
		i++;
	}

	/* List current working directory if no arguments on command line */
	if (args.size() == 1)
		list_directory(".", showHidden);
	return EXIT_SUCCESS;
}

static void output_file(const char* fn) {
	/* Open file */
	FILE* fp = fopen(fn, "r");
	if (!fp) {
		/* Could not open directory */
		error("Cannot open " + std::string(fn));
		return;
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

int API::cat(std::vector<std::string> args) {
	/* Require at least one file */
	if (args.size() == 1) {
		error("Usage: cat filename\n");
		return EXIT_FAILURE;
	}

	/* For each file name argument in command line */
	size_t i = 1;
	while (i < args.size()) {
		output_file(args[i].c_str());
		i++;
	}
	return EXIT_SUCCESS;
}

int API::help(std::vector<std::string> args) {
	if (args.size() > 1) {
		if (args.size() == 2) {
			for (size_t i = 0; i < sizeof(API::builtin_list) / sizeof(API::builtin_list[0]); i++) {
				if (args[1] == API::builtin_list[i]) {
					print(args[1] + " " + API::builtin_man[i], true);
					return EXIT_SUCCESS;
				} else if (args[1] == "color") {
					print("color <int (0-7)>\nColors:\n");
					int fix = 10;
					print(charfix("0: BLACK", fix) + charfix("1: RED", fix) + charfix("2: GREEN", fix) + charfix("3: GOLD", fix) + '\n');
					print(charfix("4: BLUE", fix) + charfix("5: PURPLE", fix) + charfix("6: CYAN", fix) + charfix("7: GREY (default)", fix) + '\n');
					return EXIT_SUCCESS;
				}
			}
		} else {
			error("Usage: " + API::builtin_man[API::Commands::HELP]);
		}
		return EXIT_FAILURE;
	}
	// assuming help all functions
	cprint("Use `help [command]` too see a command\'s usage", bright_red, true);
	print(charfix("color", 8) + " Change the color of the prompt", true);

	for (size_t i = 0; i < sizeof(API::builtin_list) / sizeof(API::builtin_list[0]); i++) {
		print(charfix(API::builtin_list[i], 8) + " " + API::builtin_desc[i], true);
	}
	return EXIT_SUCCESS;
}

int API::ruler(std::vector<std::string> args) {
	int i, j, w = 80, h = 24;
	for (i = 1; i <= h - 1; i++) {
		for (j = 1; j <= w; j++) {
			ANSI::foreground(white);
			if (j % 10 == 0 && i == 1) {
				ANSI::foreground(red);
				printf("%1d", (j - (j % 10)) / 10);
			} else if (i % 10 == 0 && j == 1) {
				ANSI::foreground(red);
				printf("%1d", (i - (i % 10)) / 10);
			} else if (i == 2) {
				if (j % 10 == 0)
					ANSI::foreground(red);
				printf("%1d", j % 10);
			} else if (j == 2) {
				if (i % 10 == 0)
					ANSI::foreground(red);
				printf("%1d", i % 10);
			} else if (j == 80 && i % 10 == 0) printf("┫");
			else if (j % 10 == 0 && i % 10 == 0)
				printf("╋");
			else if (j % 10 == 0)
				printf("┃");
			else if (i % 10 == 0)
				printf("━");
			else
				printf(" ");
		}
		printf("\n");
	}
	ANSI::foreground(red);
	printf("%d ", h);
	ANSI::reset();
	return EXIT_SUCCESS;
}

int API::welcome(std::array<int, 4> colors) {
	cprint("╔═════════════════════════════════╤════════════════════════════════════════════╗", colors[1], true);
	cprint("║",colors[1]); cprint("        ███████████            ",colors[0]);cprint("│", colors[1]); cprint(" Vader " + charfix(VADER::API::VERSION, 8), colors[0]);
		cprint("                             ║", colors[1], true);
	cprint("║",colors[1]); cprint("          █                ",colors[0]);cprint("│", colors[1]); cprint(" Copyright Logan Houston 2022.              ", colors[0]);cprint("║", colors[1], true);
	cprint("║",colors[1]); cprint("       ▓▓ █ █▓ ▓▓▓           ",colors[0]);cprint("│", colors[1]); cprint(" All rights reserved.                       ", colors[0]);cprint("║", colors[1], true);
	cprint("║",colors[1]); cprint("      █ ▓      █▓          ",colors[0]);cprint("│", colors[1]); cprint(" Vader recommends using a nerd font.        ", colors[0]);cprint("║", colors[1], true);
	cprint("║",colors[1]); cprint("      █ ▓  █ █     █   █         ",colors[0]);cprint("│", colors[1]); cprint("                                            ", colors[0]);cprint("║", colors[1], true);
	cprint("║",colors[1]); cprint("      █▄▄▄▄██ ▄▄▄           ",colors[0]);cprint("│", colors[1]); cprint("                                            ", colors[0]);cprint("║", colors[1], true);
	cprint("║",colors[1]); cprint("         █    █          ",colors[0]);cprint("│", colors[1]); cprint("                                            ", colors[0]);cprint("║", colors[1], true);
	cprint("║",colors[1]); cprint("         █   █          ",colors[0]);cprint("│", colors[1]); cprint("                                            ", colors[0]);cprint("║", colors[1], true);
	cprint("║",colors[1]); cprint("      ▀▀▀▀▀▀▀▀▀▀█          ",colors[0]);cprint("│", colors[1]); cprint("                                            ", colors[0]);cprint("║", colors[1], true);
	cprint("║",colors[1]); cprint("        ▄▄▄▄▄  █      █     ",colors[0]);cprint("│", colors[1]); cprint("                                            ", colors[0]);cprint("║", colors[1], true);
	cprint("║",colors[1]); cprint("      ▓ ▓ ▓ ██   ░░░▓▓▓    ",colors[0]);cprint("│", colors[1]); cprint("                                            ", colors[0]);cprint("║", colors[1], true);
	cprint("║",colors[1]); cprint(" █      █▓ ▓ ▓  █ ░░░    ▓▓   ",colors[0]);cprint("│", colors[1]); cprint("                                            ", colors[0]);cprint("║", colors[1], true);
	cprint("║",colors[1]); cprint(" █      ███████░░░         ▓  ",colors[0]);cprint("│", colors[1]); cprint("                                            ", colors[0]);cprint("║", colors[1], true);
	cprint("║",colors[1]); cprint("                            █  ",colors[0]);cprint("│ ", colors[1]);
	for (int x = 0; x < 2; x++) {
		for (int i = 0; i < 7; i++) {
			cprint("  ", reset, i + (x * 60));
		}
		cprint("                             ║", colors[1], true);
		if (x == 0) {
			cprint("║", colors[1]); cprint("  ███████████████████████████  ", colors[0]); cprint("│ ", colors[1]);
		}
	}
	cprint("╚═════════════════════════════════╧════════════════════════════════════════════╝", colors[1], true);

	return EXIT_SUCCESS;
}

int API::_welcome(std::vector<std::string> args) {
	if (args.size() != 5) {
		error("Something went wrong passing the colors to the command");
		return EXIT_FAILURE;
	}
	std::array<int, 4> arg2 = { stoi(args[1]), stoi(args[2]), stoi(args[3]), stoi(args[4]) };
	return API::welcome(arg2);
}

int API::clear() {
#ifdef _WIN32
	API::launch({ "cls" });
#elif defined __unix__
	API::launch({ "clear" });
#endif
	return EXIT_SUCCESS;
}

int API::_clear(std::vector<std::string> args) {
	return API::clear();
}

int API::info(std::vector<std::string> args) {
	if (args.size() != 2) {
		error("Usage: info " + builtin_man[API::INFO] + "\nTypes: mem, cpu");
		return EXIT_FAILURE;
	}
	// unsigned short procs;
	if (args[1] == "mem") {
#ifdef _WIN32
#include <winbase.h>
		float totalRam, freeRam, usedRam, totalVirtual, freeVirtual;
		DWORD memLoad;
		struct _MEMORYSTATUS memstat;

		GlobalMemoryStatus(&memstat);
		totalRam = memstat.dwTotalPhys / 1073741824.f;
		freeRam = memstat.dwAvailPhys / 1073741824.f;
		usedRam = totalRam - freeRam;
		totalVirtual = memstat.dwTotalVirtual / 1099511627776.f;
		freeVirtual = memstat.dwAvailVirtual / 1099511627776.f;
		memLoad = memstat.dwMemoryLoad;
		int c, bc;
		if (memLoad >= 75) {
			bc = red;
			c = gray;
		} else if (memLoad >= 50) {
			bc = yellow;
			c = black;
		} else if (memLoad >= 25) {
			bc = bright_green;
			c = dark_gray;
		} else {
			bc = green;
			c = black;
		}

		ANSI::background(bc);
		ANSI::foreground(c);
		printf("%s: %lu%%\n", charfix("% Used", 8).c_str(), memLoad);
		ANSI::reset();
		printf("%s: %.2f GiB\n", charfix("Total Ram", 16).c_str(), totalRam);
		printf("%s: %.2f GiB\n", charfix("Free ram", 16).c_str(), freeRam);
		if (usedRam < 1) {
			usedRam *= 1024;
			printf("%s: %.0f Mib\n", charfix("Used Ram", 16).c_str(), usedRam);
		} else {
			printf("%s: %.2f GiB\n", charfix("Used Ram", 16).c_str(), usedRam);
		}
		printf("%s: %.0f TiB\n", charfix("Total Virtual Mem", 24).c_str(), totalVirtual);
		printf("%s: %.0f TiB\n", charfix("Free Virtual Mem", 24).c_str(), freeVirtual);
#elif defined __unix__
		float totalRam, freeRam, usedRam, cached, bufferRam, totalSwap, freeSwap, memLoad;
		totalRam = get_mem_total() / 1048576.f;
		freeRam = get_mem_free() / 1048576.f;
		usedRam = totalRam - freeRam;
		memLoad = usedRam / totalRam;
		bufferRam = get_buffer_mem() / 1024.f;
		totalSwap = get_swap_mem_total() / 1048576.f;
		freeSwap = get_swap_mem_free() / 1048576.f;
		cached = get_cached_mem() / 1024.f;
		int c, bc;
		if (memLoad >= 75) {
			bc = red;
			c = gray;
		} else if (memLoad >= 50) {
			bc = yellow;
			c = black;
		} else if (memLoad >= 25) {
			bc = bright_green;
			c = dark_gray;
		} else {
			bc = green;
			c = black;
		}
		ANSI::background(bc);
		ANSI::foreground(c);
		printf("%s: %.0f%%\n", charfix("% Used", 8), memLoad);
		ANSI::reset();
		printf("%s: %.2f GiB\n", charfix("Total Ram", 16), totalRam);
		printf("%s: %.2f GiB\n", charfix("Free ram", 16), freeRam);
		if (usedRam < 1) {
			usedRam *= 1024;
			printf("%s: %.0f Mib\n", charfix("Used Ram", 16), usedRam);
		} else {
			printf("%s: %.2f GiB\n", charfix("Used Ram", 16), usedRam);
		}
		printf("%s: %.0f MiB\n", charfix("Buffer Mem:", 16), bufferRam);
		printf("%s: %.0f MiB\n", charfix("Cached Mem", 16), cached);
		printf("%s: %.0f MiB\n", charfix("Total Swap Mem", 16), totalSwap);
		printf("%s: %.0f MiB\n", charfix("Free Swap Mem", 16), freeSwap);
#endif
		return EXIT_SUCCESS;
	} else if (args[1] == "cpu") {
#ifdef _WIN32

#elif defined __unix__
		typedef struct { std::string data[7]; } thread;
		typedef struct { std::vector<thread> data; } threads;
		//cores, siblings, cache, cpuMHZ, cpu_name, cpu_brand, id;
		threads _threads;
		thread temp = { { "", "", "", "", "", "", "" } };
		std::ifstream cpuinfo("/proc/cpuinfo");
		std::string line;
		int args = 0;
		while (std::getline(cpuinfo, line, '\n')) {
			std::string name = "", val = "";
			std::stringstream x(line);
			for (size_t i = 0; i < 2; i++) {
				if (i == 0) {
					getline(x, name, ':');
					for (int i = 0; i < name.size(); i++) {
						if (name[i] == '\t') name.erase(i);
					}
				}
				else {
					std::getline(x, val, '\n');
					val.erase(0, 1);
					args++;
					if (name.find("cpu cores") != std::string::npos) temp.data[0] = val;
					else if (name.find("siblings") != std::string::npos) temp.data[1] = val;
					else if (name.find("cache size") != std::string::npos) temp.data[2] = val;
					else if (name.find("cpu MHz") != std::string::npos) temp.data[3] = val;
					else if (name.find("model name") != std::string::npos) temp.data[4] = val;
					else if (name.find("vendor_id") != std::string::npos) temp.data[5] = val;
					else if (name.find("physical id") != std::string::npos) temp.data[6] = val;
					else args--;
					if (args == 7) {
						_threads.data.push_back(temp);
						args = 0;
					}
				}
			}
		}
		if (cpuinfo.is_open()) cpuinfo.close();
		std::vector<std::string> printed_cpus;
		for (size_t i = 0; i < _threads.data.size(); i++) {
			bool cpu_printed = false;
			for (size_t j = 0; j < printed_cpus.size(); j++) {
				if (printed_cpus[j] == _threads.data[i].data[6]) {
					cpu_printed = true;
					break;
				}
			}
			if (cpu_printed) continue;
			else printed_cpus.push_back(_threads.data[i].data[6]);
			if (i != 0) printf("------------------------");
			print(charfix("Name", 8) + ": " + _threads.data[i].data[4], true);
			print(charfix("Brand", 8) + ": " + _threads.data[i].data[5], true);
			print(charfix("MHz", 8) + ": " + _threads.data[i].data[3], true);
			print(charfix("Cores", 8) + ": " + _threads.data[i].data[0], true);
			print(charfix("Threads", 8) + ": " + _threads.data[i].data[1], true);
			print(charfix("Cache", 8) + ": " + _threads.data[i].data[2], true);
		}
#endif
		return EXIT_SUCCESS;
	} else {
		error("Usage: info " + builtin_man[API::INFO] + "\nTypes: mem, cpu");
		return EXIT_FAILURE;
	}
}

int API::version(std::vector<std::string> args) {
	cprint("Vader " + API::VERSION + '\n', purple);
	return EXIT_SUCCESS;
}
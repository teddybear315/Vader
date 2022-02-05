#include "include/appfuncs.h"
#include "include/colors.h"
#include "include/icons.h"

#include <iostream>
#include <vector>
#include <iterator>
#include <sstream>

void print() {
	std::cout << std::endl;
}

std::string lower(std::string text) {
	for (size_t i = 0; i <= text.size(); i++)
		if (text[i] >= 65 && text[i] <= 90) text[i] = text[i] + 32;
	return text;
}

char lower(char text) {
	if (text >= 65 && text <= 90) return text + 32;
	return text;
}

std::string error(std::string _err) {
	cprint("✘: " + _err, red, true);
	return _err;
}

std::string print(std::string text) {
	std::cout << text;
	return text;
}

std::string print(std::string text, bool newLine) {
	std::cout << text;
	if (newLine) std::cout << std::endl;
	return text;
}

std::string cprint(std::string text, int foreground) {
	print("\033[" + std::to_string(foreground + 30) + "m" + text + "\033[0m", false);
	return text;
}

std::string cprint(std::string text, int foreground, int background) {
	print("\033[" + std::to_string(foreground + 30) + ";" + std::to_string(background + 40) + "m" + text + "\033[0m", false);
	return text;
}

std::string cprint(std::string text, int foreground, bool newLine) {
	print("\033[" + std::to_string(foreground + 30) + "m" + text + "\033[0m", newLine);
	return text;
}

std::string cprint(std::string text, int foreground, int background, bool newLine) {
	print("\033[" + std::to_string(foreground + 30) + ";" + std::to_string(background + 40) + "m" + text + "\033[0m", newLine);
	return text;
}

int ANSI::foreground(int foreground) {
	print("\033[" + std::to_string(foreground + 30) + "m");
	return foreground;
}

int ANSI::background(int background) {
	print("\033[" + std::to_string(background + 40) + "m");
	return background;
}

void ANSI::reset() { print("\033[0m"); }

std::vector<std::string> parse(std::string input) {
	std::vector<std::string> argv;
	std::istringstream iss(input);
	for (std::string s; iss >> s;) {
		argv.push_back(s);
	}

	int endq = -1, startq = -1;
	std::vector<int[2]> markers;

	for (size_t i = 0; i < argv.size(); i++) {
		if (argv[i][0] == '\"') {
			startq = i;
			argv[i].erase(argv[i].begin());
		}
		if (argv[i][argv[i].size() - 1] == '\"') {
			endq = i;
			argv[i].erase(argv[i].end() - 1);
		}
		if (startq != -1 && endq != -1) {
			int len = 1 + (endq - startq);
			std::string str;
			for (int j = 0; j < len; j++) {
				str.append(argv[startq + j]);
				if (j != len - 1) str += ' ';
			}
			argv[startq] = str;
			if (endq - (startq + 1) == 0) argv.erase(argv.begin() + startq + 1);
			else argv.erase(argv.begin() + startq + 1, argv.begin() + endq + 1);
			startq = -1;
			endq = -1;
		}
	}

#ifdef _DEBUG
	std::string test;
	for (size_t i = 0; i < argv.size(); i++) {
		test.append('\'' + argv[i] + "\' ");
	}
	print("[Vader DEBUG] Parser: " + test, true);
#endif
	return argv;
}

std::string charfix(std::string in, int chars) {
	std::string out;
	if (in.size() > chars) return in;
	out = in;
	chars -= in.size();
	for (int i = 0; i < chars; i++) out.append(" ");
	return out;
}

int resultIndex(std::vector<std::string> arr, std::string k) {
	for (int i = 0; i < arr.size(); i++) {
		if (arr.at(i) == k) {
			return i;
		}
	}
	return -1;
}

#ifdef __unix__
#include <cstring>
#include <sys/sysinfo.h>
#include <fstream>
#include <algorithm>
unsigned long get_mem_total() {
	std::string token;
	std::ifstream file("/proc/meminfo");
	while (file >> token) {
		if (token == "MemTotal:") {
			unsigned long mem;
			if (file >> mem) {
				return mem;
			} else {
				return 0;
			}
		}
		// Ignore the rest of the line
		file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	return 0; // Nothing found
}
unsigned long get_mem_free() {
	std::string token;
	std::ifstream file("/proc/meminfo");
	while (file >> token) {
		if (token == "MemFree:") {
			unsigned long mem;
			if (file >> mem) {
				return mem;
			} else {
				return 0;
			}
		}
		// Ignore the rest of the line
		file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	return 0; // Nothing found
}
unsigned long get_buffer_mem() {
	std::string token;
	std::ifstream file("/proc/meminfo");
	while (file >> token) {
		if (token == "Buffers:") {
			unsigned long mem;
			if (file >> mem) {
				return mem;
			} else {
				return 0;
			}
		}
		// Ignore the rest of the line
		file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	return 0; // Nothing found
}
unsigned long get_swap_mem_total() {
	std::string token;
	std::ifstream file("/proc/meminfo");
	while (file >> token) {
		if (token == "SwapTotal:") {
			unsigned long mem;
			if (file >> mem) {
				return mem;
			} else {
				return 0;
			}
		}
		// Ignore the rest of the line
		file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	return 0; // Nothing found
}
unsigned long get_swap_mem_free() {
	std::string token;
	std::ifstream file("/proc/meminfo");
	while (file >> token) {
		if (token == "SwapFree:") {
			unsigned long mem;
			if (file >> mem) {
				return mem;
			} else {
				return 0;
			}
		}
		// Ignore the rest of the line
		file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	return 0; // Nothing found
}
unsigned long get_cached_mem() {
	std::string token;
	std::ifstream file("/proc/meminfo");
	while (file >> token) {
		if (token == "Cached:") {
			unsigned long mem;
			if (file >> mem) {
				return mem;
			} else {
				return 0;
			}
		}
		// Ignore the rest of the line
		file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	return 0; // Nothing found
}
#endif

void printcaret(int before_color, int after_color, bool head) {
	cprint(" ", black, before_color);
	cprint(icons::LEFT_CARET, before_color, after_color);
	if (head) print(" ", false);
	else cprint(" ", reset, after_color);
}

#ifdef _WIN32
#include <Windows.h>
LPWSTR ConvertString(const std::string& instr) {
	// Assumes std::string is encoded in the current Windows ANSI codepage
	int bufferlen = ::MultiByteToWideChar(CP_ACP, 0, instr.c_str(), instr.size(), NULL, 0);

	if (bufferlen == 0) {
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
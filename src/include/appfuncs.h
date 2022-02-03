#ifndef APPFUNC_H	
#define APPFUNC_H

#include <iostream>
#include <vector>

void clear();

char lower(char text);

std::string lower(std::string text);
std::string error(std::string _err);
void print();
std::string print(std::string text);
std::string print(std::string text, bool newLine);
std::string cprint(std::string text, int foreground);
std::string cprint(std::string text, int foreground, int background);
std::string cprint(std::string text, int foreground, bool newLine);
std::string cprint(std::string text, int foreground, int background, bool newLine);
int resultIndex(std::vector<std::string> arr, std::string k);
std::string charfix(std::string in, int chars);
void printcaret(int before_color, int after_color, bool head = false);

namespace ANSI {
	int foreground(int foreground);
	int background(int background);
	void reset();
}

std::vector<std::string> parse(std::string input);

#ifdef __unix__
unsigned long get_mem_total();
unsigned long get_mem_free();
unsigned long get_buffer_mem();
unsigned long get_swap_mem_total();
unsigned long get_swap_mem_free();
unsigned long get_cached_mem();
#endif

#ifdef _WIN32
#include <Windows.h>
LPWSTR ConvertString(const std::string& instr);
#endif

#endif
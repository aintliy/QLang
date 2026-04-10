#include "Diagnostics.h"
#include <cstdlib>
#include <cstdio>
#include <string>

void error(int line, int col, const std::string& message) {
    fprintf(stderr, "error at line %d, col %d: %s\n", line, col, message.c_str());
    exit(1);
}

void error(const std::string& message) {
    fprintf(stderr, "error: %s\n", message.c_str());
    exit(1);
}
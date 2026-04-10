#pragma once
#include <string>

void error(int line, int col, const std::string& message);
void error(const std::string& message);  // without location
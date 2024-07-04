#pragma once

#include <string>
#include <vector>
std::string trim(const std::string& str);

bool startsWith(const std::string& str, const std::string& prefix);

std::vector<std::string> splitString(std::string& text, char delimiter);

std::string replaceString(std::string str, const std::string& from,
                          const std::string& to);

std::string removeWhitespace(std::string str);

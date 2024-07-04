#include "common/parsing/Utils.hpp"

#include <algorithm>
#include <sstream>

std::string trim(const std::string& str) {
  auto start = std::find_if_not(str.begin(), str.end(), ::isspace);
  auto end = std::find_if_not(str.rbegin(), str.rend(), ::isspace).base();
  return (start < end) ? std::string(start, end) : std::string();
}

bool startsWith(const std::string& str, const std::string& prefix) {
  return str.compare(0, prefix.size(), prefix) == 0;
}

std::vector<std::string> splitString(std::string& text, char delimiter) {
  std::vector<std::string> result;
  std::istringstream iss(text);
  for (std::string s; std::getline(iss, s, delimiter);) {
    result.push_back(s);
  }
  return result;
}

std::string replaceString(std::string str, const std::string& from,
                          const std::string& to) {
  size_t startPos = 0;
  while ((startPos = str.find(from, startPos)) != std::string::npos) {
    str.replace(startPos, from.length(), to);
    startPos += to.length(); // Handles case where 'to' is a substring of 'from'
  }
  return str;
}

std::string removeWhitespace(std::string str) {
  str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end());
  return str;
}

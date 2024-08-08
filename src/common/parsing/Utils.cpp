#include "common/parsing/Utils.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <vector>

std::string trim(const std::string& str) {
  auto start = std::find_if_not(str.begin(), str.end(), ::isspace);
  auto end = std::find_if_not(str.rbegin(), str.rend(), ::isspace).base();
  return (start < end) ? std::string(start, end) : std::string();
}

bool startsWith(const std::string& str, const std::string& prefix) {
  return str.compare(0, prefix.size(), prefix) == 0;
}

std::vector<std::string> splitString(const std::string& text, char delimiter) {
  const std::vector<char> delimiters{delimiter};
  return splitString(text, delimiters);
}

std::vector<std::string> splitString(const std::string& text,
                                     const std::vector<char>& delimiters) {
  std::vector<std::string> result;
  size_t pos = 0;
  while (true) {
    size_t min = std::string ::npos;
    for (const auto del : delimiters) {
      const size_t newPos = text.find(del, pos);
      min = newPos < min ? newPos : min;
    }
    if (min == std::string::npos) {
      break;
    }
    result.push_back(text.substr(pos, min - pos));
    pos = min + 1;
  }
  result.push_back(text.substr(pos, text.length() - pos));
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

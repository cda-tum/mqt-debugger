/**
 * @file Utils.cpp
 * @brief Implementation of utility functions used by the debugger.
 */

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

std::vector<std::string> splitString(const std::string& text, char delimiter,
                                     bool includeEmpty) {
  const std::vector<char> delimiters{delimiter};
  return splitString(text, delimiters, includeEmpty);
}

std::vector<std::string> splitString(const std::string& text,
                                     const std::vector<char>& delimiters,
                                     bool includeEmpty) {
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
    if (min > pos || includeEmpty) {
      result.push_back(text.substr(pos, min - pos));
    }
    pos = min + 1;
  }
  if (text.length() > pos || includeEmpty) {
    result.push_back(text.substr(pos, text.length() - pos));
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

bool variablesEqual(const std::string& v1, const std::string& v2) {
  if (v1.find('[') != std::string::npos && v2.find('[') != std::string::npos) {
    return v1 == v2;
  }
  if (v1.find('[') != std::string::npos) {
    return variablesEqual(splitString(v1, '[')[0], v2);
  }
  if (v2.find('[') != std::string::npos) {
    return variablesEqual(splitString(v2, '[')[0], v1);
  }
  return v1 == v2;
}

std::string variableBaseName(const std::string& v) {
  return splitString(v, '[')[0];
}

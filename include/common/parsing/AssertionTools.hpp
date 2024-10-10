#pragma once

#include "AssertionParsing.hpp"
#include "CodePreprocessing.hpp"

#include <memory>
#include <string>

bool doesCommute(const std::unique_ptr<Assertion>& assertion,
                 const Instruction& instruction);

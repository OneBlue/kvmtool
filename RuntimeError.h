#pragma once

#include <stdexcept>
#include <experimental/source_location>

using source_location = std::experimental::source_location;

class RuntimeError : public std::runtime_error
{
public:
  RuntimeError(const std::string& what, const source_location& source = source_location::current());
};

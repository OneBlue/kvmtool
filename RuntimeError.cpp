#include <sstream>
#include "RuntimeError.h"


static std::string Format(const std::string& what, const source_location& where)
{
  std::stringstream str;
  str << "RuntimeError thrown from: " << where.function_name() << "(" << where.file_name() << ":" << where.line() << "): ";
  str << what;

  return str.str();
}

RuntimeError::RuntimeError(const std::string& what,
                           const source_location& where)
    : std::runtime_error(Format(what, where))
{
}

#pragma once

#include <vector>

template <typename T>
struct is_vector
{
  static const bool value = false;
};

template <typename T>
struct is_vector<std::vector<T>>
{
  static const bool value = true;
};

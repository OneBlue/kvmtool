#pragma once

#include <iostream>
struct Position
{
  int x;
  int y;
  unsigned int width;
  unsigned int height;
};

std::ostream& operator<<(std::ostream& str, const Position& position);

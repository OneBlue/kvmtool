#include "Position.h"

std::ostream& operator<<(std::ostream& str, const Position& position)
{
  return str << "x=" << position.x << ", y=" << position.y
             << ", width=" << position.width << ", height= " << position.height;
}

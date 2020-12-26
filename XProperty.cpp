#include "XProperty.h"
#include <utility>

XProperty::XProperty(void* addr, Atom type, unsigned long items)
    : _addr(addr), _type(type), _items(items)
{
}

XProperty::~XProperty()
{
  if (_addr != nullptr)
  {
    XFree(_addr);
  }
}

XProperty::XProperty(XProperty&& other)
{
  *this = std::move(other);
}

XProperty& XProperty::operator=(XProperty&& other)
{
  _addr = other._addr;
  _type = other._type;
  _items = other._items;

  other._addr = nullptr;

  return *this;
}

unsigned long XProperty::Items() const
{
  return _items;
}

void* XProperty::Data() const
{
  return _addr;
}


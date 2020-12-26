#pragma once

#include <X11/Xatom.h>
#include <X11/Xlib.h>

class XProperty
{
  public:
    XProperty(void* addr, Atom type, unsigned long items);
    ~XProperty();


    XProperty(XProperty&& other);
    XProperty& operator=(XProperty&& other);

    XProperty(const XProperty& other) = delete;
    XProperty& operator=(const XProperty& other) = delete;

    unsigned long Items() const;

    void* Data() const;

  private:
    void* _addr;
    Atom _type;
    unsigned long _items;
};

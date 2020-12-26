#pragma once


#include <vector>
#include <string>
#include <X11/Xlib.h>
#include <type_traits>
#include "traits.h"
#include "Position.h"
#include "XProperty.h"

class XWindow
{
  public:
    XWindow(Display* display, Window window);

    std::vector<XWindow> Children();
    Position CurrentPosition();

    std::string Title();

    void SetPosition(const Position& position);
    
    std::vector<unsigned long> WmState();

    void SetWmState(const std::vector<unsigned long>& state, bool set);

    Window WindowHandle() const;

  private:
    template <typename T>
    std::enable_if_t<is_vector<T>::value, T> GetProperty(const char* name, Atom type);

    template <typename T>
    std::enable_if_t<!is_vector<T>::value, T> GetProperty(const char* name, Atom type);

    XProperty GetPropertyImpl(const char* name, Atom type);

    void SendRawEvent(const char* type, const std::vector<unsigned long>& data);

    bool GetStateFlag(const char* name);
    void SetStateFlag(const char* name, bool flag);

  private:
    Display* _display;
    Window _window;
};

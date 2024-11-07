#include <algorithm>
#include <X11/Xatom.h>
#include <map>
#include <thread>

#include "XWindow.h"
#include "RuntimeError.h"

constexpr auto MaxPropertyName = 40960;

XWindow::XWindow(Display* display, Window window)
    : _display(display), _window(window)
{
}

template <typename T>
inline std::enable_if_t<is_vector<T>::value, T>
XWindow::GetProperty(const char* name, Atom type)
{

  auto property = GetPropertyImpl(name, type);

  T output;
  using TElement = std::decay<decltype(*output.begin())>::type;
  for (size_t i = 0; i < property.Items(); i++)
  {
    output.emplace_back(reinterpret_cast<TElement*>(property.Data())[i]);
  }

  return output;
}

template <typename T>
inline std::enable_if_t<!is_vector<T>::value, T>
XWindow::GetProperty(const char* name, Atom type)
{
  auto property = GetPropertyImpl(name, type);

  if constexpr (std::is_same_v<T, std::string>)
  {
    return std::string{reinterpret_cast<char*>(property.Data())};
  }
  else
  {
    static_assert(sizeof(T) != sizeof(T), "Unsupported property type");
  }
}

XProperty XWindow::GetPropertyImpl(const char* name, Atom type)
{
  Atom actual_type{};
  int ret_format = 0;
  unsigned long items = 0;
  unsigned long _;
  unsigned char* buffer = nullptr;

  auto atom = XInternAtom(_display, name, False);
  auto result = XGetWindowProperty(_display,
                                   _window,
                                   atom,
                                   0,
                                   MaxPropertyName,
                                   false,
                                   type,
                                   &actual_type,
                                   &ret_format,
                                   &items,
                                   &_,
                                   &buffer);

  if (result != Success)
  {
    throw RuntimeError("XGetWindowProperty failed, " + std::to_string(result));
  }

  if (actual_type != type)
  {
    throw RuntimeError("Unexpected property type: " +
                       std::to_string(actual_type) + " for property: " + name);
  }

  return {buffer, type, items};
}

std::vector<XWindow> XWindow::Children()
{
  auto children =
      GetProperty<std::vector<Window>>("_NET_CLIENT_LIST", XA_WINDOW);

  std::vector<XWindow> windows;
  std::transform(children.begin(),
                 children.end(),
                 std::back_inserter(windows),
                 [&](const auto& e) {
                   return XWindow{_display, e};
                 });

  return windows;
}

std::string XWindow::Title()
{
  return GetProperty<std::string>("_NET_WM_NAME",
                                  XInternAtom(_display, "UTF8_STRING", False));
}

Position XWindow::CurrentPosition()
{
  Window root{};
  unsigned int _;
  unsigned int __;
  Position position{};

  auto result = XGetGeometry(_display,
                             _window,
                             &root,
                             &position.x,
                             &position.y,
                             &position.width,
                             &position.height,
                             &_,
                             &__);

  if (result == 0)
  {
    throw RuntimeError("GetGeometry failed, " + std::to_string(result));
  }

  result = XTranslateCoordinates(_display,
                                 _window,
                                 root,
                                 position.x,
                                 position.y,
                                 &position.x,
                                 &position.y,
                                 &root);

  return position;
}

void XWindow::SendRawEvent(const char* type,
                           const std::vector<unsigned long>& data)
{
  XEvent event{};
  event.xclient.type = ClientMessage;
  event.xclient.serial = 0;
  event.xclient.send_event = true;
  event.xclient.message_type = XInternAtom(_display, type, False);
  event.xclient.window = _window;
  event.xclient.format = 32;
  event.xclient.display = _display;

  for (size_t i = 0;
       i < std::min(data.size(), sizeof(event.xclient.data.l) / sizeof(long));
       i++)
  {
    event.xclient.data.l[i] = data[i];
  }

  if (XSendEvent(_display,
                 DefaultRootWindow(_display),
                 true,
                 SubstructureRedirectMask | SubstructureNotifyMask,
                 &event) == 0)
  {
    throw RuntimeError("Failed to send event " + std::string(type) +
                       " on window: " + std::to_string(_window));
  }

  XFlush(_display);
}

void XWindow::SetPosition(const Position& position)
{
  /*
   * Experiments have shown that sending a MOVERSIZE_WINDOW event don't work
   * under gnome & derivates if any of the MAXIMIZED_* flags are set.
   * To work around that, this method saves, removes, and restores these flags
   */

  auto state = WmState();

  auto v_atom = XInternAtom(_display, "_NET_WM_STATE_MAXIMIZED_VERT", true);
  auto h_atom = XInternAtom(_display, "_NET_WM_STATE_MAXIMIZED_HORZ", true);
  auto fullscreen = XInternAtom(_display, "_NET_WM_STATE_FULLSCREEN", true);

  auto new_state = state;
  new_state.erase(std::remove_if(new_state.begin(),
                                 new_state.end(),
                                 [&](auto e) {
                                   return e == v_atom || e == h_atom ||
                                          e == fullscreen;
                                 }),
                  new_state.end());

  if (std::find(state.begin(), state.end(), fullscreen) != state.end())
  {
    std::cerr << "Removing fullscreen state from window " << Title()
              << std::endl;
    SetWmState({fullscreen}, false);
  }

  SetWmState({v_atom, h_atom}, false);

  int flags = (1 << 8) | (1 << 9) | (1 << 10) | (1 << 11);
  SendRawEvent("_NET_MOVERESIZE_WINDOW",
               {static_cast<unsigned long>(flags),
                static_cast<unsigned long>(position.x),
                static_cast<unsigned long>(position.y),
                position.width,
                position.height});

  SetWmState(state, true);
  if (std::find(state.begin(), state.end(), fullscreen) != state.end())
  {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    SetWmState({fullscreen}, true);
  }
}

std::vector<unsigned long> XWindow::WmState()
{
  return GetProperty<std::vector<unsigned long>>("_NET_WM_STATE", XA_ATOM);
}

bool XWindow::GetStateFlag(const char* name)
{
  auto flags = WmState();
  auto atom = XInternAtom(_display, name, true);

  return std::find(flags.begin(), flags.end(), atom) != flags.end();
}

void XWindow::SetWmState(const std::vector<unsigned long>& state, bool set)
{
  auto data = state;
  data.insert(data.begin(), set);

  SendRawEvent("_NET_WM_STATE", data);
}

Window XWindow::WindowHandle() const
{
  return _window;
}

void XWindow::Activate()
{
  SendRawEvent("_NET_ACTIVE_WINDOW", {});
  XMapRaised(_display, _window);

  auto fullscreen = XInternAtom(_display, "_NET_WM_STATE_FULLSCREEN", true);
  auto state = WmState();
  if (std::find(state.begin(), state.end(), fullscreen) != state.end())
  {
    // Corner case for fullscreen windows: They can be 'broken' if they aren't
    // set to non-fullscreen and back
    SetWmState({fullscreen}, false);

    std::this_thread::sleep_for(std::chrono::seconds(3));
    SetWmState({fullscreen}, true);
  }
}

#include <iostream>
#include <chrono>
#include <thread>
#include <sstream>
#include <X11/extensions/randr.h>
#include <X11/extensions/Xrandr.h>
#include <getopt.h>
#include "XWindow.h"
#include "RuntimeError.h"

struct WindowState
{
  XWindow window;
  Position position;
  std::vector<unsigned long> state;
};

using timepoint = std::chrono::system_clock::time_point;

static std::vector<WindowState>
GetWinddowsState(XWindow& window, const std::vector<std::string>& exclude)
{
  std::vector<WindowState> windows;

  for (auto& e : window.Children())
  {
    bool excluded = false;
    try
    {
      excluded =
          std::find(exclude.begin(), exclude.end(), e.Title()) != exclude.end();
    }
    catch (const std::exception& ex)
    {
      std::cerr << "Couldn't read title for window: " << e.WindowHandle()
                << ", " << ex.what() << std::endl;
    }

    if (!excluded)
    {
      windows.emplace_back(WindowState{e, e.CurrentPosition(), e.WmState()});
    }
  }

  return windows;
}

void RestoreWindows(std::vector<WindowState>& state)
{
  for (auto& e : state)
  {
    try
    {
      std::cerr << "Restoring window: " << e.window.Title() << " -> "
                << e.position << std::endl;

      e.window.SetPosition(e.position);
    }
    catch (const std::exception& ex)
    {
      std::cerr << "Error while restoring window: " << e.window.WindowHandle()
                << ", " << ex.what() << std::endl;
    }
  }
}

void Run(Display* display,
         XWindow root,
         size_t period_ms,
         size_t event_timeout_ms,
         int original_x,
         int original_y,
         const std::vector<std::string>& exclude)
{
  if (XSelectInput(display, root.WindowHandle(), RRScreenChangeNotifyMask) == 0)
  {
    throw RuntimeError("XSelectInput failed");
  }

  XRRSelectInput(display, root.WindowHandle(), RRScreenChangeNotifyMask);

  int rr_event_base = 0;
  int rr_error_base = 0;
  if (!XRRQueryExtension(display, &rr_event_base, &rr_error_base))
  {
    throw RuntimeError("X11 RR extension is not available");
  }

  std::vector<WindowState> state;
  bool all_screens_present = true;
  timepoint last_event_ts;
  while (true)
  {

    /* Race condition: It's in theory possible that the resolution changed
     * just after the last XPending() call. That's why we have this extra
     * timeout check here*/
    auto now = std::chrono::system_clock::now();
    if (all_screens_present &&
        (state.empty() ||
         last_event_ts + std::chrono::milliseconds(event_timeout_ms) < now))
    {
      state = GetWinddowsState(root, exclude);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(period_ms));
    while (XPending(display))
    {
      XEvent event;
      XNextEvent(display, &event);

      last_event_ts = std::chrono::system_clock::now();
      if (event.type != rr_event_base + RRScreenChangeNotify)
      {
        continue;
      }

      const auto* screen_event =
          reinterpret_cast<XRRScreenChangeNotifyEvent*>(&event);

      bool original_screens = screen_event->width == original_x &&
                              screen_event->height == original_y;

      if (!all_screens_present && original_screens)
      {
        std::cerr << "Original screens detected" << std::endl;
        RestoreWindows(state);
      }
      else if (all_screens_present && !original_screens)
      {
        std::cerr << "Original screens lost (" << screen_event->width << ", "
                  << screen_event->height << ")" << std::endl;
      }

      all_screens_present = original_screens;
    }
  }
}

void Help(const char* name)
{
  const char* help =
      "Usage: %s -x screen_witdh -y screen_height [--screen-timeout timeout_ms] [--refresh refresh_ms] [--exclude window1,window2]\n\
Options: \n\
	-x: The width, in pixels of the original screen area\n\
	-y: The height, in pixels of the original screen area\n\
	-screen_timeout: The timeout, in milliseconds, to wait for RRScreenChangeNotify events after a new screen is plugged / unplugged\n\
	--exclude: A comma separated list of window titles to exclude when saving / restoring positions\n\
	--refresh: The refresh rate at which windows are to be saved (in milliseconds)\n\
	--help: Display this message\n";

  fprintf(stderr, help, name);

  std::cerr << "Build time: " << __DATE__ << " " <<__TIME__ << std::endl;
}

int main(int argc, char** argv)
{
  option options[] = {{"x", required_argument, 0, 0},
                      {"y", required_argument, 0, 0},
                      {"refresh", required_argument, 0, 0},
                      {"screen-timeout", required_argument, 0, 0},
                      {"exclude", required_argument, 0, 0},
                      {"help", no_argument, 0, 0},
                      {0, 0, 0, 0}};

  auto parse_int = [](const char* str) {
    try
    {
      return std::stoul(str);
    }
    catch (const std::exception& e)
    {
      std::cerr << "Invalid integer value: " << str << ", " << str << std::endl;
      exit(1);
    }
  };

  int x = -1;
  int y = -1;
  size_t refresh_timeout = 5000;
  size_t screen_timeout = 2000;

  std::vector<std::string> exclude;

  int arg = -1;
  int index = -1;
  while ((arg = getopt_long(argc, argv, "x:y:r:s:eh", options, &index)) != -1)
  {
    switch (arg)
    {
      case 'x':
        x = parse_int(optarg);
        break;

      case 'y':
        y = parse_int(optarg);
        break;

      case 'r':
        refresh_timeout = parse_int(optarg);
        break;

      case 's':
        screen_timeout = parse_int(optarg);
        break;

      case 'h':
        Help(argv[0]);
        exit(1);
        break;

      case 'e':
        std::istringstream str(optarg);
        std::string window;
        while (std::getline(str, window, ','))
        {
          exclude.emplace_back(std::move(window));
        }
        break;
    }
  }

  if (optind != argc || x == -1 || y == -1)
  {
    Help(argv[0]);
    return 1;
  }

  auto* display = XOpenDisplay(nullptr);
  if (display == nullptr)
  {
    std::cerr << "Failed to open display" << std::endl;
    return 1;
  }

  Window root = XDefaultRootWindow(display);

  Run(display,
      XWindow{display, root},
      refresh_timeout,
      screen_timeout,
      x,
      y,
      {"nemo-desktop", "Desktop"});

  XCloseDisplay(display);

  return 0;
}


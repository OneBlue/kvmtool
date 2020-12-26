# kvmtool

A daemon to save and restore the state / positions of X11 windows when screen are plugged / unplugged.

This tends to be useful for people (like me) using passive KVM's, because switching between machines causes the screen to be physically detached, and so all windows usually end up in one corner which can be annoying.
This program solves the problem by periodically saving the position of all windows on an X11 display, and restoring them when screens are unplugged and then plugged again.


## Usage

```
Usage: ./kvmtool -x screen_witdh -y screen_height [--screen-timeout timeout_ms] [--refresh refresh_ms] [--exclude window1,window2]
Options:
	-x: The width, in pixels of the original screen area
	-y: The height, in pixels of the original screen area
	-screen_timeout: The timeout, in milliseconds, to wait for RRScreenChangeNotify events after a new screen is plugged / unplugged
	--exclude: A comma separated list of window titles to exclude when saving / restoring positions
	--refresh: The refresh rate at which windows are to be saved (in milliseconds)
	--help: Display this message
```


# Build


Instructions for debian based distros
```
$ apt-get install libx11-dev # Install the libx11 headers
$ git clone https://bitbucket.org/_Blue/kvmtool # Clone this repository
$ cd kvmtool
$ make
```

# clearview
Desktop magnification tool

(definitely not a stolen idea from [tsoding](https://github.com/tsoding/boomer))

Currently supports X11 and Windows. Wayland support is planned, and MacOS support could be possible, but I have no way of testing it.

The CMake file automatically detects your platform and creates the relevant executable. To build the project, simply do the following

```bash
$ mkdir build && cd build
$ cmake -G Ninja ..
$ cmake --build .
```

## Setting a global keybind

You can also set this as a global keybind in your system so that you can invoke this from anywhere. I personally use Ctrl + Alt + B to open it.

### Linux

I forgot the exact instructions for linux, and it's likely that they change slightly depending on distro, but essentially all you'll want to do is find the keybind settings and add a custom one to invoke the following command

```bash
$ ./sh -c '/path/to/clearview_[x11/wayland]'
```

### Windows

To do so on Windows, you'll want to take the executable that was built (`clearview_win.exe`) and create a shortcut. Then, put this shortcut in `C:/ProgramData/Microsoft/Windows/Start Menu/Programs` and right click on it to open its `Properties` window. In here, you'll be able to set the app's keybind. You'll also want to set the window to open minimized so that the command line doesn't get in the way of the screen capture. Then press `Apply` and test it out.

*Note*: If there is a significant delay between when you press the keybind and when the app starts, press `Win + R` and type in `services.msc` and press enter. Then, find `SysMain` and open its properties. You'll want to set the Startup type to disabled and then press `Stop` to close the process. Then restart your machine and the keybind should work almost instantly.

linux:
	g++ -Wall -Werror -Wextra -g `pkg-config --cflags libsecret-1 glib-2.0` gnome.cc `pkg-config --libs libsecret-1 glib-2.0` -o gnome

windows:
	cl.exe windows.cc Advapi32.lib

clean:
	rm -rf gnome windows.exe windows.obj

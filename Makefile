
linux:
	g++ -Wall -Werror -Wextra -g `pkg-config --cflags libsecret-1 glib-2.0` gnome.cc `pkg-config --libs libsecret-1 glib-2.0` -o gnome

windows:
	cl.exe windows.cc Advapi32.lib

dpapi:
	cl.exe dpapi.c

win_auth:
	cl.exe win_auth.cc Advapi32.lib Credui.lib Secur32.lib

clean:
	rm -rf gnome windows.exe windows.obj dpapi.exe dpapi.obj win_auth.exe win_auth.obj

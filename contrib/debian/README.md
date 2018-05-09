
Debian
====================
This directory contains files used to package massgridd/massgrid-qt
for Debian-based Linux systems. If you compile massgridd/massgrid-qt yourself, there are some useful files here.

## massgrid: URI support ##


massgrid-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install massgrid-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your massgrid-qt binary to `/usr/bin`
and the `../../share/pixmaps/massgrid128.png` to `/usr/share/pixmaps`

massgrid-qt.protocol (KDE)


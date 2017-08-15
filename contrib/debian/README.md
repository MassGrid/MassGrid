
Debian
====================
This directory contains files used to package mlgbcoind/mlgbcoin-qt
for Debian-based Linux systems. If you compile mlgbcoind/mlgbcoin-qt yourself, there are some useful files here.

## mlgbcoin: URI support ##


mlgbcoin-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install mlgbcoin-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your mlgbcoin-qt binary to `/usr/bin`
and the `../../share/pixmaps/mlgbcoin128.png` to `/usr/share/pixmaps`

mlgbcoin-qt.protocol (KDE)


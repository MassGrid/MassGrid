MLGBcoin Core 0.10.4
=====================

Setup
---------------------
[MLGBcoin Core](http://mlgbcoin.org/en/download) is the original MLGBcoin client and it builds the backbone of the network. However, it downloads and stores the entire history of MLGBcoin transactions (which is currently several GBs); depending on the speed of your computer and network connection, the synchronization process can take anywhere from a few hours to a day or more. Thankfully you only have to do this once. If you would like the process to go faster you can [download the blockchain directly](bootstrap.md).

Running
---------------------
The following are some helpful notes on how to run MLGBcoin on your native platform.

### Unix

You need the Qt4 run-time libraries to run MLGBcoin-Qt. On Debian or Ubuntu:

	sudo apt-get install libqtgui4

Unpack the files into a directory and run:

- bin/32/mlgbcoin-qt (GUI, 32-bit) or bin/32/mlgbcoind (headless, 32-bit)
- bin/64/mlgbcoin-qt (GUI, 64-bit) or bin/64/mlgbcoind (headless, 64-bit)



### Windows

Unpack the files into a directory, and then run mlgbcoin-qt.exe.

### OS X

Drag MLGBcoin-Qt to your applications folder, and then run MLGBcoin-Qt.

### Need Help?

* See the documentation at the [MLGBcoin Wiki](https://en.mlgbcoin.it/wiki/Main_Page)
for help and more information.
* Ask for help on [#mlgbcoin](http://webchat.freenode.net?channels=mlgbcoin) on Freenode. If you don't have an IRC client use [webchat here](http://webchat.freenode.net?channels=mlgbcoin).
* Ask for help on the [MLGBcoinTalk](https://mlgbcointalk.org/) forums, in the [Technical Support board](https://mlgbcointalk.org/index.php?board=4.0).

Building
---------------------
The following are developer notes on how to build MLGBcoin on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [OS X Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)

Development
---------------------
The MLGBcoin repo's [root README](https://github.com/mlgbcoin/mlgbcoin/blob/master/README.md) contains relevant information on the development process and automated testing.

- [Coding Guidelines](coding.md)
- [Multiwallet Qt Development](multiwallet-qt.md)
- [Release Notes](release-notes.md)
- [Release Process](release-process.md)
- [Source Code Documentation (External Link)](https://dev.visucore.com/mlgbcoin/doxygen/)
- [Translation Process](translation_process.md)
- [Unit Tests](unit-tests.md)

### Resources
* Discuss on the [MLGBcoinTalk](https://mlgbcointalk.org/) forums, in the [Development & Technical Discussion board](https://mlgbcointalk.org/index.php?board=6.0).
* Discuss on [#mlgbcoin-dev](http://webchat.freenode.net/?channels=mlgbcoin) on Freenode. If you don't have an IRC client use [webchat here](http://webchat.freenode.net/?channels=mlgbcoin-dev).

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [Files](files.md)
- [Tor Support](tor.md)
- [Init Scripts (systemd/upstart/openrc)](init.md)

License
---------------------
Distributed under the [MIT/X11 software license](http://www.opensource.org/licenses/mit-license.php).
This product includes software developed by the OpenSSL Project for use in the [OpenSSL Toolkit](https://www.openssl.org/). This product includes
cryptographic software written by Eric Young ([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.

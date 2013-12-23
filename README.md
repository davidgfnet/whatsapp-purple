whatsapp-purple
===============

WhatsApp protocol implementation for libpurple (Pidgin)

Get a copy
----------

Ubuntu users can use the Launchpad repository:
https://launchpad.net/~whatsapp-purple/+archive/ppa/

Windows users can find a copy at the nightly builds server:
http://davidgf.net/nightly/whatsapp-purple/win32/

Building
--------

Just run `make` (if you have to choose between 32 and 64 bit, run `make
ARCH=i686` or `make ARCH=x86_64`). `Makefile.mingw` is the Makefile for 32-bit
Windows.

Binaries
--------

Check http://davidgf.net/nightly/whatsapp-purple/ for nightly builds ;)

For experimental builds (branch `pu`) check
http://davidgf.net/nightly/whatsapp-purple-experimental/

FAQ
---

### How do I get my user name and password?

Your user name is your phone number (including the country code but without any
additional leading zeros, e.g. `4917012345678`), as for the password there are
many ways to get it. You can either sniff it or just ask for a new one. Check
these links here:

* https://github.com/venomous0x/WhatsAPI#faq
* https://github.com/shirioko/MissVenom
* http://blog.philippheckel.com/2013/07/05/how-to-sniff-the-whatsapp-password-from-your-android-phone-or-iphone/

If you want to register a new WhatsApp account, you can use tools like yowsup
or WART:

* https://github.com/tgalal/yowsup
* https://github.com/shirioko/WART

Please, do not contact me by email for this kind of issues, I won't answer your
questions. For developing matters you can open an issue, create a pull request
or (in case you think it's necessary) email me.

### How do I get graphical WhatsApp smileys?

You need to install and enable the Emoji smiley theme. Just copy one of the
subdirectories from following Git repositories to `$HOME/.purple/smileys/` and
enable the newly installed theme in the Pidgin preferences window:

* https://github.com/stv0g/unicode-emoji
* https://github.com/VxJasonxV/emoji-for-pidgin

### How do I get a meaningful backtrace?

In order to get a proper backtrace, you can either use your package manager to
install a debug package or rebuild the library with debug symbols using `make
debug`.

whatsapp-purple
===============

[![Build Status](https://travis-ci.org/davidgfnet/whatsapp-purple.png?branch=master)](https://travis-ci.org/davidgfnet/whatsapp-purple)

WhatsApp protocol implementation for libpurple (Pidgin)

***Important!*** As of April 15th 2016 I won't be working on this plugin anymore. If someone wants to pick up development feel free to email me. There are some other up-to-date APIs that you may want to check (yowsup, ChatAPI, etc).

Get a copy
----------

To get meaningful instructions on how to use this (if you are a user, not a 
developer) please go to https://davidgf.net/whatsapp/ (Instructions for Ubuntu,
Fedora and Windows are provided).

Official binary sources (provided by davidgfnet):

* Ubuntu: https://launchpad.net/~whatsapp-purple/+archive/ubuntu/ppa
* Fedora: https://copr.fedoraproject.org/coprs/davidgf/whatsapp-purple/
* Windows: https://gosell.it/product/whatsapp-for-pidgin-20


Building
--------

Dependencies:
Libc6-dev, libfreeimage-dev

Just run `make` (if you have to choose between 32 and 64 bit, run `make
ARCH=i686` or `make ARCH=x86_64`). `Makefile.mingw` is the Makefile for 32-bit
Windows.

FAQ
---

### I want to be updated on news about this plugin

If you are a telegram user, just join the whatsapp_purple channel (https://telegram.me/whatsapp_purple)

### How do I get my user name and password?

Your user name is your phone number (including the country code but without any
additional leading zeros, e.g. `4917012345678`), as for the password there are
many ways to get it. You can either sniff it or just ask for a new one. Check
these links here:

* https://davidgf.net/whatsapp/pwd.html
* https://github.com/venomous0x/WhatsAPI#faq
* https://github.com/shirioko/MissVenom
* http://blog.philippheckel.com/2013/07/05/how-to-sniff-the-whatsapp-password-from-your-android-phone-or-iphone/

If you want to register a new WhatsApp account, you can use tools like yowsup
or WART:

* https://github.com/tgalal/yowsup
* https://github.com/mgp25/WART

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


*Disclaimer
-----------

WhatsApp is a registered trademark of WhatsApp Inc registered in the U.S. and
other countries. This project is an independent work and has not been authorized,
sponsored, or otherwise approved by Whatsapp Inc. 


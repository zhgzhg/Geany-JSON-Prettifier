JSON Prettifier Plugin for Geany
================================

JSON Prettifier is a plugin used to validate, format and prettify ugly,
not formatted JSON files or to minify a prettified ones.

This repository represents an independent project whose results could
be manually integrated with Geany.

Features:

* Pretty formatting
* Minification formatting
* Full or partial JSON validation (configurable)
* Escaping of forward slashes (configurable)
* Distinguishes separate JSON entities in one file while formatting all
of them (configurable)
* Support for partial formatting limited to the text that is currently
selected
* Indentation format settings (spaces or tabs plus number of symbols)

* Supported platforms: Linux
* License: GPLv2 or later

Dependencies:

* geany, geany-dev(el), geany-plugins-common
* gtk+2.0-dev(el)
* yajl, yajl-dev(el)  --- version(2) --- integrated in this repository

* make
* cmake
* pkg-config

Compilation
-----------

To compile run: `make`

To install (you may need root privileges) run: `make install`

To uninstall (you may need root privileges) run: `make uninstall`

Attention MacOS users - this plugin will work with the manually
installed and compiled Geany editor from source code. It will not work
with the version installed from dmg files.

Other Useful Plugins
--------------------
* [Geany Generic SQL Formatter](https://github.com/zhgzhg/Geany-Generic-SQL-Formatter)
* [Geany Unix Timestamp Converter](https://github.com/zhgzhg/Geany-Unix-Timestamp-Converter)

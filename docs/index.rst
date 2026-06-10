.. SPDX-FileCopyrightText: 2026 Maarten L. Hekkelman
..
.. SPDX-License-Identifier: BSD-2-Clause

Introduction
============

This library attempts to implement the `POSIX.1-2017 <https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap12.html>`_ standard for parsing arguments passed to an application. These arguments are delivered to the main function as the well known argc and argv parameters. This library allows you to parse the contents of these variables and then provides easy access to the information.

The library also contains code to parse configuration files marked up in the well known .ini file format. Support for sections in these files is supported as of version 2.0.

Synopsis
--------

.. include:: ../README.md
  :parser: myst_parser.sphinx_
  :start-after: ## Synopsis
  :end-before: Installation
  :tab-width: 4

Options and Operands
--------------------

The POSIX standard defines two kinds of arguments, the ones whose name start with a hyphen are called *options* whereas the rest is called *operands*. An example is:

.. code-block:: console

	my-tool [-v] [-o option_arg] [operand...]

The option **-o** in the example above has an *option-argument*, the **-v** does not. Operands usually follow the options, but in the case of libmcfp options and operands can be mixed.

configuration files
-------------------

The syntax for configuration files is the usual format of *name* followed by an equals character and then a *value* terminated by an end of line. E.g.:

.. code-block:: ini

	name = value

The *name* here should be the long name of the option, not the short name.

As of version 2.0, mcfp supports more *.ini* file syntax rules. Like comments that can start with either a hash (#) or a semicolon (;) character. And there is support for sections. If your config file contains:

.. code-block:: ini

   # A simple ini file with two sections, this is the first
   [one]
   name = value1

   ; And this is the second
   [second]
   name = value2

Then you can access these values like this:

.. code-block:: cpp

   auto &config = mcfp::config::instance();

   ...

   config.get("one.name"); // returns "value1"

The function :cpp:func:`~mcfp::config::parse_config_file` can be used to parse these files. The first variant of this function is noteworthy, it takes an *option* name and uses its *option-argument* if specified as replacement for the second parameter which holds the default configuration file name. This file is then searched in the list of directories in the third parameter and when found, the file is parsed and the options in the file are appended to the config instance. Options provided on the command line take precedence.

For flag options (ones that do not take an argument on the command line) an argument is required in the config file. This value can be either *true*, *false* or an integral numerical value. So, the equivalent of passing `-vvv` on the command line is `verbose = 3` in a config file if the option was inited with `mcfp::make_option("verbose,v")`.

Use from a library
------------------

Suppose you have a library that would like to have configurable options. This library can specify these options in a section by using the static call `mcfp::config::init_lib`. The options passed to this call will be appended as a section to the global set of options each time they are reset using `mcfp::config::init`.


Installation
------------

To build **mcfp** you will need a very recent compiler. This code has been tested with GCC 14 and CLang 22 as well as the compiler that comes with Visual Studio version 2026.

Other requirements are `CMake <https://cmake.org/>`_ version 3.28 or higher.

.. code-block:: console

   git clone https://forge.hekkelman.net/maarten/mcfp.git
   cd mcfp
   cmake -B build
   cmake --build build
   cmake --install build

.. toctree::
   :maxdepth: 2
   :caption: Contents

   self
   api/library_root.rst
   genindex.rst


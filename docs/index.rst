libmcfp
=======

Introduction
------------

This library attempts to implement the `POSIX.1-2017 <https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap12.html>`_ standard for parsing arguments passed to an application. These arguments are delivered to the main function as the well known argc and argv parameters. This library allows you to parse the contents of these variables and then provides easy access to the information. The library also contains code to parse configuration files 

Options and Operands
--------------------

The POSIX standard defines two kinds of arguments, the ones whose name start with a hyphen are called *options* whereas the rest is called *operands*. An example is:

.. code-block:: sh

	my-tool [-v] [-o option_arg] [operand...]

The option **-o** in the example above has an *option-argument*, the **-v** does not. Operands usually follow the options, but in the case of libmcfp options and operands can be mixed.

configuration files
-------------------

The syntax for configuration files is the usual format of *name* followed by an equals character and then a *value* terminated by an end of line. E.g.:

.. code-block::

	name = value

The function :cpp:func:`mcfp::config::parse_config_file` can be used to parse these files. The first variant of this function is noteworthy, it takes an *option* name and uses its *option-argument* if specified as replacement for the second parameter which holds the default configuration file name. This file is then searched in the list of directories in the third parameter and when found, the file is parsed and the options in the file are appended to the config instance. Options provided on the command line take precedence.

Installation
------------

Use `CMake <https://cmake.org/>`_ to install **libmcfp**.

.. code-block:: sh

   git clone https://github.com/mhekkel/libmcfp.git
   cd libmcfp
   cmake -S . -B build
   cmake --build build
   cmake --install build

Synopsis
--------

.. include:: ../README.md
  :parser: myst_parser.sphinx_
  :start-after: ## Synopsis
  :end-before: Installation
  :tab-width: 4

.. toctree::
   :hidden:
   :maxdepth: 2
   :caption: Contents:

   installation
   api/library_root
   genindex


libmcfp
=======

Introduction
------------

libmcfp is a header only library for C++ that allows you to parse command line arguments and configuration files and then access this information from anywhere in your source code.

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


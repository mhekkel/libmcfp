[![github CI](https://github.com/mhekkel/libmcfp/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/mhekkel/libmcfp/actions)
[![github CI](https://github.com/mhekkel/libmcfp/actions/workflows/build-documentation.yml/badge.svg)](https://github.com/mhekkel/libmcfp/actions)

# libmcfp

A library for parsing command line arguments and configuration files and making them available throughout a program.

There's a config file parser as well.

> **_NOTE:_** The naming of libmcfp has changed again in version 1.3.4 reverting the rename of 1.3.3. To use libmcfp you should use find_package(mcfp) and link to mcfp::mcfp

> **_NOTE:_** This library is now a cpp20 module library instead of a header only library.

## Synopsis

```cpp
// Example of using libmcfp

#include <iomanip>
#include <iostream>
#include <vector>

import mcfp;

int main(int argc, char *const argv[])
{
	// config is a singleton
	auto &config = mcfp::config::instance();

	// Initialise the config object. This can be done more than once,
	// e.g. when you have different sets of options depending on the
	// first operand.

	config.init(
			  // The first parameter is the 'usage' line, used when printing out the options
			  "usage: example [options] file",

			  // Flag options (not taking a parameter)
			  mcfp::make_option("help,h", "Print this help text"),
			  mcfp::make_option("verbose,v", "Verbose level, can be specified more than once to increase level"),

			  // A couple of options with parameter
			  mcfp::make_option<std::string>("config", "Config file to use"),
			  mcfp::make_option<std::string>("text", "The text string to echo"),

			  // And options with a default parameter
			  mcfp::make_option<int>("a", 1, "first parameter for multiplication"),
			  mcfp::make_option<float>("b", 2.0f, "second parameter for multiplication"),

			  // You can also allow multiple values
			  mcfp::make_option<std::vector<std::string>>("c", "Option c, can be specified more than once"),

			  // This option is not shown when printing out the options
			  mcfp::make_hidden_option("d", "Debug mode"))
		.add_section("section-1",
			mcfp::make_option<std::string>("text", "Another text option, now part of section-1"),

			mcfp::make_option<std::string>("an-option-with-a-long-name",
				"Shows that the output of help ends up correctly and wrapped as well if you have a small terminal"));

	// There are two flavors of calls, ones that take an error_code
	// and return the error in that code in case something is wrong.
	// The alternative is calling without an error_code, in which
	// case an exception is thrown when appropriate

	// Parse the command line arguments here

	std::error_code ec;
	config.parse(argc, argv, ec);
	if (ec)
	{
		std::cerr << "Error parsing argument " << std::quoted(config.get_last_option()) << ": " << ec.message() << '\n';
		exit(1);
	}

	// First check, to see if we need to stop early on

	if (config.has("help") or config.operands().size() != 1)
	{
		// Tell user what was wrong
		// This will print out the 'usage' message with all the visible options
		std::cerr << config << '\n';

		if (config.operands().size() != 1)
			std::cerr << "Invalid number of operands, should be exactly one\n\n";

		exit(config.has("help") ? 0 : 1);
	}

	// Configuration files, read it if it exists. If the users
	// specifies an alternative config file, it is an error if that
	// file cannot be found.

	config.parse_config_file("config", "example.conf", { "." }, ec);
	if (ec)
	{
		std::cerr << "Error parsing config file, option " << std::quoted(config.get_last_option()) << ": " << ec.message() << '\n';
		exit(1);
	}

	// If options are specified more than once, you can get the count

	int VERBOSE = config.count("verbose");

	// Operands are arguments that are not options, e.g. files to act upon

	std::cout << "The first operand is " << config.operands().front() << '\n';

	// Getting the value of a string option

	auto text = config.get<std::string>("text", ec);
	if (ec)
	{
		std::cerr << "Error getting option text: " << ec.message() << '\n';
		exit(1);
	}

	std::cout << "Text option is " << std::quoted(text) << '\n';

	// Alternative, using get_optional

	if (auto t1 = config.get_optional("text"))
		std::cout << "Text option still is " << std::quoted(*t1) << '\n';

	// getting values for numeric options

	if (config.has("a") and config.has("b"))
	{
		int a = config.get<int>("a");
		float b = config.get<float>("b");

		std::cout << "a (" << a << ") * b (" << b << ") = " << a * b << '\n';
	}

	// And multiple strings

	for (const std::string& s : config.get<std::vector<std::string>>("c"))
		std::cout << "c: " << s << '\n';

	// Section support

	if (auto t = config.get_optional("section-1.text"); t.has_value())
		std::cout << "Text option for 'section-1' is " << std::quoted(*t) << '\n';

	return 0;
}
```

Running the program without any options, or `--help` results in:

```console
usage: example [options] file

  -h [ --help ]                 Print this help text
  -v [ --verbose ]              Verbose level, can be specified more than
                                once to increase level
  --config arg                  Config file to use
  --text arg                    The text string to echo
  -a arg (=1)                   first parameter for multiplication
  -b arg (=2)                   second parameter for multiplication
  -c arg                        Option c, can be specified more than once

section "section-1"

  --section-1.text arg          Another text option, now part of section-1
  --section-1.an-option-with-a-long-name arg
                                Shows that the output of help ends up correctly and
                                wrapped as well if you have a small terminal
```


## Installation

Use [cmake](https://cmake.org/) to install _libmcfp_. You will also need a very
recent compiler.

```bash
git clone https://github.com/mhekkel/libmcfp.git
cd libmcfp
cmake -B build -G Ninja
cmake --build build
cmake --install build
```

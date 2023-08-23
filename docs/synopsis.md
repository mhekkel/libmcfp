# Synopsis

```c++
// Example of using libmcfp

#include <iostream>
#include <filesystem>

#include <mcfp/mcfp.hpp>

int main(int argc, char * const argv[])
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
        mcfp::make_hidden_option("d", "Debug mode")
    );

    // There are two flavors of calls, ones that take an error_code
    // and return the error in that code in case something is wrong.
    // The alternative is calling without an error_code, in which
    // case an exception is thrown when appropriate

    // Parse the command line arguments here

    std::error_code ec;
    config.parse(argc, argv, ec);
    if (ec)
    {
        std::cerr << "Error parsing arguments: " << ec.message() << std::endl;
        exit(1);
    }

    // First check, to see if we need to stop early on

    if (config.has("help") or config.operands().size() != 1)
    {
        // This will print out the 'usage' message with all the visible options
        std::cerr << config << std::endl;
        exit(config.has("help") ? 0 : 1);
    }

    // Configuration files, read it if it exists. If the users
    // specifies an alternative config file, it is an error if that
    // file cannot be found.

    config.parse_config_file("config", "example.conf", { "." }, ec);
    if (ec)
    {
        std::cerr << "Error parsing config file: " << ec.message() << std::endl;
        exit(1);
    }

    // If options are specified more than once, you can get the count 

    int VERBOSE = config.count("verbose");

    // Operands are arguments that are not options, e.g. files to act upon

    std::cout << "The first operand is " << config.operands().front() << std::endl;

    // Getting the value of a string option

    auto text = config.get<std::string>("text", ec);
    if (ec)
    {
        std::cerr << "Error getting option text: " << ec.message() << std::endl;
        exit(1);
    }

    std::cout << "Text option is " << text << std::endl;

    // Likewise for numeric options

    int a = config.get<int>("a");
    float b = config.get<float>("b");

    std::cout << "a (" << a << ") * b (" << b << ") = " << a * b << std::endl;

    // And multiple strings

    for (std::string s : config.get<std::vector<std::string>>("c"))
        std::cout << "c: " << s << std::endl;

    return 0;
}
```


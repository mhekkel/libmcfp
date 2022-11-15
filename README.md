# libcfp

A library for parsing command line arguments and making them available throughout a program.

There's a config file parser as well.

## Synopsis

```c++
#include <cfp.hpp>

int VERBOSE = 0;

int main(int argc, char * const argv[])
{
    // config is a singleton class
    auto &config = cfp::config::instance();

    // Tell config what options to accept
    // This can be done more than once, replacing
    // the current set of options.
    config.init("usage: test [options] input output",
        cfp::make_option("verbose,v", "Use verbose output"),
        cfp::make_option("help,h", "Show this help"),
        cfp::make_option<std::string>("opt1", "foo", "First option"),
        cfp::make_option<std::string>("config", "Name of a config file to use"),
        cfp::make_option<float>("float-value,f", "Another option")
    );

    // Do the parsing of argv
    config.parse(argc, argv);

    // Set verbose based on number of times the
    // -v or --verbose flag was passed
    VERBOSE = config.count("verbose");

    // Check for the help flag
    if (config.has("help"))
    {
        // print out the options to the screen
        std::cout << config << std::endl;
        exit(0);
    }

    // Optionally read a config file
    // The parameters below specify that the config file is by default called myapp.conf
    // but can be overridden with the --config parameter and the config files should be
    // located in current dictory or /etc
    config.parse_config_file("config", "myapp.conf",
        { fs::current_path().string(), "/etc/" }
    );

    // Operands are parameters that are not options
    // E.g. filenames to process
    if (config.operands().size() != 2)
    {
        std::cerr << config << std::endl;
        exit(1);
    }

    std::filesystem::path input = config.operands().front();
    std::filesystem::path output = config.operands().back();
    
    // Get values from options with a value
    std::option1 = config.get<std::string>("opt1");
    float option2 = 3.14;
    if (config.has("float-value"))
        option2 = config.get<float>("float-value");

    ... etc

}
```

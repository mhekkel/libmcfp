/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Maarten L. Hekkelman
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define CATCH_CONFIG_RUNNER

#if CATCH22
# include <catch2/catch.hpp>
#else
# include <catch2/catch_all.hpp>
#endif

#include <filesystem>

#include <mcfp/mcfp.hpp>

namespace fs = std::filesystem;

std::filesystem::path gTestDir = std::filesystem::current_path();

int main(int argc, char *argv[])
{
	Catch::Session session; // There must be exactly one instance

	// Build a new parser on top of Catch2's
#if CATCH22
	using namespace Catch::clara;
#else
	// Build a new parser on top of Catch2's
	using namespace Catch::Clara;
#endif

	auto cli = session.cli()                                // Get Catch2's command line parser
	           | Opt(gTestDir, "data-dir")                  // bind variable to a new option, with a hint string
	                 ["-D"]["--data-dir"]                   // the option names it will respond to
	           ("The directory containing the data files"); // description string for the help output

	// Now pass the new composite back to Catch2 so it uses that
	session.cli(cli);

	// Let Catch2 (using Clara) parse the command line
	int returnCode = session.applyCommandLine(argc, argv);
	if (returnCode != 0) // Indicates a command line error
		return returnCode;

	return session.run();
}

// --------------------------------------------------------------------

TEST_CASE("t_1, * utf::tolerance(0.001)")
{
	int argc = 3;
	const char *const argv[] = {
		"test", "--flag", nullptr
	};

	auto &config = mcfp::config::instance();

	config.init(
		"test [options]",
		mcfp::make_option("flag", ""),
		mcfp::make_option<int>("param_int", ""),
		mcfp::make_option("param_int_2", 1, ""),
		mcfp::make_option<float>("param_float", ""),
		mcfp::make_option("param_float_2", 3.14f, ""));
	
	config.parse(argc, argv);

	CHECK(config.has("flag"));
	CHECK(not config.has("flag2"));

	CHECK(config.get<int>("param_int_2") == 1);
	CHECK_THROWS_AS(config.get<float>("param_int_2"), std::system_error);
	CHECK_THROWS_AS(config.get<int>("param_int"), std::system_error);

	CHECK(std::to_string(config.get<float>("param_float_2")) == std::to_string(3.14));
	CHECK_THROWS_AS(config.get<int>("param_float_2"), std::system_error);
	CHECK_THROWS_AS(config.get<float>("param_float"), std::system_error);
}

TEST_CASE("t_2")
{
	int argc = 3;
	const char *const argv[] = {
		"test", "-vvvv", "--verbose", nullptr
	};

	auto &config = mcfp::config::instance();

	config.init(
		"test [options]",
		mcfp::make_option("verbose,v", ""));
	
	config.parse(argc, argv);

	CHECK(config.count("verbose") == 5);
}

TEST_CASE("t_3")
{
	int argc = 2;
	const char *const argv[] = {
		"test", "--param_int=42", nullptr
	};

	auto &config = mcfp::config::instance();

	config.init(
		"test [options]",
		mcfp::make_option<int>("param_int", ""));
	
	config.parse(argc, argv);

	CHECK(config.has("param_int"));
	CHECK(config.get<int>("param_int") == 42);
}
TEST_CASE("t_4")
{
	int argc = 3;
	const char *const argv[] = {
		"test", "--param_int", "42", nullptr
	};

	auto &config = mcfp::config::instance();

	config.init(
		"test [options]",
		mcfp::make_option<int>("param_int", ""));
	
	config.parse(argc, argv);

	CHECK(config.has("param_int"));
	CHECK(config.get<int>("param_int") == 42);
}

TEST_CASE("t_5")
{
	const char *const argv[] = {
		"test", "-i", "42", "-j43", nullptr
	};
	int argc = sizeof(argv) / sizeof(char*);
	
	auto &config = mcfp::config::instance();

	config.init(
		"test [options]",
		mcfp::make_option<int>("nr1,i", ""),
		mcfp::make_option<int>("nr2,j", ""));
	
	config.parse(argc, argv);

	CHECK(config.has("nr1"));
	CHECK(config.has("nr2"));

	CHECK(config.get<int>("nr1") == 42);
	CHECK(config.get<int>("nr2") == 43);
}

TEST_CASE("t_6")
{
	const char *const argv[] = {
		"test", "-i", "42", "-j43", "foo", "bar", nullptr
	};
	int argc = sizeof(argv) / sizeof(char*);
	
	auto &config = mcfp::config::instance();

	config.init(
		"test [options]",
		mcfp::make_option<int>("nr1,i", ""),
		mcfp::make_option<int>("nr2,j", ""));
	
	config.parse(argc, argv);

	CHECK(config.has("nr1"));
	CHECK(config.has("nr2"));

	CHECK(config.get<int>("nr1") == 42);
	CHECK(config.get<int>("nr2") == 43);

	CHECK(config.operands().size() == 2);
	CHECK(config.operands().front() == "foo");
	CHECK(config.operands().back() == "bar");
}

TEST_CASE("t_7")
{
	const char *const argv[] = {
		"test", "--", "-i", "42", "-j43", "foo", "bar", nullptr
	};
	int argc = sizeof(argv) / sizeof(char*) - 1;
	
	auto &config = mcfp::config::instance();

	config.init(
		"test [options]",
		mcfp::make_option<int>("nr1,i", ""),
		mcfp::make_option<int>("nr2,j", ""));
	
	config.parse(argc, argv);

	CHECK(not config.has("nr1"));
	CHECK(not config.has("nr2"));

	CHECK(config.operands().size() == 5);

	auto compare = std::vector<std::string>{ argv[2], argv[3], argv[4], argv[5], argv[6] };
	CHECK(config.operands() == compare);
}

TEST_CASE("t_8")
{
	const char *const argv[] = {
		"test", "-i", "foo", "-jbar", nullptr
	};
	int argc = sizeof(argv) / sizeof(char*) - 1;
	
	auto &config = mcfp::config::instance();

	config.init(
		"test [options]",
		mcfp::make_option<const char*>("i", ""),
		mcfp::make_option<std::string_view>("j", ""),
		mcfp::make_option("k", "baz", ""));
	
	config.parse(argc, argv);

	CHECK(config.has("i"));
	CHECK(config.get<std::string>("i") == "foo");
	CHECK(config.has("j"));
	CHECK(config.get<std::string>("j") == "bar");

	CHECK(config.has("k"));
	CHECK(config.get<std::string>("k") == "baz");
}

TEST_CASE("t_9")
{
	auto &config = mcfp::config::instance();

	config.set_usage("usage: test [options]");

	config.init(
		"test [options]",
		mcfp::make_option<const char*>("i", "First option"),
		mcfp::make_option<std::string_view>("j", "This is the second option"),
		mcfp::make_option("a-very-long-option-name,k", "baz", "And, you guessed it, this must be option three."));
	
// 	std::stringstream ss;

// 	int fd = open("/dev/null", O_RDWR);
// 	dup2(fd, STDOUT_FILENO);

// 	ss << config << std::endl;

// 	const char kExpected[] = R"(usage: test [options]
//   -i arg                                First option
//   -j arg                                This is the second option
//   -k [ --a-very-long-option-name ] arg (=baz)
//                                         And, you guessed it, this must be
//                                         option three.

// )";

// 	std::cerr << '>' << kExpected << '<' << std::endl;
// 	std::cerr << '>' << ss.str() << '<' << std::endl;

// 	CHECK_EQUAL(ss.str(), kExpected);
}

TEST_CASE("t_10")
{
	std::string s1 = R"(SPDX-License-Identifier: BSD-2-Clause

Copyright (c) 2022 Maarten L. Hekkelman

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
)";

	mcfp::word_wrapper ww(s1, 80);

	std::ostringstream os;

	for (auto line : ww)
		os << line << std::endl;
	
	CHECK(os.str() == R"(SPDX-License-Identifier: BSD-2-Clause

Copyright (c) 2022 Maarten L. Hekkelman

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this 
list of conditions and the following disclaimer
2. Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation and/
or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

)");
}

TEST_CASE("t_11")
{
	const char *const argv[] = {
		"test", "-faap", "-fnoot", "-fmies", nullptr
	};
	int argc = sizeof(argv) / sizeof(char*) - 1;

	auto &config = mcfp::config::instance();

	config.init(
		"test [options]",
		mcfp::make_option<std::vector<std::string>>("file,f", ""));
	
	config.parse(argc, argv);

	CHECK(config.count("file") == 3);
	
	std::vector<std::string> files = config.get<std::vector<std::string>>("file");
	CHECK(files.size() == 3);
	CHECK(files[0] == "aap");
	CHECK(files[1] == "noot");
	CHECK(files[2] == "mies");
}

TEST_CASE("t_12")
{
	const char *const argv[] = {
		"test", "--aap", nullptr
	};
	int argc = sizeof(argv) / sizeof(char*) - 1;

	auto &config = mcfp::config::instance();

	config.init(
		"test [options]",
		mcfp::make_option<std::vector<std::string>>("file,f", ""));
	
	std::error_code ec;
	config.parse(argc, argv, ec);
	CHECK(ec == mcfp::config_error::unknown_option);

	config.set_ignore_unknown(true);
	ec = {};

	config.parse(argc, argv, ec);
	CHECK(not ec);
}

TEST_CASE("t_13")
{
	const char *const argv[] = {
		"test", "--test=bla", nullptr
	};
	int argc = sizeof(argv) / sizeof(char*) - 1;

	auto &config = mcfp::config::instance();

	config.init(
		"test [options]",
		mcfp::make_option<std::string>("test", ""));
	
	CHECK_NOTHROW(config.parse(argc, argv));

	CHECK(config.has("test"));
	CHECK(config.get("test") == "bla");
}

TEST_CASE("t_14")
{
	const char *const argv[] = {
		"test", "-test=bla", nullptr
	};
	int argc = sizeof(argv) / sizeof(char*) - 1;

	auto &config = mcfp::config::instance();

	config.init(
		"test [options]",
		mcfp::make_option<std::string>("test", ""));
	
	CHECK_THROWS_AS(config.parse(argc, argv), std::system_error);
}

// --------------------------------------------------------------------

TEST_CASE("file_1, * utf::tolerance(0.001)")
{
	const std::string_view config_file{ R"(
# This is a test configuration
aap=1
noot = 2
mies = 	
pi = 3.14
s = hello, world!
verbose
	)" };

	struct membuf : public std::streambuf
	{
		membuf(char * text, size_t length)
		{
			this->setg(text, text, text + length);
		}
	} buffer(const_cast<char *>(config_file.data()), config_file.length());

	std::istream is(&buffer);

	auto &config = mcfp::config::instance();

	config.init(
		"test [options]",
		mcfp::make_option<const char*>("aap", ""),
		mcfp::make_option<int>("noot", ""),
		mcfp::make_option<std::string>("mies", ""),
		mcfp::make_option<float>("pi", ""),
		mcfp::make_option<std::string>("s", ""),
		mcfp::make_option("verbose,v", ""));
	
	std::error_code ec;

	config.parse_config_file(is, ec);

	CHECK(not ec);

	CHECK(config.has("aap"));
	CHECK(config.get<std::string>("aap") == "1");

	CHECK(config.has("noot"));
	CHECK(config.get<int>("noot") == 2);

	CHECK(config.has("pi"));
	CHECK(std::to_string(config.get<float>("pi")) == std::to_string(3.14));

	CHECK(config.has("s"));
	CHECK(config.get<std::string>("s") == "hello, world!");

	CHECK(config.has("verbose"));
}

TEST_CASE("file_2")
{
	auto &config = mcfp::config::instance();

	std::tuple<std::string_view,std::string_view,std::error_code> tests[] = {
		{ "aap !", "aap", make_error_code(mcfp::config_error::invalid_config_file) },
		{ "aap=aap", "aap", {} },
		{ "aap", "aap", make_error_code(mcfp::config_error::missing_argument_for_option) },
		{ "verbose=1", "verbose", make_error_code(mcfp::config_error::option_does_not_accept_argument) },
				
	};

	for (const auto &[config_file, option, err] : tests)
	{
		struct membuf : public std::streambuf
		{
			membuf(char * text, size_t length)
			{
				this->setg(text, text, text + length);
			}
		} buffer(const_cast<char *>(config_file.data()), config_file.length());

		std::istream is(&buffer);

		std::error_code ec;
		config.init(
			"test [options]",
			mcfp::make_option<const char*>("aap", ""),
			mcfp::make_option<int>("noot", ""),
			mcfp::make_option<float>("pi", ""),
			mcfp::make_option<std::string>("s", ""),
			mcfp::make_option("verbose,v", ""));
		
		config.parse_config_file(is, ec);

		CHECK(ec == err);

		if (ec == std::errc())
			CHECK(config.has(option));
	}
}

TEST_CASE("file_3")
{
	auto &config = mcfp::config::instance();

	config.init(
		"test [options]",
		mcfp::make_option<const char*>("aap", ""),
		mcfp::make_option<int>("noot", ""),
		mcfp::make_option<std::string>("config", ""));
	
	std::error_code ec;

	const char *const argv[] = {
		"test", "--aap=aap", "--noot=42", "--config=unit-test.conf", nullptr
	};
	int argc = sizeof(argv) / sizeof(char*) - 1;

	config.parse(argc, argv);

	config.parse_config_file("config", "bla-bla.conf", { gTestDir.string() }, ec);

	CHECK(not ec);

	CHECK(config.has("aap"));
	CHECK(config.get<std::string>("aap") == "aap");

	CHECK(config.has("noot"));
	CHECK(config.get<int>("noot") == 42);
}

	TEST_CASE("file_4")
{
	auto &config = mcfp::config::instance();

	config.init(
		"test [options]",
		mcfp::make_option<const char*>("aap", ""),
		mcfp::make_option<int>("noot", ""),
		mcfp::make_option<std::string>("config", ""));
	
	std::error_code ec;

	const char *const argv[] = {
		"test", "--aap=aap", nullptr
	};
	int argc = sizeof(argv) / sizeof(char*) - 1;

	config.parse(argc, argv);

	config.parse_config_file("config", "unit-test.conf", { gTestDir.string() }, ec);

	CHECK(not ec);

	CHECK(config.has("aap"));
	CHECK(config.get<std::string>("aap") == "aap");

	CHECK(config.has("noot"));
	CHECK(config.get<int>("noot") == 3);
}

// Copyright Maarten L. Hekkelman 2022-2025
//
// SPDX-License-Identifier: BSD-2-Clause

#include "test-main.hpp"

#include <filesystem>
#include <fstream>

#if MCFP_MODULE_MODE
import mcfp;
#else
# include <mcfp/mcfp.hpp>
#endif

// --------------------------------------------------------------------

#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

class testRunListener : public Catch::EventListenerBase
{
  public:
	using Catch::EventListenerBase::EventListenerBase;

	void testRunStarting(Catch::TestRunInfo const & /*testRunInfo*/) override
	{
		mcfp::config::init_lib("ccd",
			mcfp::make_option("download-missing-files", "This option will allow your software to download missing CCD files"));
	}
};

CATCH_REGISTER_LISTENER(testRunListener)

TEST_CASE("suffixed-options")
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

	// --------------------------------------------------------------------

	CHECK_NOTHROW(config.has("config"));
	CHECK_FALSE(config.has("config"));

	// --------------------------------------------------------------------

	std::ostringstream os;
	os.width(72);
	os << config;

	auto test_str = R"(test [options]

  -v [ --verbose ]      

section [ccd]

  --ccd.download-missing-files
                        This option will allow your software to
                        download missing CCD files
)";

	CHECK(os.str() == test_str);
}

TEST_CASE("suffixed-options-2")
{
	auto &config = mcfp::config::instance();

	config.init(
		"test [options]",
		mcfp::make_option("verbose,v", ""));

	int argc = 3;

	const char *const argv[] = {
		"test", nullptr
	};

	config.parse(argc, argv);

	std::error_code ec;
	// This is a test of replacing the default config file too btw
	config.parse_config_file(gTestDir / "lib-test.conf", ec);
	REQUIRE(ec == std::errc{});

	CHECK(config.has("ccd.download-missing-files"));
}
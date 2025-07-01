/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022-2025 Maarten L. hekkelman
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

#include "test-main.hpp"

#include <mcfp/mcfp.hpp>

namespace fs = std::filesystem;

// --------------------------------------------------------------------

MCFP_DEFINE_LIB_OPTIONS(libcifpp,
	mcfp::make_option<std::string>("config", "The libcifpp configuration file"),
	mcfp::make_option("download-missing-ccd-files", "This option will allow your software to download missing CCD files"));

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

  --config arg      The libcifpp configuration file
  --download-missing-ccd-files
                    This option will allow your software to download
                    missing CCD files
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

	fs::path configFile = gTestDir / "lib-test.conf";

	const char *const argv[] = {
		"test", "--config", configFile.c_str(), nullptr
	};

	config.parse(argc, argv);

	REQUIRE(config.has("config"));
	CHECK(config.get("config") == configFile.string());

	std::error_code ec;
    config.parse_config_file("config", "unit-test.conf", { gTestDir.string() });
	REQUIRE(ec == std::errc{});


}
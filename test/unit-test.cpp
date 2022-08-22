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

#define BOOST_TEST_ALTERNATIVE_INIT_API
#include <boost/test/included/unit_test.hpp>

#include <filesystem>

#include <cfg.hpp>

namespace tt = boost::test_tools;
namespace fs = std::filesystem;

fs::path gTestDir = fs::current_path();

// --------------------------------------------------------------------

bool init_unit_test()
{
	// not a test, just initialize test dir
	if (boost::unit_test::framework::master_test_suite().argc == 2)
		gTestDir = boost::unit_test::framework::master_test_suite().argv[1];

	return true;
}

// --------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(t_1)
{
	int argc = 3;
	const char *const argv[] = {
		"test", "--flag", nullptr
	};

	auto &config = cfg::config::instance();

	config.init(
		cfg::make_option("flag"),
		cfg::make_option<int>("param_int"),
		cfg::make_option<int>("param_int_2", 1));
	
	config.parse(argc, argv);

	BOOST_CHECK(config.has("flag"));
	BOOST_CHECK(not config.has("flag2"));

	BOOST_CHECK_EQUAL(config.get<int>("param_int_2"), 1);
	BOOST_CHECK_THROW(config.get<float>("param_int_2"), cfg::invalid_param_type_error);
	BOOST_CHECK_THROW(config.get<int>("param_int"), cfg::no_param_error);
}


BOOST_AUTO_TEST_CASE(t_2)
{
	int argc = 3;
	const char *const argv[] = {
		"test", "-vvvv", "--verbose", nullptr
	};

	auto &config = cfg::config::instance();

	config.init(
		cfg::make_option("verbose,v"));
	
	config.parse(argc, argv);

	BOOST_CHECK_EQUAL(config.count("verbose"), 5);
}

BOOST_AUTO_TEST_CASE(t_3)
{
	int argc = 2;
	const char *const argv[] = {
		"test", "--param_int=42", nullptr
	};

	auto &config = cfg::config::instance();

	config.init(
		cfg::make_option<int>("param_int"));
	
	config.parse(argc, argv);

	BOOST_CHECK(config.has("param_int"));
	BOOST_CHECK_EQUAL(config.get<int>("param_int"), 42);
}

BOOST_AUTO_TEST_CASE(t_4)
{
	int argc = 3;
	const char *const argv[] = {
		"test", "--param_int", "42", nullptr
	};

	auto &config = cfg::config::instance();

	config.init(
		cfg::make_option<int>("param_int"));
	
	config.parse(argc, argv);

	BOOST_CHECK(config.has("param_int"));
	BOOST_CHECK_EQUAL(config.get<int>("param_int"), 42);
}
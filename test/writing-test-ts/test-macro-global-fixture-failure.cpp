//  (C) Copyright Raffi Enficiaud 2016.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// checks issue https://svn.boost.org/trac/boost/ticket/5563

// we need the included version to be able to access the current framework state
// through boost::unit_test::framework::impl::s_frk_state()

#define BOOST_TEST_MODULE test_macro_in_global_fixture
#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include <boost/test/tools/output_test_stream.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/framework.hpp>
#include <boost/test/unit_test_parameters.hpp>

using boost::test_tools::output_test_stream;
using namespace boost::unit_test;


struct GlobalFixture {

    enum ebehaviour {
        be_disabled,
        be_throw,
        be_message,
        be_assert_non_fatal,
        be_assert,
        be_message_dtor,
        be_assert_non_fatal_dtor,
        be_assert_dtor
    };

    GlobalFixture() {
        if(behaviour != be_disabled)
            std::cout << "global fixture" << std::endl;
        switch(behaviour) {

        case be_throw:
            throw std::runtime_error("[FIXTURE] exception in ctor");

        case be_assert_non_fatal:
            BOOST_TEST( false );
            break;

        case be_assert:
            BOOST_FAIL("[FIXTURE] failure in ctor");
            break;

        case be_message:
            BOOST_TEST_MESSAGE("[FIXTURE] message in ctor");
            break;

        case be_disabled:
        default:
            break;
        }
    }

    ~GlobalFixture() {
        if(behaviour != be_disabled)
            std::cout << "global fixture-dtor" << std::endl;

        switch(behaviour) {
        case be_assert_non_fatal_dtor:
            BOOST_TEST( false );
            break;

        case be_assert_dtor:
            BOOST_FAIL("[FIXTURE] failure in dtor"); // this crashes because it raises an uncaught exception in the dtor
            break;

        case be_message_dtor:
            BOOST_TEST_MESSAGE("[FIXTURE] message in dtor");
            break;

        default:
            break;
        }
    }
    static ebehaviour behaviour;
};

GlobalFixture::ebehaviour GlobalFixture::behaviour = GlobalFixture::be_disabled;

BOOST_GLOBAL_FIXTURE( GlobalFixture );

int nb_simple_checks_executed = 0;
void simple_check()
{
    BOOST_TEST_MESSAGE("Executing the test case with current execution count " << nb_simple_checks_executed);
    nb_simple_checks_executed ++;
    BOOST_TEST( true );
}

struct guard_reset_tu_id {
    guard_reset_tu_id(output_test_stream &output_stream, output_format format ) {
        m_tu_id = framework::impl::s_frk_state().m_curr_test_case;
        framework::impl::s_frk_state().m_curr_test_case = INV_TEST_UNIT_ID;
        unit_test_log.set_format( format );
        unit_test_log.set_threshold_level( log_messages );
        unit_test_log.set_stream( output_stream );
    }
    ~guard_reset_tu_id() {
        framework::impl::s_frk_state().m_curr_test_case = m_tu_id;
        unit_test_log.set_format( OF_CLF );
        unit_test_log.set_stream( std::cout ); // important for the next check on the output pattern
    }

private:
    boost::unit_test::test_unit_id m_tu_id;
};

void check( output_test_stream& output, output_format format, std::string global_fixture_behaviour, test_suite* ts)
{
    {
        guard_reset_tu_id tu_guard(output, format);
        ts->p_default_status.value = test_unit::RS_ENABLED;

        output << "\n\n* " << global_fixture_behaviour << "-fixture  *******************************************************************";
        output << std::endl;
        framework::finalize_setup_phase( ts->p_id );
        try {
            framework::run( ts->p_id, false ); // do not continue the test tree to have the test_log_start/end
        }
        catch( framework::setup_error &e) {
            output << "[TEST] setup error: " << e.what();
        }

        output << std::endl;

        BOOST_TEST_MESSAGE("Current test count is: " << nb_simple_checks_executed);
    }
    // guard should end here, right before comparison, and the log stream should not be the output anymore
    BOOST_TEST( output.match_pattern(true) ); // flushes the stream at the end of the comparison.
}

struct guard {
    ~guard()
    {
        unit_test_log.set_format( runtime_config::get<output_format>( runtime_config::LOG_FORMAT ) );
        unit_test_log.set_stream( std::cout );
        GlobalFixture::behaviour = GlobalFixture::be_disabled;
    }
};


// this comes from the same test as log-formatter-test.cpp
class output_test_stream_for_loggers : public output_test_stream {

public:
    explicit output_test_stream_for_loggers(
        boost::unit_test::const_string    pattern_file_name = boost::unit_test::const_string(),
        bool                              match_or_save     = true,
        bool                              text_or_binary    = true )
    : output_test_stream(pattern_file_name, match_or_save, text_or_binary)
    {}

    static std::string normalize_path(const std::string &str) {
        const std::string to_look_for[] = {"\\"};
        const std::string to_replace[]  = {"/"};
        return utils::replace_all_occurrences_of(
                    str,
                    to_look_for, to_look_for + sizeof(to_look_for)/sizeof(to_look_for[0]),
                    to_replace, to_replace + sizeof(to_replace)/sizeof(to_replace[0])
              );
    }

    static std::string get_basename() {
        static std::string basename;

        if(basename.empty()) {
            basename = normalize_path(__FILE__);
            std::string::size_type basename_pos = basename.rfind('/');
            if(basename_pos != std::string::npos) {
                 basename = basename.substr(basename_pos+1);
            }
        }
        return basename;
    }

    virtual std::string get_stream_string_representation() const {
        std::string current_string = output_test_stream::get_stream_string_representation();

        std::string pathname_fixes;
        {
            static const std::string to_look_for[] = {normalize_path(__FILE__)};
            static const std::string to_replace[]  = {"xxx/" + get_basename() };
            pathname_fixes = utils::replace_all_occurrences_of(
                current_string,
                to_look_for, to_look_for + sizeof(to_look_for)/sizeof(to_look_for[0]),
                to_replace, to_replace + sizeof(to_replace)/sizeof(to_replace[0])
            );
        }

        std::string other_vars_fixes;
        {
            static const std::string to_look_for[] = {"time=\"*\"",
                                                      get_basename() + "(*):",
                                                      "unknown location(*):",
                                                      "; testing time: *us\n", // removing this is far more easier than adding a testing time
                                                      "; testing time: *ms\n",
                                                      "<TestingTime>*</TestingTime>",
                                                      };

            static const std::string to_replace[]  = {"time=\"0.1234\"",
                                                      get_basename() + ":*:" ,
                                                      "unknown location:*:",
                                                      "\n",
                                                      "\n",
                                                      "<TestingTime>ZZZ</TestingTime>",
                                                      };

            other_vars_fixes = utils::replace_all_occurrences_with_wildcards(
                pathname_fixes,
                to_look_for, to_look_for + sizeof(to_look_for)/sizeof(to_look_for[0]),
                to_replace, to_replace + sizeof(to_replace)/sizeof(to_replace[0])
            );
        }

        return other_vars_fixes;
    }
};

BOOST_AUTO_TEST_CASE( fixture_check )
{
    guard G;
    ut_detail::ignore_unused_variable_warning( G );

#define PATTERN_FILE_NAME "global-fixture-tests.pattern"

    std::string pattern_file_name(
        framework::master_test_suite().argc == 1
            ? (runtime_config::save_pattern() ? PATTERN_FILE_NAME : "./baseline-outputs/" PATTERN_FILE_NAME )
            : framework::master_test_suite().argv[1] );

    output_test_stream_for_loggers test_output( pattern_file_name, !runtime_config::save_pattern() );

    test_suite* ts_0 = BOOST_TEST_SUITE( "fake root" );
    ts_0->add( BOOST_TEST_CASE( simple_check ) );

    output_format formats_to_check[] = {OF_CLF, OF_XML, OF_JUNIT};

    for(int current_format_index = 0;
        current_format_index < sizeof(formats_to_check)/sizeof(formats_to_check[0]);
        current_format_index ++ )
    {
        nb_simple_checks_executed = 0;
        output_format current_format = formats_to_check[current_format_index];
        GlobalFixture::behaviour = GlobalFixture::be_message;
        check( test_output, current_format, "message", ts_0 );

        GlobalFixture::behaviour = GlobalFixture::be_throw;
        check( test_output, current_format, "exception", ts_0 );

        GlobalFixture::behaviour = GlobalFixture::be_assert;
        check( test_output, current_format, "assertion", ts_0 );

        GlobalFixture::behaviour = GlobalFixture::be_message_dtor;
        check( test_output, current_format, "message-dtor", ts_0 );

        GlobalFixture::behaviour = GlobalFixture::be_assert_dtor;
        check( test_output, current_format, "assertion-dtor", ts_0 );

        GlobalFixture::behaviour = GlobalFixture::be_assert_non_fatal_dtor;
        check( test_output, current_format, "assertion-non-fatal-dtor", ts_0 );

        GlobalFixture::behaviour = GlobalFixture::be_assert_non_fatal;
        check( test_output, current_format, "assertion-non-fatal", ts_0 );
    }
}


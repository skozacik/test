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
        be_assert,
        be_message_dtor,
        be_assert_dtor
    };

    GlobalFixture() {
        if(behaviour != be_disabled)
            std::cout << "global fixture" << std::endl;
        switch(behaviour) {

        case be_throw:
            throw std::runtime_error("exception in ctor");

        case be_assert:
            BOOST_FAIL("failure in ctor");
            break;

        case be_message:
            BOOST_TEST_MESSAGE("message in ctor");
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
        case be_assert_dtor:
            BOOST_FAIL("failure in dtor"); // this crashes because it raises an uncaught exception in the dtor
            break;

        case be_message_dtor:
            BOOST_TEST_MESSAGE("message in dtor");
            break;

        default:
            break;
        }
    }
    static ebehaviour behaviour;
};

GlobalFixture::ebehaviour GlobalFixture::behaviour = GlobalFixture::be_disabled;

BOOST_GLOBAL_FIXTURE( GlobalFixture );

void simple_check()
{
    BOOST_TEST( true );
}

struct guard_reset_tu_id {
    guard_reset_tu_id() {
        m_tu_id = framework::impl::s_frk_state().m_curr_test_case;
        framework::impl::s_frk_state().m_curr_test_case = INV_TEST_UNIT_ID;
    }
    ~guard_reset_tu_id() {
        framework::impl::s_frk_state().m_curr_test_case = m_tu_id;
    }

private:
    boost::unit_test::test_unit_id m_tu_id;
};

void check( output_test_stream& output, std::string global_fixture_behaviour, test_suite* ts)
{
    {
        unit_test_log.set_stream( output );
        unit_test_log.set_threshold_level( log_all_errors );

        guard_reset_tu_id tu_guard;
        ts->p_default_status.value = test_unit::RS_ENABLED;

        output << "* " << global_fixture_behaviour << "-fixture  *******************************************************************";
        output << std::endl;
        framework::finalize_setup_phase( ts->p_id );
        try
        {
            framework::run( ts->p_id, false ); // do not continue the test tree to have the test_log_start/end
        }
        catch( framework::setup_error &e) {
            output << "setup error: " << e.what();
        }

        output << std::endl;

        unit_test_log.set_format(OF_CLF);
        unit_test_log.set_stream(std::cout);

    }
    // guard should end here
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

BOOST_AUTO_TEST_CASE( fixture_check )
{
    guard G;
    ut_detail::ignore_unused_variable_warning( G );

#define PATTERN_FILE_NAME "global-fixture-tests.pattern"

    std::string pattern_file_name(
        framework::master_test_suite().argc == 1
            ? (runtime_config::save_pattern() ? PATTERN_FILE_NAME : "./baseline-outputs/" PATTERN_FILE_NAME )
            : framework::master_test_suite().argv[1] );

    output_test_stream test_output( pattern_file_name, !runtime_config::save_pattern() );

    test_suite* ts_0 = BOOST_TEST_SUITE( "fake root" );
    ts_0->add( BOOST_TEST_CASE( simple_check ) );

    GlobalFixture::behaviour = GlobalFixture::be_message;
    check( test_output, "message", ts_0 );

    GlobalFixture::behaviour = GlobalFixture::be_throw;
    check( test_output, "exception", ts_0 );

    GlobalFixture::behaviour = GlobalFixture::be_assert;
    check( test_output, "assertion", ts_0 );

    GlobalFixture::behaviour = GlobalFixture::be_message_dtor;
    check( test_output, "message-dtor", ts_0 );

    GlobalFixture::behaviour = GlobalFixture::be_assert_dtor;
    check( test_output, "assertion-dtor", ts_0 );
}


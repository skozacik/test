//  (C) Copyright Raffi Enficiaud 2016.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// checks issue https://svn.boost.org/trac/boost/ticket/5563

#define BOOST_TEST_MODULE test_macro_in_global_fixture_with_ctor_assert
#include <boost/test/unit_test.hpp>
#include <iostream>

struct GlobalFixture {
    GlobalFixture() {
        BOOST_FAIL( "this should not crash" );
    }
    ~GlobalFixture() {
        //BOOST_FAIL( "oups" );
    }
};

BOOST_GLOBAL_FIXTURE( GlobalFixture );

BOOST_AUTO_TEST_CASE( some_test )
{
    BOOST_TEST( true );
}

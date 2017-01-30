//  (C) Copyright Raffi Enficiaud 2017.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

#define BOOST_TEST_MODULE message in dataset
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>

namespace data = boost::unit_test::data;

std::string filenames[] = { "util/test_image2.jpg" };
BOOST_DATA_TEST_CASE(test_update,
                     data::make(filenames))
{
    std::string field_name = "Volume";
    int         value = 100;

    BOOST_TEST_MESSAGE("Testing update :");
    BOOST_TEST_MESSAGE("Update " << field_name << " with " << value);
}

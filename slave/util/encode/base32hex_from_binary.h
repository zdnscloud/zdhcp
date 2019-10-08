#pragma once

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// base32hex_from_binary.h (derived from boost base64_from_binary.hpp)

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <cassert>

#include <cstddef> // size_t
#include <boost/config.hpp> // for BOOST_DEDUCED_TYPENAME
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::size_t; 
} // namespace std
#endif

// We use the same boost header files used in "base64_from_".  Since the
// precise path to these headers may vary depending on the boost version we
// simply include the base64 header here.
#include <boost/archive/iterators/base64_from_binary.hpp>

namespace boost { 
namespace archive {
namespace iterators {

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// convert binary integers to base32hex characters

namespace detail {

template<class CharType>
struct from_5_bit {
    typedef CharType result_type;
    CharType operator()(CharType t) const{
        const char * lookup_table = 
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUV";
        assert(t < 32);
        return (lookup_table[static_cast<size_t>(t)]);
    }
};

} // namespace detail

// note: what we would like to do is
// template<class Base, class CharType = BOOST_DEDUCED_TYPENAME Base::value_type>
//  typedef transform_iterator<
//      from_5_bit<CharType>,
//      transform_width<Base, 5, sizeof(Base::value_type) * 8, CharType>
//  > base32hex_from_binary;
// but C++ won't accept this.  Rather than using a "type generator" and
// using a different syntax, make a derivation which should be equivalent.
//
// Another issue addressed here is that the transform_iterator doesn't have
// a templated constructor.  This makes it incompatible with the dataflow
// ideal.  This is also addressed here.

//template<class Base, class CharType = BOOST_DEDUCED_TYPENAME Base::value_type>
template<
    class Base, 
    class CharType = BOOST_DEDUCED_TYPENAME boost::iterator_value<Base>::type
>
class base32hex_from_binary : 
    public transform_iterator<
        detail::from_5_bit<CharType>,
        Base
    >
{
    friend class boost::iterator_core_access;
    typedef transform_iterator<
        BOOST_DEDUCED_TYPENAME detail::from_5_bit<CharType>,
        Base
    > super_t;

public:
    // make composable by using templated constructor
    template<class T>
    base32hex_from_binary(T start) :
        super_t(
            Base(static_cast<T>(start)),
            detail::from_5_bit<CharType>()
        )
    {}
    // intel 7.1 doesn't like default copy constructor
    base32hex_from_binary(const base32hex_from_binary & rhs) : 
        super_t(
            Base(rhs.base_reference()),
            detail::from_5_bit<CharType>()
        )
    {}
//    base32hex_from_binary(){};
};

}; 
};
};

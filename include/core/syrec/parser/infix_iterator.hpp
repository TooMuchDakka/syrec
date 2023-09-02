#ifndef INFIX_ITERATOR_HPP
#define INFIX_ITERATOR_HPP
#pragma once

#include <iterator>
#include <string>

/*
 * Adapted from https://codereview.stackexchange.com/questions/13176/infix-iterator-code
 * Is also available @ https://en.cppreference.com/w/cpp/experimental/ostream_joiner
 *
 * Refactoring due to deprecation of std::iterator in >= C++17 with help of: https://www.fluentcpp.com/2018/05/08/std-iterator-deprecated/
 */

template<class T, class CharT = char, class Traits = std::char_traits<CharT>>
class InfixIterator {
public:
    using iterator_category = std::output_iterator_tag;
    using value_type        = void;
    using difference_type   = void;
    using pointer           = void;
    using reference         = void;

    typedef CharT                             char_type;
    typedef Traits                            traits_type;
    typedef std::basic_ostream<CharT, Traits> ostream_type;

    InfixIterator(ostream_type& s):
        os(&s) {}

    InfixIterator(ostream_type& s, CharT const* d):
        os(&s),
        real_delim(d) {}

    InfixIterator<T, CharT, Traits>& operator=(T const& item) {
        *os << delimiter << item;
        delimiter = real_delim;
        return *this;
    }

    InfixIterator<T, CharT, Traits>& operator*() {
        return *this;
    }

    InfixIterator<T, CharT, Traits>& operator++() {
        return *this;
    }

    InfixIterator<T, CharT, Traits>& operator++(int) {
        return *this;
    }

private:
    std::basic_ostream<CharT, Traits>* os;
    std::basic_string<CharT>           delimiter;
    std::basic_string<CharT>           real_delim;
};

#endif
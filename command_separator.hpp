/*
 * command_separator.hpp
 *
 *  Created on: Feb 20, 2013
 *      Author: calucian
 */

#ifndef COMMAND_SEPARATOR_HPP_
#define COMMAND_SEPARATOR_HPP_

// Adapted after
// Boost token_functions.hpp  ------------------------------------------------//

// Copyright John R. Bandela 2001.

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/libs/tokenizer/ for documentation.

// Revision History:
// 01 Oct 2004   Joaquin M Lopez Munoz
//      Workaround for a problem with string::assign in msvc-stlport
// 06 Apr 2004   John Bandela
//      Fixed a bug involving using char_delimiter with a true input iterator
// 28 Nov 2003   Robert Zeh and John Bandela
//      Converted into "fast" functions that avoid using += when
//      the supplied iterator isn't an input_iterator; based on
//      some work done at Archelon and a version that was checked into
//      the boost CVS for a short period of time.
// 20 Feb 2002   John Maddock
//      Removed using namespace std declarations and added
//      workaround for BOOST_NO_STDC_NAMESPACE (the library
//      can be safely mixed with regex).
// 06 Feb 2002   Jeremy Siek
//      Added char_separator.
// 02 Feb 2002   Jeremy Siek
//      Removed tabs and a little cleanup.


#include <vector>
#include <stdexcept>
#include <string>
#include <cctype>
#include <algorithm> // for find_if
#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/mpl/if.hpp>
#if !defined(BOOST_NO_CWCTYPE)
#include <cwctype>
#endif

//
// the following must not be macros if we are to prefix them
// with std:: (they shouldn't be macros anyway...)
//
#ifdef ispunct
#  undef ispunct
#endif
#ifdef iswpunct
#  undef iswpunct
#endif
#ifdef isspace
#  undef isspace
#endif
#ifdef iswspace
#  undef iswspace
#endif
//
// fix namespace problems:
//
#ifdef BOOST_NO_STDC_NAMESPACE
namespace std{
 using ::ispunct;
 using ::isspace;
#if !defined(BOOST_NO_CWCTYPE)
 using ::iswpunct;
 using ::iswspace;
#endif
}
#endif

namespace boost{
  //===========================================================================
  // The escaped_list_separator class. Which is a model of TokenizerFunction
  // An escaped list is a super-set of what is commonly known as a comma
  // separated value (csv) list.It is separated into fields by a comma or
  // other character. If the delimiting character is inside quotes, then it is
  // counted as a regular character.To allow for embedded quotes in a field,
  // there can be escape sequences using the \ much like C.
  // The role of the comma, the quotation mark, and the escape
  // character (backslash \), can be assigned to other characters.

  struct command_parse_error : public std::runtime_error{
    command_parse_error(const std::string& what_arg):std::runtime_error(what_arg) { }
  };


// The out of the box GCC 2.95 on cygwin does not have a char_traits class.
// MSVC does not like the following typename
  template <class Char,
    class Traits = BOOST_DEDUCED_TYPENAME std::basic_string<Char>::traits_type >
  class command_separator {

  private:
    typedef std::basic_string<Char,Traits> string_type;
    struct char_eq {
      Char e_;
      char_eq(Char e):e_(e) { }
      bool operator()(Char c) {
        return Traits::eq(e_,c);
      }
    };
    string_type  escape_;
    string_type  c_;
    string_type  quote_;
    bool last_;
    bool sep_;
    bool include_sep_;

    bool is_escape(Char e) {
      char_eq f(e);
      return std::find_if(escape_.begin(),escape_.end(),f)!=escape_.end();
    }
    bool is_c(Char e) {
      char_eq f(e);
      return std::find_if(c_.begin(),c_.end(),f)!=c_.end();
    }
    bool is_quote(Char e) {
      char_eq f(e);
      return std::find_if(quote_.begin(),quote_.end(),f)!=quote_.end();
    }
    template <typename iterator, typename Token>
    void do_escape(iterator& next,iterator end,Token& tok) {
      if (++next == end)
        throw command_parse_error(std::string("cannot end with escape"));
      if (Traits::eq(*next,'n')) {
        tok+='\n';
        return;
      }
      else if (is_quote(*next)) {
        tok+=*next;
        return;
      }
      else if (is_c(*next)) {
        tok+=*next;
        return;
      }
      else if (is_escape(*next)) {
        tok+=*next;
        return;
      }
      else
        throw command_parse_error(std::string("unknown escape sequence"));
    }

    public:

    explicit command_separator(Char  e = '\\',
                                    Char c = ',',Char  q = '\"', bool include_sep = false)
      : escape_(1,e), c_(1,c), quote_(1,q), last_(false), sep_(false), include_sep_(include_sep){ }

    command_separator(string_type e, string_type c, string_type q, bool include_sep)
      : escape_(e), c_(c), quote_(q), last_(false),  sep_(false), include_sep_(include_sep) { }

    void reset() {last_=false;}

    template <typename InputIterator, typename Token>
    bool operator()(InputIterator& next,InputIterator end,Token& tok) {
      bool bInQuote = false;
      tok = Token();

      if (next == end) {
        if (last_) {
          last_ = false;
          return true;
        }
        else
          return false;
      }
      last_ = false;
      for (;next != end;++next) {
        if (is_escape(*next)) {
          do_escape(next,end,tok);
        }
        else if (is_c(*next)) {
          if (!bInQuote) {
        	// If we also output separators, do that
        	if(sep_ && include_sep_){
        		sep_=!sep_;
        		tok+=*next;
        		++next;
        		last_ = true;
        		return true;
        	}
            // If we are not in quote, then we are done
        	if( !include_sep_ ) ++next;
        	else sep_ = true;
            // The last character was a c, that means there is
            // 1 more blank field
            last_ = true;
            return true;
          }
          else tok+=*next;
        }
        else if (is_quote(*next)) {
          bInQuote=!bInQuote;
        }
        else {
          tok += *next;
        }
      }
      return true;
    }
  };


} //namespace boost


#endif /* COMMAND_SEPARATOR_HPP_ */

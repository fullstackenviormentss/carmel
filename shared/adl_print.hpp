/** \file

     several levels of indirection to ensure we can find the free 'o << val' or
     'print(o, val, state)' member in the same namespace as val (or o or state)

     the idea is that impls of these can also be free to call adl::adl_print, or
     if they know what they're doing, the ultimate x << val or print(o, val,
     state) directly.

     further, this gives us a chance to directly print to things besides
     ostreams (e.g. string_buffer, which should be more direct/performant than
     stringstream)

     finally, helper o << printer(val, state) calls print(o, val, state)

     and o << printer(val) forces adl::adl_print(o, v) - which can locate
     defaults for pair, collection, etc.

     note that (pre-c++11) printer objects capture a reference to val and so
     (would have to double check standard) like std::max can't be safely passed
     a computed temporary value. presumably the AdlPrinterMove and PrinterMove
     classes will be selected for temporary values, which is definitely safe (if
     a move ctor exists).
*/

#ifndef ADL_PRINT_GRAEHL_2015_10_29_HPP
#define ADL_PRINT_GRAEHL_2015_10_29_HPP
#pragma once

#include <string>
#include <utility>
#include <iostream>
#include <graehl/shared/type_traits.hpp>
#include <graehl/shared/is_container.hpp>

#ifndef GRAEHL_ADL_PRINTER_MOVE_OVERLOAD
/** (C++11 only): the AdlPrinterMove and PrinterMove classes will be selected
     for temporary values, which is definitely safe (if a move ctor exists).
*/
#define GRAEHL_ADL_PRINTER_MOVE_OVERLOAD 1
#endif

namespace graehl {}

/// several levels of indirection to ensure we can find the free 'o << val' or
/// 'print(o, val, state)' member in the same namespace as val (or o or state)
namespace adl {

template <class O, class V>
inline void adl_print(O& o, V const& val);
template <class O, class V, class S>
void adl_print(O& o, V const& val, S const& s);

template <class V, class Enable = void>
struct Print {
  template <class O>
  static void call(O& o, V const& v) {
    using namespace graehl;
    o << v;
  }
  template <class O, class S>
  static void call(O& o, V const& v, S const& s) {
    using namespace graehl;
    print(o, v, s);
  }
#if 0
  //TODO: maybe. need to test in C++98, msvc. for now use adl_to_string.hpp
  std::string str(V const& v) {
    std::string r;
    call(r, v);
    return r;
  }
  template <class S>
  std::string str(V const& v, S &s) {
    std::string r;
    call(r, v, s);
    return r;
  }
#endif
};

template <>
struct Print<std::string, void> {
  typedef std::string V;
  template <class O>
  static void call(O& o, V const& v) {
    o << v;
  }
  template <class O, class S>
  static void call(O& o, V const& v, S const& s) {
    using namespace graehl;
    print(o, v, s);
  }
};

template <class V>
struct Print<V, typename graehl::enable_if<graehl::is_nonstring_container<V>::value>::type> {
  typedef typename V::value_type W;
  template <class O>
  static void call(O& o, V const& v) {
    bool first = true;
    for (W const& x : v) {
      if (first)
        first = false;
      else
        o << ' ';
      adl::adl_print(o, v);
    }
  }
  template <class O, class S>
  static void call(O& o, V const& v, S const& s) {
    bool first = true;
    for (W const& x : v) {
      if (first)
        first = false;
      else
        o << ' ';
      adl::adl_print(o, v, s);
    }
  }
};

template <class A, class B>
struct Print<std::pair<A, B>, void> {
  static const char sep = '=';
  typedef std::pair<A, B> V;
  template <class O>
  static void call(O& o, V const& p) {
    adl::adl_print(o, p.first);
    o << sep;
    adl::adl_print(o, p.second);
  }
  template <class O, class S>
  static void call(O& o, V const& p, S const& s) {
    adl::adl_print(o, p.first, s);
    o << sep;
    adl::adl_print(o, p.second, s);
  }
};

template <class O, class V>
inline void adl_print(O& o, V const& v) {
  Print<V>::call(o, v);
}
template <class O, class V, class S>
void adl_print(O& o, V const& v, S const& s) {
  Print<V>::call(o, v, s);
}
}

namespace graehl {

/**
   S should be a ref or pointer or lightweight WARNING: v and s are both
   potentially captured by reference, so don't pass a temporary (same issue as
   std::max). should be safe in context of immediately printing rather than
   holding forever
*/
template <class V, class S>
struct Printer {
  V const& v;
  S const& s;
  Printer(V const& v, S const& s) : v(v), s(s) {}
};

/// it's important to return ostream and not the more specific stream.
template <class V, class S>
inline std::ostream& operator<<(std::ostream& out, Printer<V, S> const& x) {
  // must be found by ADL - note: typedefs won't help.
  // that is, if you have a typedef and a shared_ptr, you have to put your print in either ns LW or boost
  ::adl::Print<V>::call(out, x.v, x.s);
  return out;
}

/// for O = e.g. string_builder
template <class O, class V, class S>
typename enable_if<is_container<std::ostream>::value, O>::type& operator<<(O& out, Printer<V, S> const& x) {
  ::adl::Print<V>::call(out, x.v, x.s);
  return out;
}

template <class V, class S>
Printer<V, S> printer(V const& v, S const& s) {
  return Printer<V, S>(v, s);
}

template <class V>
struct AdlPrinter {
  V v;
  AdlPrinter(V v) : v(v) {}
};

/// it's important to return ostream and not the more specific stream.
template <class V>
inline std::ostream& operator<<(std::ostream& out, AdlPrinter<V> const& x) {
  // must be found by ADL - note: typedefs won't help.
  // that is, if you have a typedef and a shared_ptr, you have to put your print in either ns LW or boost
  ::adl::Print<V>::call(out, x.v);
  return out;
}

/// for O = e.g. string_builder
template <class O, class V>
typename enable_if<is_container<std::ostream>::value, O>::type& operator<<(O& out, AdlPrinter<V> const& x) {
  ::adl::Print<V>::call(out, x.v);
  return out;
}

/// warning: captures reference.
template <class V>
AdlPrinter<V> printer(V const& v) {
  return v;
}

#if __cplusplus >= 201103L && GRAEHL_ADL_PRINTER_MOVE_OVERLOAD
template <class V, class S>
struct PrinterMove {
  V v;
  S s;
  PrinterMove(V&& v, S s) : v(std::forward<V>(v)), s(s) {}
};

/// it's important to return ostream and not the more specific stream.
template <class V, class S>
inline std::ostream& operator<<(std::ostream& out, PrinterMove<V, S> const& x) {
  // must be found by ADL - note: typedefs won't help.
  // that is, if you have a typedef and a shared_ptr, you have to put your print in either ns LW or boost
  ::adl::Print<V>::call(out, x.v, x.s);
  return out;
}

/// for O = e.g. string_builder
template <class O, class V, class S>
typename enable_if<is_container<std::ostream>::value, O>::type& operator<<(O& out, PrinterMove<V, S> const& x) {
  ::adl::Print<V>::call(out, x.v, x.s);
  return out;
}

template <class V>
struct AdlPrinterMove {
  V v;
  AdlPrinterMove(V&& v) : v(std::forward<V>(v)) {}
};

template <class V>
AdlPrinterMove<V> printer(V&& v) {
  return AdlPrinterMove<V>(std::forward<V>(v));
}

/// it's important to return ostream and not the more specific stream.
template <class V>
inline std::ostream& operator<<(std::ostream& out, AdlPrinterMove<V> const& x) {
  // must be found by ADL - note: typedefs won't help.
  // that is, if you have a typedef and a shared_ptr, you have to put your print in either ns LW or boost
  ::adl::Print<V>::call(out, x.v);
  return out;
}

/// for O = e.g. string_builder
template <class O, class V>
typename enable_if<is_container<std::ostream>::value, O>::type& operator<<(O& out, AdlPrinterMove<V> const& x) {
  ::adl::Print<V>::call(out, x.v);
  return out;
}


#if 0
// is_lvalue_reference
template <class V>
AdlPrinter<V const&> printer_impl(V&& v, true_type) {
  return AdlPrinter<V const&>(v);
}

template <class V>
AdlPrinterMove<V> printer_impl(V&& v, false_type) {
  return AdlPrinterMove<V>(std::forward<V>(v));
}

// is_lvalue_reference
template <class V, class S>
Printer<V const&, S> printer_impl(V&& v, S const& s, true_type) {
  return Printer<V const&, S>(v, s);
}

template <class V, class S>
PrinterMove<V, S> printer_impl(V&& v, S&& s, false_type) {
  return PrinterMove<V, S>(std::forward<V>(v), s);
}

template <class V>
auto printer(V&& v) -> decltype(printer_impl(std::forward<V>(v), is_lvalue_reference<V>())) {
  return printer_impl(std::forward<V>(v), is_lvalue_reference<V>());
}

template <class V, class S>
auto printer(V&& v, S const& s) -> decltype(printer_impl(std::forward<V>(v), s, is_lvalue_reference<V>())) {
  return printer_impl(std::forward<V>(v), s, is_lvalue_reference<V>());
}
#else

template <class V, class Lvalue = true_type>
struct AdlPrinterType {
  typedef AdlPrinter<V const&> type;
};

template <class V>
struct AdlPrinterType<V, false_type> {
  typedef AdlPrinterMove<V> type;
};

template <class V, class S, class Lvalue = true_type >
struct PrinterType {
  typedef Printer<V const&, S> type;
};

template <class V, class S>
struct PrinterType<V, S, false_type> {
  typedef PrinterMove<V, S> type;
};

template <class V>
typename AdlPrinterType<V, is_lvalue_reference<V>>::type printer(V&& v) {
  return std::forward<V>(v);
}

template <class V, class S>
typename PrinterType<V, S, is_lvalue_reference<V>>::type printer(V&& v, S const& s) {
  return typename PrinterType<V, S, is_lvalue_reference<V>>::type(std::forward<V>(v), s);
}
#endif

#endif


}

#endif

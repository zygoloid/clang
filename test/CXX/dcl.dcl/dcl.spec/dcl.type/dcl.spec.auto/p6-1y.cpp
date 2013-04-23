// RUN: %clang_cc1 -verify -std=c++1y %s

namespace std {
  template<typename T> struct initializer_list {
    const T *p;
    unsigned long n;
    initializer_list(const T *p, unsigned long n);
  };
}

// FIXME: This may not be p6 in C++1y; N3638 isn't very clear whether paragraphs
// were added. It might be p8?

int i;
int &&f();

using Int = int;
using IntLRef = int&;
using IntRRef = int&&;
using InitListInt = std::initializer_list<int>;
using IntPtr = int*;

auto x3a = i;
decltype(auto) x3d = i;
using Int = decltype(x3a);
using Int = decltype(x3d);

auto x4a = (i);
decltype(auto) x4d = (i);
using Int = decltype(x4a);
using IntLRef = decltype(x4d);

auto x5a = f();
decltype(auto) x5d = f();
using Int = decltype(x5a);
using IntRRef = decltype(x5d);

auto x6a = { 1, 2 };
decltype(auto) x6d = { 1, 2 }; // expected-error {{cannot deduce 'decltype(auto)' from initializer list}}
using InitListInt = decltype(x6a);

auto *x7a = &i;
decltype(auto) *x7d = &i; // expected-error {{something}}
using IntPtr = decltype(x7a);

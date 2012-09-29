// RUN: %clang_cc1 -std=c++1y -verify -fsyntax-only %s

auto f(); // expected-note {{previous}}
int f(); // expected-error {{differ only in their return type}}

auto &g();
auto g() -> auto &;

auto h() -> auto *;
auto *h();

struct S {
  // n3386 permits this, but it conflicts with another popular extension.
  operator auto(); // expected-error {{'auto' not allowed here}}
};

auto a();
auto a() { return 0; } // expected-note {{previous}}
using T = decltype(a());
using T = int;

// FIXME:
auto a(); // unexpected-error {{differ only in their return type}}

auto b(bool k) {
  if (k)
    return "hello";
  return "goodbye";
}

auto *ptr_1() {
  return 100; // expected-error {{function 'ptr_1' with return type 'auto *' has incompatible return value of type 'int'}}
}

const auto &ref_1() {
  return 0; // expected-warning {{returning reference to local temporary}}
}

auto init_list_1() {
  return { 1, 2, 3 }; // expected-error {{include <initializer_list>}}
}

using size_t = decltype(sizeof(0));

namespace std {
  template<typename T> struct initializer_list {
    T *p;
    decltype(sizeof(0)) n;
    initializer_list(T *p, size_t n) : p(p), n(n) {}
  };
}

auto init_list_2() {
  return { 1, 2, 3 };
}

auto init_list_3() {
  return { 1, 2, 3.0 }; // expected-error {{cannot deduce return type for function 'init_list_3' with specified return type 'auto' from initializer list}}
}

auto fac(int n) {
  if (n <= 2)
    return n;
  return n * fac(n-1); // ok
}

#if 0
auto fac_2(int n) {
  if (n > 2)
    return n * fac_2(n-1); // expected-error {{cannot be used before it is defined}}
  return n;
}
#endif

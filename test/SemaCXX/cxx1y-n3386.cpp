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
auto a() { return 0; }
using T = decltype(a());
using T = int;
auto a(); // expected-note {{previous}}
using T = decltype(a());
auto *a(); // expected-error {{differ only in their return type}}

auto b(bool k) {
  if (k)
    return "hello";
  return "goodbye";
}

auto *ptr_1() {
  return 100; // expected-error {{cannot deduce return type 'auto *' from returned value of type 'int'}}
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
  // FIXME: Explain why we can't deduce std::initializer_list<...> here.
  return { 1, 2, 3.0 }; // expected-error {{cannot deduce return type 'auto' from initializer list}}
}

auto fwd_decl(); // expected-note 2{{here}}

int n = fwd_decl(); // expected-error {{function 'fwd_decl' with deduced return type cannot be used before it is defined}}
int k = sizeof(fwd_decl()); // expected-error {{used before it is defined}}

auto fac(int n) {
  if (n <= 2)
    return n;
  return n * fac(n-1); // ok
}

auto fac_2(int n) { // expected-note {{declared here}}
  if (n > 2)
    return n * fac_2(n-1); // expected-error {{cannot be used before it is defined}}
  return n;
}

auto void_ret() {}
using Void = void;
using Void = decltype(void_ret());

auto &void_ret_2() {} // expected-error {{cannot deduce return type 'auto &' for function with no return statements}}
const auto void_ret_3() {} // ok, return type 'const void' is adjusted to 'void'

const auto void_ret_4() {
  if (false)
    return void();
  if (false)
    return;
  return 0; // expected-error {{'auto' in return type deduced as 'int' here but deduced as 'void' in earlier return statement}}
}

namespace Templates {
  template<typename T> auto f1() {
    return T() + 1;
  }
  template<typename T> auto &f2(T &&v) { return v; }
  int a = f1<int>();
  const int &b = f2(0);
  double d;
  float &c = f2(0.0); // expected-error {{non-const lvalue reference to type 'float' cannot bind to a value of unrelated type 'double'}}

  template<typename T> auto fwd_decl(); // expected-note {{declared here}}
  int e = fwd_decl<int>(); // expected-error {{cannot be used before it is defined}}
  template<typename T> auto fwd_decl() { return 0; }
  int f = fwd_decl<int>();
  template<typename T> auto fwd_decl(); // expected-note {{declared here}}
  int g = fwd_decl<char>();

  auto (*p)() = f1; // expected-error {{incompatible initializer}}
  auto (*q)() = f1<int>; // ok

  typedef decltype(f2(1.2)) dbl; // expected-note {{previous}}
  typedef float dbl; // expected-error {{typedef redefinition with different types ('float' vs 'decltype(f2(1.2))' (aka 'double &'))}}

  extern template auto fwd_decl<double>();
  int k1 = fwd_decl<double>(); // expected-error {{cannot be used before it is defined}}
  // FIXME: what is the right behavior here?
  extern template int fwd_decl<char>();
  int k2 = fwd_decl<char>();
}

auto fwd_decl_using();
namespace N { using ::fwd_decl_using; }
auto fwd_decl_using() { return 0; }
namespace N { int k = N::fwd_decl_using(); }

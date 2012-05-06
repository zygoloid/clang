// RUN: %clang_cc1 -std=c++11 -verify -Wno-pedantic-constexpr %s -fcxx-exceptions

// Test for various constexpr extensions we support.

// Non-literal types can have constexpr members.
struct S {
  S();

  int arr[10];

  // FIXME: There should be a way to write a constexpr but non-const
  //        member function. Perhaps:
  // constexpr int &get(int n) mutable { return arr[n]; }
  constexpr const int &get(int n) { return arr[n]; }
};

const S s = S();
static_assert(&s.get(4) - &s.get(2) == 2, "");

// Compound-statements can be used in constexpr functions.
constexpr int e() {{{{}} return 5; }}
static_assert(e() == 5, "");

// Types can be defined in constexpr functions.
constexpr int f() {
  enum E { e1, e2, e3 };

  struct S {
    constexpr S(E e) : e(e) {}
    constexpr int get() { return e; }
    E e;
  };

  return S(e2).get();
}
static_assert(f() == 1, "");

// Variables can be declared in constexpr functions.
constexpr int g(int k) {
  static const int n = 9;
  int k2 = k * k;
  int k3 = k2 * k;
  return 3 * k3 + 5 * k2 + n * k - 20;
}
static_assert(g(2) == 42, "");
constexpr int h() {
  static const int m = 1 / 0; // expected-warning {{division by zero}} expected-error {{static variable in constexpr function must be initialized by a constant expression}}
  return 0;
}
constexpr int i(int n) {
  static const int m = n; // expected-error {{static variable in constexpr function must be initialized by a constant expression}}
  return m;
}

constexpr int *static_var() {
  static int k = 52;
  return &k;
}
static_assert(static_var() == static_var(), "");

// if-statements can be used in constexpr functions.
constexpr int j(int k) {
  if (k == 5)
    return 1;
  if (k == 1)
    return 5;
  else {
    if (int n = 2 * k - 4) {
      return n + 1;
      return 2;
    }
  }
} // expected-note 2{{control reached end of constexpr function}}
static_assert(j(0) == -3, "");
static_assert(j(1) == 5, "");
static_assert(j(2), ""); // expected-error {{constant expression}} expected-note {{in call to 'j(2)'}}
static_assert(j(3) == 3, "");
static_assert(j(4) == 5, "");
static_assert(j(5) == 1, "");

// ... but we still require at least one return statement.
constexpr int k() { // expected-error {{no return statement in constexpr function}}
  if (k()) {}
}

// We evaluate the body of a constexpr constructor, to check for side-effects.
struct U {
  constexpr U(int n) {
    if (j(n)) {} // expected-note {{in call to 'j(2)'}}
  }
};
constexpr U u1{1};
constexpr U u2{2}; // expected-error {{constant expression}} expected-note {{in call to 'U(2)'}}

// We allow expression-statements.
constexpr int l(bool b) {
  if (b)
    throw "invalid value for b!"; // expected-note {{subexpression not valid}}
  return 5;
}
static_assert(l(false) == 5, "");
static_assert(l(true), ""); // expected-error {{constant expression}} expected-note {{in call to 'l(true)'}}

// Potential constant expression checking is still applied in the presence of
// the extensions.
constexpr int htonl(int x) { // expected-error {{never produces a constant expression}}
  typedef unsigned char uchar;
  uchar arr[4] = { uchar(x >> 24), uchar(x >> 16), uchar(x >> 8), uchar(x) };
  return *reinterpret_cast<int*>(arr); // expected-note {{reinterpret_cast is not allowed in a constant expression}}
}

constexpr int maybe_htonl(bool isBigEndian, int x) {
  if (isBigEndian)
    return x;

  typedef unsigned char uchar;
  uchar arr[4] = { uchar(x >> 24), uchar(x >> 16), uchar(x >> 8), uchar(x) };
  return *reinterpret_cast<int*>(arr); // expected-note {{reinterpret_cast is not allowed in a constant expression}}
}

constexpr int swapped = maybe_htonl(false, 123); // expected-error {{constant expression}} expected-note {{in call}}

namespace NS {
  constexpr int n = 0;
}
constexpr int namespace_alias() {
  namespace N = NS;
  return N::n;
}

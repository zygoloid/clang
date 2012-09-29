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

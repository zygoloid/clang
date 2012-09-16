// RUN: %clang_cc1 -std=c++11 %s -verify

namespace Test1 {
  int f(int, int, int);
  template<int...Ns> void g() {
    int k = f(...3 + Ns ...);
  };

  void h() {
    g<9, 2, 4>();
  }
}

namespace Test2 {
  constexpr int g(int a, int b) { return a * 5 + b; }
  template<bool...B> constexpr int f() {
    return g( (B ? ...2 : 2 + ...2) ...);
  }
  static_assert(f<false, true>() == 11, "");
}

namespace std {
  template<int N, typename T> struct tuple_elem {
    T value;
  };
  template<typename...Ts> struct tuple
      : tuple_elem<...3/*sizeof...(Ts)*/, Ts>... {
    template<typename...Us> constexpr tuple(Us &&...us)
        : tuple_elem<...3, Ts>{us}... {}
  };
  template<int N, typename T> constexpr T get(const tuple_elem<N, T> &e) {
    return e.value;
  }
  template<typename T> struct remove_reference { typedef T type; };
  template<typename T> struct remove_reference<T&> { typedef T type; };
  template<typename...Ts>
  constexpr tuple<typename remove_reference<Ts>::type...>
  make_tuple(Ts &&...ts) {
    return { ts... };
  };
}

namespace Test3 {
  using std::get;

  constexpr std::tuple<int, char, long> t(42, 'x', 3000l);

  constexpr bool f(int a, char b, long c) {
    return a == 42 && b == 'x' && c == 3000l;
  }

  template<typename T> constexpr bool g() {
    return f(get<...3>(t) ...);
  }

  constexpr bool h() {
    return f(get<...3>(t) ...);
  }

  static_assert(g<void>(), "");
  static_assert(h(), "");
}

namespace Test4 {
  using std::get;

  constexpr int g(int a, int b, int c) { return a * 100 + b * 10 + c; }
#define indexof ...sizeof
  template<typename...Ts> constexpr int f(Ts &&...ts) {
    return g(ts + indexof...(Ts) ...);
  }
  static_assert(f(4, 1, 7) == 429, "");

  template<typename...Ts>
  constexpr auto diag(Ts &&...ts) ->
      decltype(std::make_tuple(get<indexof...(Ts)>(ts)...)) {
    return std::make_tuple(get<indexof...(Ts)>(ts)...);
  }
  static_assert(g(get<...3>(diag(std::make_tuple(1, 2, 3),
                                 std::make_tuple(4, 5, 6),
                                 std::make_tuple(7, 8, 9))) ...) == 159, "");
}

namespace Test5 {
  template<typename...Ts> int g(Ts...);
  template<int N, typename...Ts> int f(Ts ...ts) {
    g(ts + ...N + ... 3 ...); // \
    // expected-error {{packs that have different lengths (4 vs. 3)}} \
    // expected-error {{packs that have different lengths (2 vs. 3)}} \
    // expected-error {{packs that have different lengths (2 vs. 3)}} \
    // expected-error {{pack that has a different length (3 vs. 2) from outer parameter packs}} \
    // expected-error {{pack that has a different length (3 vs. 4) from outer parameter packs}}

    return 0;
  };
  int test1 = f<0>(... 1 + ... 2 ...); // expected-error {{1 vs. 2}}
  int test2 = f<2>(... 4 ...); // expected-note {{here}}
  int test3 = f<3>(... 2 ...); // expected-note {{here}}
  int test4 = f<4>(... 2 ...); // expected-note {{here}}
  int test5 = f<2>(... 3 ...); // expected-note {{here}}
  int test6 = f<4>(... 3 ...); // expected-note {{here}}
  int test7 = f<3>(... 3 ...);
}

namespace Test6 {
  int f(int, int);
  int k = f(...(... 2)...); // expected-error {{contains an unexpanded parameter pack}}
}

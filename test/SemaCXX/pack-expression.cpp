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

namespace Test3 {
  template<int N, typename T> struct tuple_elem {
    T value;
  };
  template<typename...Ts> struct tuple : tuple_elem<...3/*sizeof...(Ts)*/, Ts>... {
    template<typename...Us> constexpr tuple(Us &&...us) : tuple_elem<...3, Ts>{us}... {}
  };
  template<int N, typename T> constexpr T get(const tuple_elem<N, T> &e) { return e.value; }

  constexpr tuple<int, char, long> t(42, 'x', 3000l);

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

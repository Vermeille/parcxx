#include <cctype>
#include <iostream>

#include <boost/optional.hpp>

static const class None {
} none{};

template <class T>
class optional {
    T val_;
    bool present_;

   public:
    optional(T&& x) : val_(x), present_(true) {}
    optional(None) : present_(false) {}
    optional() : present_(false) {}

    operator bool() const { return present_; }
    T* operator->() {
        assert(present_);
        return &val_;
    }
    const T* operator->() const {
        assert(present_);
        return &val_;
    }

    T& operator*() {
        assert(present_);
        return val_;
    }
    const T& operator*() const {
        assert(present_);
        return val_;
    }

    template <class F>
    auto then(F&& f) {
        if (present_) {
            return f(val_);
        } else {
            return none;
        }
    }
    template <class F>
    auto fmap(F&& f) -> optional<decltype(f(val_))> {
        if (present_) {
            return optional<decltype(f(val_))>(f(val_));
        } else {
            return none;
        }
    }

    optional& operator=(optional&& o) = default;
    optional(optional&&) = default;
    optional(const optional&) = default;
};

template <class T, class F>
auto operator>>=(const optional<T>& o, F&& f) -> decltype(f(*o)) {
    if (o) {
        return f(*o);
    } else {
        return none;
    }
}

template <class T>
optional<T> make_optional(T&& x) {
    return optional<T>(std::forward<T>(x));
}

template <class T>
using ParserRet = optional<std::pair<T, std::string::const_iterator>>;

typedef std::string::const_iterator str_iterator;

static auto ParserChar(str_iterator begin, str_iterator end) {
    if (begin != end) {
        return make_optional(std::make_pair(*begin, begin + 1));
    }
    return ParserRet<char>();
}

template <class Par, class Pred>
auto parser_pred(Par par, Pred p) {
    return [par, p](str_iterator begin,
                    str_iterator end) -> decltype(par(begin, end)) {
        auto res = par(begin, end);
        if (!res) {
            return none;
        }
        if (p(res->first)) {
            return res;
        }
        return none;
    };
}

template <class T>
using parser_ret_type =
    decltype(std::declval<T>()(str_iterator(), str_iterator())->first);

template <class T, class U>
auto chain(T a, U b) {
    return [=](str_iterator begin, str_iterator end) {
        auto res = a(begin, end);
        if (!res) {
            return ParserRet<std::pair<parser_ret_type<decltype(a)>,
                                       parser_ret_type<decltype(b)>>>();
        }

        auto res2 = b(res->second, end);
        if (!res2) {
            return ParserRet<std::pair<parser_ret_type<decltype(a)>,
                                       parser_ret_type<decltype(b)>>>();
        }

        return make_optional(std::make_pair(
            std::make_pair(res->first, res2->first), res2->second));
    };
}

template <class T, class F>
auto apply(T&& x, F&& f) {
    return [=](str_iterator begin, str_iterator end) {
        return x(begin, end).fmap([=](auto a) {
            return std::make_pair(f(a.first), a.second);
        });
    };
}

auto parse_digit() {
    return apply(parser_pred(ParserChar, isdigit),
                 [](char c) -> int { return c - '0'; });
}

template <class P, class U, class F>
auto parse_while(P p, U u, F f) {
    return [=](str_iterator begin, str_iterator end) -> ParserRet<U> {
        ParserRet<U> ret(std::make_pair(u, begin));
        while (1) {
            auto res = p(ret->second, end);
            if (!res) {
                return ret;
            }
            ret = ParserRet<U>(
                std::make_pair(f(ret->first, res->first), res->second));
        }
    };
}

template <class P1, class P2>
auto pthen(P1 p1, P2 p2) {
    return [=](str_iterator begin, str_iterator end) {
        auto res = p1(begin, end);
        if (res) {
            return p2(res->first)(res->second, end);
        }
        return decltype(p2(res->first)(res->second, end))();
    };
}

template <class P, class U, class F>
auto parse_while1(P p, U u, F f) {
    return pthen(p,
                 [=](const auto& res) { return parse_while(p, f(u, res), f); });
}

auto parse_int() {
    return parse_while1(
        parse_digit(), 0, [](int a, int b) -> int { return a * 10 + b; });
}

template <class P>
void test_parser(P p, const std::string& a) {
    auto res = p(std::begin(a), std::end(a));
    std::cout << a << ": " << (res ? "y " + std::to_string(res->first) : "n")
              << "\n";
}

int main() {
#if 1
    test_parser(parse_digit(), "1aa");
    test_parser(parse_digit(), "12");
    auto int_2p = apply(chain(parse_digit(), parse_digit()),
                        [](auto a) { return a.first * 10 + a.second; });
    test_parser(int_2p, "12");
    test_parser(int_2p, "a2");
    test_parser(parse_int(), "a2");
    test_parser(parse_int(), "666");
    test_parser(parse_int(), "666a");
    test_parser(parse_int(), "a666a");
    test_parser(parse_int(), "a6a66a");
#endif
    return 0;
}

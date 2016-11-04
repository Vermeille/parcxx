#pragma once

#include "optional.h"

template <class T>
using ParserRet = optional<std::pair<T, std::string::const_iterator>>;

typedef std::string::const_iterator str_iterator;

template <class T>
using parser_ret_type =
    decltype(std::declval<T>()(str_iterator(), str_iterator())->first);

class Empty {};

auto parse_char() {
    return [](str_iterator begin, str_iterator end) {
        if (begin != end) {
            return make_optional(std::make_pair(*begin, begin + 1));
        }
        return ParserRet<char>();
    };
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

template <class P, class T>
auto parse_seq(P p, T str) {
    return [=](str_iterator begin, str_iterator end) {
        ParserRet<char> ret(make_optional(std::make_pair('\0', begin)));
        T str2 = str;
        for (; *str2; ++str2) {
            ret = parser_pred(p, [=](char c) { return c == *str2; })(
                ret->second, end);
            if (!ret) {
                return ret;
            }
        }
        return ret;
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

template <class P1, class P2>
auto ptry(P1 p1, P2 p2) {
    return [=](str_iterator begin, str_iterator end) {
        auto res = p1(begin, end);
        if (res) {
            return p2(make_optional(res->first))(res->second, end);
        }
        return p2(optional<decltype(res->first)>())(begin, end);
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

template <class T, class U>
auto chain(T a, U b) {
    return pthen(a, [=](auto r) {
        return apply(b, [=](auto r2) { return std::make_pair(r, r2); });
    });
}

template <class T, class U>
auto skipL(T a, U b) {
    return apply(chain(a, b), [](const auto& x) { return x.second; });
}

template <class T, class U>
auto skipR(T a, U b) {
    return apply(chain(a, b), [](const auto& x) { return x.first; });
}

template <class P, class U, class F>
auto parse_while(P p, U u, F f) {
    return [=](str_iterator begin, str_iterator end) {
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

template <class P>
auto skip_while(P p) {
    return parse_while(p, Empty(), [](auto, auto) { return Empty(); });
}

template <class P, class U, class F>
auto parse_while1(P p, U u, F f) {
    return pthen(p,
                 [=](const auto& res) { return parse_while(p, f(u, res), f); });
}

template <class P>
auto skip_while1(P p) {
    return parse_while1(p, Empty(), [](auto, auto) { return Empty(); });
}

template <class... Ts>
auto parse_try(Ts... p1);

template <class T>
auto parse_try(T p) {
    return p;
}

template <class T, class... Ts>
auto parse_try(T p, Ts... ps) {
    return [=](str_iterator begin, str_iterator end) {
        auto res = p(begin, end);
        if (res) {
            return res;
        }
        return parse_try(ps...)(begin, end);
    };
}

template <class T>
auto parse_nothing(T x) {
    return [=](str_iterator begin, str_iterator end) {
        return make_optional(std::make_pair(x, begin));
    };
}

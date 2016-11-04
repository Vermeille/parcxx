#include <cctype>
#include <functional>
#include <iostream>
#include <iterator>
#include <vector>

#include "combinators.h"

const None none{};

auto parse_digit() {
    return apply(parser_pred(parse_char(), isdigit),
                 [](char c) -> int { return c - '0'; });
}

auto parse_uint() {
    return parse_while1(
        parse_digit(), 0, [](int a, int b) -> int { return a * 10 + b; });
}

auto ignore_whitespaces() {
    return skip_while(parser_pred(parse_char(), isspace));
}

auto parse_int() {
    return ptry(
        parser_pred(parse_char(), [](char c) { return c == '-'; }), [](auto x) {
            return apply(parse_uint(), [=](int i) { return x ? -i : i; });
        });
}

template <class Str>
auto parse_word(Str str) {
    return parse_seq(parse_char(), str);
}

auto parse_word() {
    return parse_while(parser_pred(parse_char(), isalpha),
                       std::string(),
                       [](std::string res, char c) {
                           res.push_back(c);
                           return res;
                       });
}

auto parse_char(char c) {
    return parser_pred(parse_char(), [=](char x) { return x == c; });
}

template <class P>
void expect_false(P p, const std::string& a) {
    std::cout << "expect F " << a << "\n";
    auto res = p(std::begin(a), std::end(a));
    assert(!res);
}

template <class P, class T>
void expect_true(P p, const std::string& a, T x) {
    std::cout << "expect T " << a << "\n";
    auto res = p(std::begin(a), std::end(a));
    assert(res->first == x);
}

template <class P>
auto recursion(P& p) {
    return [&](str_iterator begin, str_iterator end) { return p(begin, end); };
}

int main(int argc, char** argv) {
    expect_true(parse_digit(), "1aa", 1);
    expect_true(parse_digit(), "12", 1);
    auto int_2p = apply(chain(parse_digit(), parse_digit()),
                        [](auto a) { return a.first * 10 + a.second; });
    expect_true(int_2p, "12", 12);
    expect_false(int_2p, "a2");
    expect_false(parse_uint(), "a2");
    expect_true(parse_uint(), "666", 666);
    expect_true(parse_uint(), "666a", 666);
    expect_false(parse_uint(), "a666a");
    expect_true(parse_int(), "666a", 666);
    expect_true(skipL(ignore_whitespaces(), parse_uint()), "   666a", 666);
    expect_true(skipL(ignore_whitespaces(), parse_uint()), "666a", 666);
    expect_true(skipL(ignore_whitespaces(), parse_int()), "-666a", -666);
    expect_true(skipL(ignore_whitespaces(), parse_int()), "-666a", -666);
    expect_true(parse_word("yes"), "yes", 's');
    expect_false(parse_word("yes"), "ayes");
    expect_false(parse_word("yes"), "yea");
    expect_true(skipR(parse_uint(), parse_char('.')), "12.", 12);
    expect_false(skipR(parse_uint(), parse_char('.')), "12");
    expect_false(skipR(parse_uint(), parse_char('.')), "12-");
#if 0
    std::vector<std::string> ws;
    auto csv = apply(skipL(apply(parse_word(),
                                 [&](const std::string& w) {
                                     ws.push_back(w);
                                     return w;
                                 }),
                           parse_while(skipL(parse_char(','), parse_word()),
                                       &ws,
                                       [](auto ws, std::string str) {
                                           ws->push_back(str);
                                           return ws;
                                       })),
                     [](auto) { return Empty(); });
    auto bitep = chain(parse_word("bite"), ignore_whitespaces());
    auto p =
        parse_try(apply(parse_word("exl"),
                        [&](auto x) {
                            ws.push_back("EXL LOL");
                            return Empty();
                        }),
                  skipL(bitep,
                        apply(parse_int(),
                              [&](auto x) {
                                  ws.push_back("BITE=" + std::to_string(x));
                                  return Empty();
                              })),
                  csv);

    // std::string a = "test,caca,lol,bite";
    std::string a = argv[1];
    auto res = p(std::begin(a), std::end(a));
    std::cout << (res ? "y\n" : "n\n");
    std::copy(std::begin(ws),
              std::end(ws),
              std::ostream_iterator<std::string>(std::cout, "\n"));
#endif
    auto intp = parse_int();
    std::function<ParserRet<int>(str_iterator, str_iterator)> expp = parse_try(
        skipR(skipL(parse_char('('), recursion(expp)), parse_char(')')), intp);
    std::string a(argv[1]);
    auto res = expp(std::begin(a), std::end(a));
    std::cout << (res ? "y " + std::to_string(res->first) + "\n" : "n\n");
    return 0;
}

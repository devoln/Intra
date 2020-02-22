This folder contains the main part of Intra library. It tends to include everything that satisfies the following criteria:

1. Highly generic things that are useful everywhere: no domain-specific things
2. OS independent
3. Header-only
4. No dynamic memory allocations
5. No dynamic initialized globals
6. Highly modular: include only what you use
7. Mostly constexpr
8. All other Intra criteria:
   1. no RTTI
   2. no exceptions
   3. Intra headers do not include any platform-specific, third-party or even standard headers by default
   4. Requires at least C++14 support
   5. No compiler flag requirements to build except `-std=c++14` or higher on older compilers where it is not default

Specifically it includes:

1. Platform and compiler feature detection
2. Metaprogramming tools: type traits, limits, metafunctions and metafunction classes
3. Concepts for basic types, arrays, ranges and containers
4. Functional programming tools
5. Basic ranges: Span, StringView, FListRange, BListRange, ...
6. Generic range decorators, composition and algorithms
7. Static containers: Tuple, Variant, Optional, Polymorphic, SBitset, SArray
8. Math functions


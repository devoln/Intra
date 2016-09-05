#pragma once

namespace Intra { namespace Op {

template<typename T> T Add(const T& a, const T& b) {return a+b;}
template<typename T> T Sub(const T& a, const T& b) {return a-b;}
template<typename T> T RSub(const T& a, const T& b) {return b-a;}
template<typename T> T Mul(const T& a, const T& b) {return a*b;}
template<typename T> T Div(const T& a, const T& b) {return a/b;}
template<typename T> T RDiv(const T& a, const T& b) {return b/a;}
template<typename T> T Mod(const T& a, const T& b) {return a%b;}
template<typename T> T RMod(const T& a, const T& b) {return b%a;}

template<typename T> T And(const T& a, const T& b) {return a&b;}
template<typename T> T Or(const T& a, const T& b) {return a|b;}
template<typename T> T Xor(const T& a, const T& b) {return b^a;}

template<typename T> T Min(const T& a, const T& b) {return a<b? a: b;}
template<typename T> T Max(const T& a, const T& b) {return a>b? a: b;}

template<typename T> bool Less(const T& a, const T& b) {return a<b;}
template<typename T> bool LEqual(const T& a, const T& b) {return a<=b;}
template<typename T> bool Greater(const T& a, const T& b) {return a>b;}
template<typename T> bool GEqual(const T& a, const T& b) {return a>=b;}
template<typename T> bool Equal(const T& a, const T& b) {return a==b;}
template<typename T> bool NotEqual(const T& a, const T& b) {return a!=b;}

template<typename T> bool IsEven(const T& value) {return (value & 1) == 0;}
template<typename T> bool IsOdd(const T& value) {return (value & 1) != 0;}

}}


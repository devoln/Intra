#pragma once

#define INTRA_DETAIL_SKIP_FIRST_ARG(firstArg, ...) __VA_ARGS__
#define INTRA_SKIP_FIRST_ARG(x, ...) INTRA_DETAIL_SKIP_FIRST_ARG x

#define INTRA_DETAIL_SELECT_FIRST_ARG(firstArg, ...) firstArg
#define INTRA_DETAIL_SELECT_FIRST_ARG_CONVERT(args) INTRA_DETAIL_SELECT_FIRST_ARG args
#define INTRA_SELECT_FIRST_ARG(x) INTRA_DETAIL_SELECT_FIRST_ARG_CONVERT( (x) )

#ifndef INTRA_CONCATENATE_TOKENS
#define INTRA_DETAIL_CONCATENATE_TOKENS(x, y) x ## y
#define INTRA_CONCATENATE_TOKENS(x, y) INTRA_DETAIL_CONCATENATE_TOKENS(x, y)
#endif

#define INTRA_MACRO_ARGUMENT(...) __VA_ARGS__
#define INTRA_MACRO_EXPAND(x) x


///////////////////////////////////////
// Common each macros implementation //
///////////////////////////////////////

#define INTRA_DETAIL_PREPROCESSOR_FUNC_CHOOSER_30(_f1, _f2, _f3, _f4, _f5, _f6, _f7, _f8, _f9, _f10, _f11, _f12, _f13, _f14, _f15, _f16, _f17, _f18, _f19, _f20, _f21, _f22, _f23, _f24, _f25, _f26, _f27, _f28, _f29, _f30, ...) _f30

#define INTRA_DETAIL_PREPROCESSOR_FUNC_RECOMPOSER(argsWithParentheses) \
        INTRA_DETAIL_PREPROCESSOR_FUNC_CHOOSER_30 argsWithParentheses

#define INTRA_DETAIL_PREPROCESSOR_MACRO_CHOOSER(target_, ...) \
        INTRA_DETAIL_PREPROCESSOR_CHOOSE_FROM_ARG_COUNT(target_, target_##_NO_ARG_EXPANDER __VA_ARGS__ ())

#define INTRA_DETAIL_PREPROCESSOR_CHOOSE_FROM_ARG_COUNT(arg_, ...) \
        INTRA_DETAIL_PREPROCESSOR_FUNC_RECOMPOSER((__VA_ARGS__, \
			arg_##_29, arg_##_28, arg_##_27, arg_##_26, \
            arg_##_25, arg_##_24, arg_##_23, arg_##_22, arg_##_21, arg_##_20, arg_##_19, arg_##_18, arg_##_17, \
			arg_##_16, arg_##_15, arg_##_14, arg_##_13, arg_##_12, arg_##_11, arg_##_10, arg_##_9, arg_##_8, \
            arg_##_7, arg_##_6, arg_##_5, arg_##_4, arg_##_3, arg_##_2, arg_##_1, ))


/////////////////////
// MACRO2_FOR_EACH //
/////////////////////

#define INTRA_DETAIL_PREPROCESSOR_ACTION_30(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,\
	a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27,a28,a29,a30) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9) s f(A,a10) s \
            f(A,a11) s f(A,a12) s f(A,a13) s f(A,a14) s f(A,a15) s f(A,a16) s f(A,a17) s f(A,a18) s f(A,a19) s \
			f(A,a20) s f(A,a21) s f(A,a22) s f(A,a23) s f(A,a24) s f(A,a25) s f(A,a26) s f(A,a27) s f(A,a28) s f(A,a29) s f(A,a30)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_29(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,\
	a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27,a28,a29) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9) s f(A,a10) s \
            f(A,a11) s f(A,a12) s f(A,a13) s f(A,a14) s f(A,a15) s f(A,a16) s f(A,a17) s f(A,a18) s f(A,a19) s \
			f(A,a20) s f(A,a21) s f(A,a22) s f(A,a23) s f(A,a24) s f(A,a25) s f(A,a26) s f(A,a27) s f(A,a28) s f(A, a29)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_28(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,\
	a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27,a28) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9) s f(A,a10) s \
            f(A,a11) s f(A,a12) s f(A,a13) s f(A,a14) s f(A,a15) s f(A,a16) s f(A,a17) s f(A,a18) s f(A,a19) s \
			f(A,a20) s f(A,a21) s f(A,a22) s f(A,a23) s f(A,a24) s f(A,a25) s f(A,a26) s f(A,a27) s f(A,a28)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_27(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,\
	a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9) s f(A,a10) s \
            f(A,a11) s f(A,a12) s f(A,a13) s f(A,a14) s f(A,a15) s f(A,a16) s f(A,a17) s f(A,a18) s f(A,a19) s \
			f(A,a20) s f(A,a21) s f(A,a22) s f(A,a23) s f(A,a24) s f(A,a25) s f(A,a26) s f(A, a27)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_26(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,\
	a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9) s f(A,a10) s \
            f(A,a11) s f(A,a12) s f(A,a13) s f(A,a14) s f(A,a15) s f(A,a16) s f(A,a17) s f(A,a18) s f(A,a19) s \
			f(A,a20) s f(A,a21) s f(A,a22) s f(A,a23) s f(A,a24) s f(A,a25) s f(A,a26)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_25(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,\
	a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9) s f(A,a10) s \
            f(A,a11) s f(A,a12) s f(A,a13) s f(A,a14) s f(A,a15) s f(A,a16) s f(A,a17) s f(A,a18) s f(A,a19) s \
			f(A,a20) s f(A,a21) s f(A,a22) s f(A,a23) s f(A,a24) s f(A,a25)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_24(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,\
	a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9) s f(A,a10) s \
            f(A,a11) s f(A,a12) s f(A,a13) s f(A,a14) s f(A,a15) s f(A,a16) s f(A,a17) s f(A,a18) s f(A,a19) s \
			f(A,a20) s f(A,a21) s f(A,a22) s f(A,a23) s f(A,a24)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_23(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9) s f(A,a10) s \
            f(A,a11) s f(A,a12) s f(A,a13) s f(A,a14) s f(A,a15) s f(A,a16) s f(A,a17) s f(A,a18) s f(A,a19) s \
			f(A,a20) s f(A,a21) s f(A,a22) s f(A,a23)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_22(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9) s f(A,a10) s \
            f(A,a11) s f(A,a12) s f(A,a13) s f(A,a14) s f(A,a15) s f(A,a16) s f(A,a17) s f(A,a18) s f(A,a19) s \
			f(A,a20) s f(A,a21) s f(A,a22)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_21(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9) s f(A,a10) s \
            f(A,a11) s f(A,a12) s f(A,a13) s f(A,a14) s f(A,a15) s f(A,a16) s f(A,a17) s f(A,a18) s f(A,a19) s \
			f(A,a20) s f(A,a21)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_20(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9) s f(A,a10) s \
            f(A,a11) s f(A,a12) s f(A,a13) s f(A,a14) s f(A,a15) s f(A,a16) s f(A,a17) s f(A,a18) s f(A,a19) s f(A,a20)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_19(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9) s f(A,a10) s \
            f(A,a11) s f(A,a12) s f(A,a13) s f(A,a14) s f(A,a15) s f(A,a16) s f(A,a17) s f(A,a18) s f(A,a19)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_18(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9) s f(A,a10) s \
            f(A,a11) s f(A,a12) s f(A,a13) s f(A,a14) s f(A,a15) s f(A,a16) s f(A,a17) s f(A,a18)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_17(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9) s f(A,a10) s \
            f(A,a11) s f(A,a12) s f(A,a13) s f(A,a14) s f(A,a15) s f(A,a16) s f(A,a17)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_16(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9) s f(A,a10) s f(A,a11) s f(A,a12) s f(A,a13) s f(A,a14) s f(A,a15) s f(A,a16)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_15(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9) s f(A,a10) s f(A,a11) s f(A,a12) s f(A,a13) s f(A,a14) s f(A,a15)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_14(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9) s f(A,a10) s f(A,a11) s f(A,a12) s f(A,a13) s f(A,a14)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_13(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9) s f(A,a10) s f(A,a11) s f(A,a12) s f(A,a13)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_12(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9) s f(A,a10) s f(A,a11) s f(A,a12)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_11(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9) s f(A,a10) s f(A,a11)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_10(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9) s f(A,a10)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_9(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8,a9) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8) s f(A,a9)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_8(s,f,A,a1,a2,a3,a4,a5,a6,a7,a8) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7) s f(A,a8)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_7(s,f,A,a1,a2,a3,a4,a5,a6,a7) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6) s f(A,a7)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_6(s,f,A,a1,a2,a3,a4,a5,a6) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5) s f(A,a6)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_5(s,f,A,a1,a2,a3,a4,a5) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4) s f(A,a5)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_4(s,f,A,a1,a2,a3,a4) \
        f(A,a1) s f(A,a2) s f(A,a3) s f(A,a4)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_3(s,f,A,a1,a2,a3) \
        f(A,a1) s f(A,a2) s f(A,a3)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_2(s,f,A,a1,a2) \
        f(A,a1) s f(A,a2)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_1(s,f,A, a1) \
        f(A,a1)

#define INTRA_DETAIL_PREPROCESSOR_ACTION_0(s,f,A)   \
        error_0_argument_not_support

#define INTRA_DETAIL_PREPROCESSOR_ACTION_NO_ARG_EXPANDER()\
        ,,,,,,,INTRA_DETAIL_PREPROCESSOR_ACTION_0

#define INTRA_MACRO2_FOR_EACH(s, f, A, ...)\
        INTRA_DETAIL_PREPROCESSOR_MACRO_CHOOSER( INTRA_DETAIL_PREPROCESSOR_ACTION, __VA_ARGS__)(INTRA_MACRO_ARGUMENT s, f, A, __VA_ARGS__)


///////////////////////////
// MACRO2_FOR_EACH_INDEX //
///////////////////////////

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_30(s,f, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,\
	a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27,a28,a29,a30) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s \
			f(8,a9) s f(9,a10) s f(10,a11) s f(11,a12) s f(12,a13) s f(13,a14) s f(14,a15) s \
			f(15,a16) s f(16,a17) s f(17,a18) s f(18,a19) s f(19,a20) s f(20,a21) s f(21,a22) s \
			f(22,a23) s f(23,a24) s f(24,a25) s f(25,a26) s f(26,a27) s f(27,a28) s f(28,a29) s f(29,a30)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_29(s,f, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,\
	a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27,a28,a29) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s \
			f(8,a9) s f(9,a10) s f(10,a11) s f(11,a12) s f(12,a13) s f(13,a14) s f(14,a15) s \
			f(15,a16) s f(16,a17) s f(17,a18) s f(18,a19) s f(19,a20) s f(20,a21) s f(21,a22) s \
			f(22,a23) s f(23,a24) s f(24,a25) s f(25,a26) s f(26,a27) s f(27,a28) s f(28,a29)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_28(s,f, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,\
	a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27,a28) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s \
			f(8,a9) s f(9,a10) s f(10,a11) s f(11,a12) s f(12,a13) s f(13,a14) s f(14,a15) s \
			f(15,a16) s f(16,a17) s f(17,a18) s f(18,a19) s f(19,a20) s f(20,a21) s f(21,a22) s \
			f(22,a23) s f(23,a24) s f(24,a25) s f(25,a26) s f(26,a27) s f(27,a28)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_27(s,f, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,\
	a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s \
			f(8,a9) s f(9,a10) s f(10,a11) s f(11,a12) s f(12,a13) s f(13,a14) s f(14,a15) s \
			f(15,a16) s f(16,a17) s f(17,a18) s f(18,a19) s f(19,a20) s f(20,a21) s f(21,a22) s \
			f(22,a23) s f(23,a24) s f(24,a25) s f(25,a26) s f(26,a27)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_26(s,f, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,\
	a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s \
			f(8,a9) s f(9,a10) s f(10,a11) s f(11,a12) s f(12,a13) s f(13,a14) s f(14,a15) s \
			f(15,a16) s f(16,a17) s f(17,a18) s f(18,a19) s f(19,a20) s f(20,a21) s f(21,a22) s \
			f(22,a23) s f(23,a24) s f(24,a25) s f(25,a26)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_25(s,f, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,\
	a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s \
			f(8,a9) s f(9,a10) s f(10,a11) s f(11,a12) s f(12,a13) s f(13,a14) s f(14,a15) s \
			f(15,a16) s f(16,a17) s f(17,a18) s f(18,a19) s f(19,a20) s f(20,a21) s f(21,a22) s \
			f(22,a23) s f(23,a24) s f(24,a25)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_24(s,f, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,\
	a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s \
			f(8,a9) s f(9,a10) s f(10,a11) s f(11,a12) s f(12,a13) s f(13,a14) s f(14,a15) s \
			f(15,a16) s f(16,a17) s f(17,a18) s f(18,a19) s f(19,a20) s f(20,a21) s f(21,a22) s \
			f(22,a23) s f(23,a24)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_23(s,f, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,\
	a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s \
			f(8,a9) s f(9,a10) s f(10,a11) s f(11,a12) s f(12,a13) s f(13,a14) s f(14,a15) s \
			f(15,a16) s f(16,a17) s f(17,a18) s f(18,a19) s f(19,a20) s f(20,a21) s f(21,a22) s f(22,a23)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_22(s,f, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,\
	a13,a14,a15,a16,a17,a18,a19,a20,a21,a22) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s \
			f(8,a9) s f(9,a10) s f(10,a11) s f(11,a12) s f(12,a13) s f(13,a14) s f(14,a15) s \
			f(15,a16) s f(16,a17) s f(17,a18) s f(18,a19) s f(19,a20) s f(20,a21) s f(21,a22)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_21(s,f, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s \
			f(8,a9) s f(9,a10) s f(10,a11) s f(11,a12) s f(12,a13) s f(13,a14) s f(14,a15) s \
			f(15,a16) s f(16,a17) s f(17,a18) s f(18,a19) s f(19,a20) s f(20,a21)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_20(s,f, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s \
			f(8,a9) s f(9,a10) s f(10,a11) s f(11,a12) s f(12,a13) s f(13,a14) s f(14,a15) s \
			f(15,a16) s f(16,a17) s f(17,a18) s f(18,a19) s f(19,a20)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_19(s,f, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s \
			f(8,a9) s f(9,a10) s f(10,a11) s f(11,a12) s f(12,a13) s f(13,a14) s f(14,a15) s \
			f(15,a16) s f(16,a17) s f(17,a18) s f(18,a19)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_18(s,f, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s \
			f(8,a9) s f(9,a10) s f(10,a11) s f(11,a12) s f(12,a13) s f(13,a14) s f(14,a15) s \
			f(15,a16) s f(16,a17) s f(17,a18)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_17(s,f, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s \
			f(8,a9) s f(9,a10) s f(10,a11) s f(11,a12) s f(12,a13) s f(13,a14) s f(14,a15) s \
			f(15,a16) s f(16,a17)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_16(s,f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s\
			f(8,a9) s f(9,a10) s f(10,a11) s f(11,a12) s f(12,a13) s f(13,a14) s f(14,a15) s f(15,a16)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_15(s,f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s \
			f(8,a9) s f(9,a10) s f(10,a11) s f(11,a12) s f(12,a13) s f(13,a14) s f(14,a15)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_14(s,f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s \
			f(8,a9) s f(9,a10) s f(10,a11) s f(11,a12) s f(12,a13) s f(13,a14)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_13(s,f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s \
			f(8,a9) s f(9,a10) s f(10,a11) s f(11,a12) s f(12,a13)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_12(s,f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s \
			f(8,a9) s f(9,a10) s f(10,a11) s f(11,a12)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_11(s,f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s \
			f(8,a9) s f(9,a10) s f(10,a11)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_10(s,f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s \
			f(8,a9) s f(9,a10)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_9(s,f,a1,a2,a3,a4,a5,a6,a7,a8,a9) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8) s f(8,a9)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_8(s,f,a1,a2,a3,a4,a5,a6,a7,a8) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7) s f(7,a8)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_7(s,f,a1,a2,a3,a4,a5,a6,a7) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6) s f(6,a7)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_6(s,f,a1,a2,a3,a4,a5,a6) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5) s f(5,a6)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_5(s,f,a1,a2,a3,a4,a5) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4) s f(4,a5)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_4(s,f,a1,a2,a3,a4) \
        f(0,a1) s f(1,a2) s f(2,a3) s f(3,a4)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_3(s,f,a1,a2,a3) \
        f(0,a1) s f(1,a2) s f(2,a3)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_2(s,f,a1,a2) \
        f(0,a1) s f(1,a2)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_1(s,f,a1) \
        f(0,a1)

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_0(s,f)   \
        error_0_argument_not_support

#define INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_NO_ARG_EXPANDER()\
        ,,,,,,,,,,,,,,,,,INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION_0

#define INTRA_MACRO2_FOR_EACH_INDEX(s, f, ...)\
        INTRA_DETAIL_PREPROCESSOR_MACRO_CHOOSER( INTRA_DETAIL_PREPROCESSOR_INDEX_ACTION, __VA_ARGS__)(INTRA_MACRO_ARGUMENT s, f, __VA_ARGS__)

#define INTRA_DETAIL_PREPROCESSOR_REPEAT0(X)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT1(X) X
#define INTRA_DETAIL_PREPROCESSOR_REPEAT2(X) INTRA_DETAIL_PREPROCESSOR_REPEAT1(X) X
#define INTRA_DETAIL_PREPROCESSOR_REPEAT3(X) INTRA_DETAIL_PREPROCESSOR_REPEAT2(X) X
#define INTRA_DETAIL_PREPROCESSOR_REPEAT4(X) INTRA_DETAIL_PREPROCESSOR_REPEAT3(X) X
#define INTRA_DETAIL_PREPROCESSOR_REPEAT5(X) INTRA_DETAIL_PREPROCESSOR_REPEAT4(X) X
#define INTRA_DETAIL_PREPROCESSOR_REPEAT6(X) INTRA_DETAIL_PREPROCESSOR_REPEAT5(X) X
#define INTRA_DETAIL_PREPROCESSOR_REPEAT7(X) INTRA_DETAIL_PREPROCESSOR_REPEAT6(X) X
#define INTRA_DETAIL_PREPROCESSOR_REPEAT8(X) INTRA_DETAIL_PREPROCESSOR_REPEAT7(X) X
#define INTRA_DETAIL_PREPROCESSOR_REPEAT9(X) INTRA_DETAIL_PREPROCESSOR_REPEAT8(X) X
#define INTRA_DETAIL_PREPROCESSOR_REPEAT10(X) INTRA_DETAIL_PREPROCESSOR_REPEAT9(X) X


#define INTRA_DETAIL_PREPROCESSOR_REPEAT_0(s,f)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_1(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_0(s,f) s f(0)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_2(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_1(s,f) s f(1)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_3(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_2(s,f) s f(2)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_4(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_3(s,f) s f(3)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_5(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_4(s,f) s f(4)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_6(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_5(s,f) s f(5)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_7(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_6(s,f) s f(6)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_8(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_7(s,f) s f(7)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_9(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_8(s,f) s f(8)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_10(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_9(s,f) s f(9)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_11(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_10(s,f) s f(10)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_12(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_11(s,f) s f(11)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_13(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_12(s,f) s f(12)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_14(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_13(s,f) s f(13)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_15(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_14(s,f) s f(14)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_16(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_15(s,f) s f(15)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_17(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_16(s,f) s f(16)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_18(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_17(s,f) s f(17)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_19(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_18(s,f) s f(18)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_20(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_19(s,f) s f(19)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_21(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_20(s,f) s f(20)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_22(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_21(s,f) s f(21)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_23(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_22(s,f) s f(22)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_24(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_23(s,f) s f(23)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_25(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_24(s,f) s f(24)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_26(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_25(s,f) s f(25)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_27(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_26(s,f) s f(26)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_28(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_27(s,f) s f(27)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_29(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_28(s,f) s f(28)
#define INTRA_DETAIL_PREPROCESSOR_REPEAT_30(s,f) INTRA_DETAIL_PREPROCESSOR_REPEAT_29(s,f) s f(29)

#define INTRA_MACRO_REPEAT(n,f,separator) INTRA_DETAIL_PREPROCESSOR_REPEAT##n(separator,f)

#pragma once

#include "VariadicCommon.h"
#include "Operations.h"

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

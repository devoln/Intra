#pragma once

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

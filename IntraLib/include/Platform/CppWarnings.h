#pragma once

#ifdef _MSC_VER

#define INTRA_WARNING_PUSH __pragma(warning(push))
#define INTRA_WARNING_POP __pragma(warning(pop))

#if _MSC_VER>=1900
#define INTRA_REDUNDANT_WARNINGS_MSVC14 4577
//4577 - чтобы не ругался на noexcept при отключённых исключениях

#define INTRA_WARNING_DISABLE_MOVE_IMPLICITLY_DELETED \
	__pragma(warning(disable : 5027))
#else
#define INTRA_REDUNDANT_WARNINGS_MSVC14
#define INTRA_WARNING_DISABLE_MOVE_IMPLICITLY_DELETED
#endif

//TODO: в будущем исправить это
#pragma warning(disable: 4710)

#define INTRA_DISABLE_REDUNDANT_WARNINGS \
	__pragma(warning(disable : 4714 4640 4514 4820 4711 4710 4061 4868 INTRA_REDUNDANT_WARNINGS_MSVC14))
//4714 - в дебаге не ругаться на то, что forceinline не сработал
//4640 - создание в конструкторе локального статического объекта может привести к ошибкам при работе с потоками
//4514 - подставляемая функция, не используемая в ссылках, была удалена
//4820 - не ругаться на выравнивание
//4711 - не сообщать об автоматическом inline
//4710 - не сообщать об автоматическом не inline
//4061 - не ругаться на необработанные явно case enum'а
//4868 - компилятор не может принудительно применить порядок вычисления "слева направо" для списка инициализаторов, заключенных в фигурные скобки


#define INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED \
	__pragma(warning(disable : 4625 4626 4512))

#define INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED \
	__pragma(warning(disable : 4510 4610 4623))

#define INTRA_PUSH_DISABLE_ALL_WARNINGS __pragma(warning(push, 0))
#define INTRA_WARNING_DISABLE_SIGN_CONVERSION

#elif defined(__GNUC__)
#define INTRA_WARNING_PUSH \
	_Pragma("GCC diagnostic push")
#define INTRA_WARNING_POP \
	_Pragma("GCC diagnostic pop")

#define INTRA_WARNING_DISABLE_SIGN_CONVERSION \
	_Pragma("GCC diagnostic ignored \"-Wsign-conversion\"")

#define INTRA_PUSH_DISABLE_ALL_WARNINGS \
	INTRA_WARNING_PUSH \
	_Pragma("GCC diagnostic ignored \"-Wall\"") \
	_Pragma("GCC diagnostic ignored \"-Wextra\"") \
	_Pragma("GCC diagnostic ignored \"-Wold-style-cast\"") \
	_Pragma("GCC diagnostic ignored \"-Wconversion\"") \
	_Pragma("GCC diagnostic ignored \"-Wsign-conversion\"") \
	_Pragma("GCC diagnostic ignored \"-Winit-self\"") \
	_Pragma("GCC diagnostic ignored \"-Wunreachable-code\"") \
	_Pragma("GCC diagnostic ignored \"-Wpointer-arith\"")

#define INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
#define INTRA_WARNING_DISABLE_MOVE_IMPLICITLY_DELETED
#define INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED
#define INTRA_DISABLE_REDUNDANT_WARNINGS

#elif defined(__clang__)
#define INTRA_WARNING_PUSH \
	_Pragma("clang diagnostic push")
#define INTRA_WARNING_POP \
	_Pragma("clang diagnostic pop")

#define INTRA_WARNING_DISABLE_SIGN_CONVERSION \
	_Pragma("clang diagnostic ignored \"-Wsign-conversion\"")

#define INTRA_PUSH_DISABLE_ALL_WARNINGS \
	INTRA_WARNING_PUSH \
	_Pragma("clang diagnostic ignored \"-Wall\"") \
	_Pragma("clang diagnostic ignored \"-Wextra\"") \
	_Pragma("clang diagnostic ignored \"-Wold-style-cast\"") \
	_Pragma("clang diagnostic ignored \"-Wconversion\"") \
	_Pragma("clang diagnostic ignored \"-Wsign-conversion\"") \
	_Pragma("clang diagnostic ignored \"-Winit-self\"") \
	_Pragma("clang diagnostic ignored \"-Wunreachable-code\"")

#define INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
#define INTRA_WARNING_DISABLE_MOVE_IMPLICITLY_DELETED
#define INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED
#define INTRA_DISABLE_REDUNDANT_WARNINGS


#else

#define INTRA_WARNING_PUSH
#define INTRA_WARNING_DISABLE_SIGN_CONVERSION
#define INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
#define INTRA_WARNING_DISABLE_MOVE_IMPLICITLY_DELETED
#define INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED
#define INTRA_DISABLE_REDUNDANT_WARNINGS
#define INTRA_PUSH_DISABLE_ALL_WARNINGS
#define INTRA_WARNING_POP

#endif

#define INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS \
	INTRA_WARNING_PUSH INTRA_DISABLE_REDUNDANT_WARNINGS

#define INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED \
	INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED \
	INTRA_WARNING_DISABLE_MOVE_IMPLICITLY_DELETED \
	INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED

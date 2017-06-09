#pragma once

#ifdef _MSC_VER

#define INTRA_WARNING_PUSH __pragma(warning(push))
#define INTRA_WARNING_POP __pragma(warning(pop))

#if _MSC_VER >= 1900
#define INTRA_REDUNDANT_WARNINGS_MSVC14 4577 4868
//4577 - чтобы не ругался на noexcept при отключённых исключениях
//4868 - компилятор не может принудительно применить порядок вычисления "слева направо" для списка инициализаторов, заключенных в фигурные скобки

#define INTRA_WARNING_DISABLE_MOVE_IMPLICITLY_DELETED \
	__pragma(warning(disable : 5026 5027))
#else
#define INTRA_REDUNDANT_WARNINGS_MSVC14
#define INTRA_WARNING_DISABLE_MOVE_IMPLICITLY_DELETED
#endif

#define INTRA_WARNING_DISABLE_NO_VIRTUAL_DESTRUCTOR \
	__pragma(warning(disable : 4265))

//TODO: в будущем исправить это
#pragma warning(disable: 4710 4714 4514)

#define INTRA_DISABLE_REDUNDANT_WARNINGS \
	__pragma(warning(disable: 4714 4514 4820 4711 4710 4061 4608 4571 INTRA_REDUNDANT_WARNINGS_MSVC14))
//4714 - в дебаге не ругаться на то, что forceinline не сработал
//4640 - создание в конструкторе локального статического объекта может привести к ошибкам при работе с потоками
//4514 - подставляемая функция, не используемая в ссылках, была удалена
//4820 - не ругаться на выравнивание
//4711 - не сообщать об автоматическом inline
//4710 - не сообщать об автоматическом не inline
//4061 - не ругаться на необработанные явно case enum'а
//4868 - компилятор не может принудительно применить порядок вычисления "слева направо" для списка инициализаторов, заключенных в фигурные скобки


#define INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED \
	__pragma(warning(disable: 4625 4626 4512))

#define INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED \
	__pragma(warning(disable: 4510 4610 4623))

#define INTRA_WARNING_DISABLE_SIGN_CONVERSION __pragma(warning(disable: 4365))
#define INTRA_WARNING_DISABLE_LOSING_CONVERSION __pragma(warning(disable: 4244))
#define INTRA_WARNING_DISABLE_UNREACHABLE_CODE __pragma(warning(disable: 4702))

#define INTRA_WARNING_DISABLE_PEDANTIC

#define INTRA_WARNING_DISABLE_UNUSED_FUNCTION

#define INTRA_PUSH_DISABLE_ALL_WARNINGS __pragma(warning(push, 0)) \
	__pragma(warning(disable: 4548 4987 4774 4702)) \
	INTRA_WARNING_DISABLE_LOSING_CONVERSION \
	INTRA_WARNING_DISABLE_SIGN_CONVERSION \
	INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED

#ifndef __clang__

#ifndef INTRA_CONCATENATE_TOKENS
#define INTRA_DETAIL_CONCATENATE_TOKENS(x, y) x ## y
#define INTRA_CONCATENATE_TOKENS(x, y) INTRA_DETAIL_CONCATENATE_TOKENS(x, y)
#endif

#define INTRA_DISABLE_LNK4221 namespace {char INTRA_CONCATENATE_TOKENS($DisableLNK4221__, __LINE__);}
#else
#define INTRA_DISABLE_LNK4221
#endif

#elif defined(__GNUC__) && !defined(__clang__)
#define INTRA_WARNING_PUSH \
	_Pragma("GCC diagnostic push")
#define INTRA_WARNING_POP \
	_Pragma("GCC diagnostic pop")

#define INTRA_IGNORE_WARNING_HELPER0(x) #x
#define INTRA_IGNORE_WARNING_HELPER1(x) INTRA_IGNORE_WARNING_HELPER0(GCC diagnostic ignored x)
#define INTRA_IGNORE_WARNING_HELPER2(y) INTRA_IGNORE_WARNING_HELPER1(#y)
#define INTRA_IGNORE_WARNING(x) _Pragma(INTRA_IGNORE_WARNING_HELPER2(-W ## x))

#elif defined(__clang__)

#define INTRA_WARNING_PUSH \
	_Pragma("clang diagnostic push")

#define INTRA_WARNING_POP \
	_Pragma("clang diagnostic pop")

#define INTRA_IGNORE_WARNING_HELPER0(x) #x
#define INTRA_IGNORE_WARNING_HELPER1(x) INTRA_IGNORE_WARNING_HELPER0(clang diagnostic ignored x)
#define INTRA_IGNORE_WARNING_HELPER2(y) INTRA_IGNORE_WARNING_HELPER1(#y)
#define INTRA_IGNORE_WARNING(x) _Pragma(INTRA_IGNORE_WARNING_HELPER2(-W ## x))

#else

#define INTRA_WARNING_PUSH
#define INTRA_WARNING_DISABLE_SIGN_CONVERSION
#define INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
#define INTRA_WARNING_DISABLE_MOVE_IMPLICITLY_DELETED
#define INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED
#define INTRA_DISABLE_REDUNDANT_WARNINGS
#define INTRA_PUSH_DISABLE_ALL_WARNINGS
#define INTRA_WARNING_POP
#define INTRA_WARNING_DISABLE_LOSING_CONVERSION
#define INTRA_WARNING_DISABLE_NO_VIRTUAL_DESTRUCTOR
#define INTRA_WARNING_DISABLE_UNUSED_FUNCTION

#define INTRA_DISABLE_LNK4221

#endif

#if (defined(__GNUC__) || defined(__clang__)) && !defined(_MSC_VER)
#define INTRA_PUSH_DISABLE_ALL_WARNINGS \
	INTRA_WARNING_PUSH \
	INTRA_IGNORE_WARNING(all) \
	INTRA_IGNORE_WARNING(extra) \
	INTRA_IGNORE_WARNING(old-style-cast) \
	INTRA_IGNORE_WARNING(conversion) \
	INTRA_IGNORE_WARNING(sign-conversion) \
	INTRA_IGNORE_WARNING(init-self) \
	INTRA_IGNORE_WARNING(unreachable-code) \
	INTRA_IGNORE_WARNING(pointer-arith) \
	INTRA_IGNORE_WARNING(pedantic)

#define INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
#define INTRA_WARNING_DISABLE_MOVE_IMPLICITLY_DELETED
#define INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED
#define INTRA_DISABLE_REDUNDANT_WARNINGS INTRA_IGNORE_WARNING(ctor-dtor-privacy)

#define INTRA_WARNING_DISABLE_NO_VIRTUAL_DESTRUCTOR INTRA_IGNORE_WARNING(non-virtual-dtor)
#define INTRA_WARNING_DISABLE_PEDANTIC INTRA_IGNORE_WARNING(pedantic)

#define INTRA_WARNING_DISABLE_LOSING_CONVERSION INTRA_IGNORE_WARNING(conversion)
#define INTRA_WARNING_DISABLE_SIGN_CONVERSION INTRA_IGNORE_WARNING(sign-conversion)
#define INTRA_WARNING_DISABLE_UNREACHABLE_CODE INTRA_IGNORE_WARNING(unreachable-code)
#define INTRA_WARNING_DISABLE_UNUSED_FUNCTION INTRA_IGNORE_WARNING(unused-function)

#define INTRA_DISABLE_LNK4221
#endif

#define INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS \
	INTRA_WARNING_PUSH INTRA_DISABLE_REDUNDANT_WARNINGS

#define INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED \
	INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED \
	INTRA_WARNING_DISABLE_MOVE_IMPLICITLY_DELETED

#define INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED \
	INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED \
	INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED

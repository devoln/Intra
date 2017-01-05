#pragma once

#ifdef _MSC_VER

#define INTRA_WARNING_PUSH __pragma(warning(push))
#define INTRA_WARNING_POP __pragma(warning(pop))

#if _MSC_VER>=1900
#define INTRA_REDUNDANT_WARNINGS_MSVC14 4577
//4577 - ����� �� ������� �� noexcept ��� ����������� �����������

#define INTRA_WARNING_DISABLE_MOVE_IMPLICITLY_DELETED \
	__pragma(warning(disable : 5027))
#else
#define INTRA_REDUNDANT_WARNINGS_MSVC14
#define INTRA_WARNING_DISABLE_MOVE_IMPLICITLY_DELETED
#endif

//TODO: � ������� ��������� ���
#pragma warning(disable: 4710)

#define INTRA_DISABLE_REDUNDANT_WARNINGS \
	__pragma(warning(disable : 4714 4640 4514 4820 4711 4710 4061 4868 INTRA_REDUNDANT_WARNINGS_MSVC14))
//4714 - � ������ �� �������� �� ��, ��� forceinline �� ��������
//4640 - �������� � ������������ ���������� ������������ ������� ����� �������� � ������� ��� ������ � ��������
//4514 - ������������� �������, �� ������������ � �������, ���� �������
//4820 - �� �������� �� ������������
//4711 - �� �������� �� �������������� inline
//4710 - �� �������� �� �������������� �� inline
//4061 - �� �������� �� �������������� ���� case enum'�
//4868 - ���������� �� ����� ������������� ��������� ������� ���������� "����� �������" ��� ������ ���������������, ����������� � �������� ������


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

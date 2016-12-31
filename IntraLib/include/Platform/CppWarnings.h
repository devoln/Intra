#pragma once

#ifdef _MSC_VER

#pragma warning(disable: 4714) //� ������ �� �������� �� ��, ��� forceinline �� ��������
#pragma warning(disable: 4063) //�� �������� �� ������������ �������� � switch
//#pragma warning(disable: 4396) //������ ���, ������ ��� �������� ���� �����, ����� inline ���: "���� ������������� ���������� ��������� �� ������������� �������-�������, ���������� ������������ ������������ ����������"

//������� �������� �� -Wall
#if _MSC_VER>=1900
#pragma warning(disable: 4577) //����� �� ������� �� noexcept ��� ����������� �����������
#pragma warning(disable: 4868) //���������� �� ����� ������������� ��������� ������� ���������� "����� �������" ��� ������ ���������������, ����������� � �������� ������
#endif

#pragma warning(disable: 4608)
#pragma warning(disable: 4640) //�������� � ������������ ���������� ������������ ������� ����� �������� � ������� ��� ������ � ��������
#pragma warning(disable: 4514) //������������� �������, �� ������������ � �������, ���� �������
#pragma warning(disable: 4820) //�� �������� �� ������������
#pragma warning(disable: 4574) //... ������������ ��� "0": ������� � ���� ������������� "#if ..."?
#pragma warning(disable: 4711) //�� �������� �� �������������� inline
#pragma warning(disable: 4710) //�� �������� �� �������������� �� inline
#pragma warning(disable: 4061) //�� �������� �� �������������� ���� case enum'�

#define INTRA_WARNING_PUSH_DISABLE_COPY_MOVE_IMPLICITLY_DELETED \
__pragma(warning(push)) __pragma(warning(disable : 4625 4626))

#define INTRA_WARNING_POP \
__pragma(warning(pop))

#else

#define INTRA_WARNING_PUSH_DISABLE_COPY_MOVE_IMPLICITLY_DELETED
#define INTRA_WARNING_POP

#endif

#ifdef __clang__

#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wreserved-user-defined-literal"
#pragma clang diagnostic ignored "-Wlogical-op-parentheses"
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wunused-value"

#endif

#pragma once

#include "Platform/FundamentalTypes.h"
#include "Platform/CppWarnings.h"

namespace Intra { namespace Memory {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

enum class Access: byte {
	None, Read, Write, ReadWrite, Execute,
	ExecuteRead, ExecuteWrite, ExecuteReadWrite,
	End};

//! Получает память напрямую от ОС.
//! Память выделяется страницами, поэтому параметр bytes округляется вверх до размера страницы.
//! Если access==None, физическая память не выделяется, а резервируется только адресное пространство процесса.
//! Если access!=None выделяется физическая память с доступом access.
AnyPtr VirtualAlloc(size_t bytes, Access access);

//! Освобождает память, выделенную VirtualAlloc.
//! \param ptr Указатель, полученный от VirtualAlloc.
void VirtualFree(void* ptr, size_t size);

//! Для страниц, попадающих в интервал [ptr, ptr+bytes) устанавливается режим доступа access.
/*!
Если access==None, физическая память освобождается и соответствующие интервалу страницы остаются в зарезервированном состоянии.
Если access!=None выделяется физическая память с доступом access.
Для тех страниц, для которых физическая память уже была выделена просто меняется доступ.
*/
void VirtualCommit(void* ptr, size_t bytes, Access access);

size_t VirtualMemoryPageSize();

INTRA_WARNING_POP

}}

#pragma once

#ifdef _MSC_VER

#pragma warning(disable: 4714) //В дебаге не ругаться на то, что forceinline не сработал
#pragma warning(disable: 4063) //Не ругаться на недопустимые варианты в switch
//#pragma warning(disable: 4396) //Видимо баг, потому что ругается даже тогда, когда inline нет: "если дружественное объявление ссылается на специализацию функции-шаблона, встроенный спецификатор использовать невозможно"

//Убираем ненужное из -Wall
#if _MSC_VER>=1900
#pragma warning(disable: 4577) //Чтобы не ругался на noexcept при отключённых исключениях
#pragma warning(disable: 4868) //компилятор не может принудительно применить порядок вычисления "слева направо" для списка инициализаторов, заключенных в фигурные скобки
#endif

#pragma warning(disable: 4608)
#pragma warning(disable: 4640) //создание в конструкторе локального статического объекта может привести к ошибкам при работе с потоками
#pragma warning(disable: 4514) //подставляемая функция, не используемая в ссылках, была удалена
#pragma warning(disable: 4820) //не ругаться на выравнивание
#pragma warning(disable: 4574) //... определяется как "0": имелось в виду использование "#if ..."?
#pragma warning(disable: 4711) //не сообщать об автоматическом inline
#pragma warning(disable: 4710) //не сообщать об автоматическом не inline
#pragma warning(disable: 4061) //не ругаться на необработанные явно case enum'а

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

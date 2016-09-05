#pragma once

#include "Containers/ForwardDeclarations.h"

namespace Intra {

enum class MessageIcon: byte {NoIcon, Information, Question, Error, Warning};
void ShowMessageBox(StringView message, StringView caption, MessageIcon icon);

}

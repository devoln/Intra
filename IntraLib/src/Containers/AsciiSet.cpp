#include "Containers/AsciiSet.h"

namespace Intra {

const AsciiSet AsciiSet::Null = null;
const AsciiSet AsciiSet::Spaces = " \t\r\n";
const AsciiSet AsciiSet::Slashes = "/\\";
const AsciiSet AsciiSet::Digits = "0123456789";

const AsciiSet AsciiSet::LatinUppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const AsciiSet AsciiSet::LatinLowercase = "abcdefghijklmnopqrstuvwxyz";
const AsciiSet AsciiSet::Latin = AsciiSet::LatinUppercase|AsciiSet::LatinLowercase;
const AsciiSet AsciiSet::LatinAndDigits = AsciiSet::Latin|AsciiSet::Digits;

const AsciiSet AsciiSet::IdentifierChars = AsciiSet::LatinAndDigits|"$_";
const AsciiSet AsciiSet::NotIdentifierChars = ~AsciiSet::IdentifierChars;

}


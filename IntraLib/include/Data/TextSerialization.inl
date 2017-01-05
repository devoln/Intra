#pragma once

#include "Platform/CppWarnings.h"
#include "TextSerialization.h"
#include "Algo/String/Ascii.h"
#include "Algo/Search.h"

namespace Intra { namespace Data {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename O> void GenericTextSerializer<O>::NextField(TextSerializerParams::TypeFlags typeFlag)
{
	if(typeFlag & TextSerializerParams::TypeFlags_Struct)
	{
		INTRA_ASSERT(typeFlag==TextSerializerParams::TypeFlags_Struct);
		Output.WriteShortRaw(Lang.FieldSeparator);
	}
	if(typeFlag & TextSerializerParams::TypeFlags_Tuple)
	{
		INTRA_ASSERT(typeFlag==TextSerializerParams::TypeFlags_Tuple);
		Output.WriteShortRaw(Lang.TupleFieldSeparator);
	}
	if(typeFlag & TextSerializerParams::TypeFlags_Array)
	{
		INTRA_ASSERT(typeFlag==TextSerializerParams::TypeFlags_Array);
		Output.WriteShortRaw(Lang.ArrayElementSeparator);
	}
	if(typeFlag & TextSerializerParams::TypeFlags_StructArray)
	{
		INTRA_ASSERT(typeFlag==TextSerializerParams::TypeFlags_StructArray);
		Output.WriteShortRaw(Lang.ArrayElementSeparator);
	}
	EndField(typeFlag);
}

inline bool TextDeserializer::IgnoreArrayElement()
{
	int counter=1;
	Input.ReadRecursiveBlock(counter, null, Lang.ArrayClose, Lang.ArrayElementSeparator,
	{
		{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
		{Lang.OneLineCommentBegin, StringView("\n")},
		{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd}
	},
	{
		{Lang.StructInstanceOpen, Lang.StructInstanceClose},
		{Lang.ArrayOpen, Lang.ArrayClose}
	});
	if(counter==0)
	{
		Input.Rest = StringView(Input.Rest.Data()-Lang.ArrayClose.Length(), Input.Rest.End());
		return true;
	}
	INTRA_ASSERT(counter==1);
	return false;
}

inline bool TextDeserializer::IgnoreField()
{
	int counter=1;
	Input.ReadRecursiveBlock(counter, Lang.StructInstanceOpen, Lang.StructInstanceClose, Lang.FieldSeparator,
	{
		{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
		{Lang.OneLineCommentBegin, "\n"},
		{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd},
		{Lang.ArrayOpen, Lang.ArrayClose}
	});
	INTRA_ASSERT(counter==0 || counter==1);
	return counter==0;
}

inline bool TextDeserializer::NextField(TextSerializerParams::TypeFlags typeFlag)
{
	if(typeFlag & TextSerializerParams::TypeFlags_Struct)
	{
		INTRA_ASSERT(typeFlag==TextSerializerParams::TypeFlags_Struct);
		if(!Expect(Lang.FieldSeparator))
		{
			if(!Input.Rest.Empty()) IgnoreField();
			return false;
		}
	}
	if(typeFlag & TextSerializerParams::TypeFlags_Tuple)
	{
		INTRA_ASSERT(typeFlag==TextSerializerParams::TypeFlags_Tuple);
		if(!Expect(Lang.TupleFieldSeparator))
		{
			if(!Input.Rest.Empty()) IgnoreField();
			return false;
		}
	}
	if(typeFlag & TextSerializerParams::TypeFlags_Array)
	{
		INTRA_ASSERT(typeFlag==TextSerializerParams::TypeFlags_Array);
		if(!Expect(Lang.ArrayElementSeparator))
			return !Input.Rest.Empty() && !IgnoreArrayElement();
	}
	if(typeFlag & TextSerializerParams::TypeFlags_StructArray)
	{
		INTRA_ASSERT(typeFlag==TextSerializerParams::TypeFlags_StructArray);
		if(!Expect(Lang.ArrayElementSeparator))
			return !Input.Rest.Empty() && !IgnoreArrayElement();
	}
	SkipAllSpacesAndComments();
	return true;
}

template<typename O> void GenericTextSerializer<O>::EndField(TextSerializerParams::TypeFlags typeFlag)
{
	if((Params.ValuePerLine & typeFlag) == 0)
	{
		Output.WriteCharRaw(' ');
		return;
	}
	Output.WriteShortRaw(Params.LineEnding);
	if((Params.UseTabs & typeFlag)==0) return;
	for(int i=0; i<NestingLevel; i++)
		Output.WriteShortRaw(Params.TabChars);
}

template<typename O> void GenericTextSerializer<O>::FieldAssignmentBeginning(StringView name)
{
	INTRA_ASSERT(name!=null);
	Output.WriteShortRaw(Lang.LeftFieldNameBeginQuote);
	Output.WriteRaw(name);
	Output.WriteShortRaw(Lang.LeftFieldNameEndQuote);

	if(Params.UseAssignmentSpaces)
		Output.WriteCharRaw(' ');

	Output.WriteShortRaw(Lang.LeftAssignmentOperator);

	if(Params.UseAssignmentSpaces && Lang.LeftAssignmentOperator!=null)
		Output.WriteCharRaw(' ');
}

inline StringView TextDeserializer::FieldAssignmentBeginning()
{
	auto oldPos = Input.Rest;
	auto oldLine = Line;
	SkipAllSpacesAndComments();
	if(!Input.Expect(Lang.LeftFieldNameBeginQuote))
	{
		Input.Rest = oldPos;
		Line = oldLine;
		return null;
	}
	SkipAllSpacesAndComments();
	StringView name = Input.ParseIdentifier();
	SkipAllSpacesAndComments();
	if(!Input.Expect(Lang.LeftFieldNameEndQuote))
	{
		Input.Rest = oldPos;
		Line = oldLine;
		return null;
	}
	SkipAllSpacesAndComments();
	if(!Input.Expect(Lang.LeftAssignmentOperator))
	{
		Input.Rest = oldPos;
		Line = oldLine;
		return null;
	}
	return name;
}

template<typename O> void GenericTextSerializer<O>::FieldAssignmentEnding(StringView name)
{
	INTRA_ASSERT(name!=null);

	if(Params.UseAssignmentSpaces)
		Output.WriteCharRaw(' ');

	Output.WriteShortRaw(Lang.RightAssignmentOperator);

	if(Params.UseAssignmentSpaces && Lang.RightAssignmentOperator!=null)
		Output.WriteCharRaw(' ');

	Output.WriteShortRaw(Lang.RightFieldNameBeginQuote);
	Output.WriteRaw(name);
	Output.WriteShortRaw(Lang.RightFieldNameEndQuote);
}

inline StringView TextDeserializer::FieldAssignmentEnding()
{
	auto oldPos = Input.Rest;
	auto oldLine = Line;
	SkipAllSpacesAndComments();
	if(!Input.Expect(Lang.RightAssignmentOperator))
	{
		Input.Rest = oldPos;
		Line = oldLine;
		return null;
	}
	SkipAllSpacesAndComments();
	if(!Input.Expect(Lang.RightFieldNameBeginQuote))
	{
		Input.Rest = oldPos;
		Line = oldLine;
		return null;
	}
	SkipAllSpacesAndComments();
	StringView name = Input.ParseIdentifier();
	SkipAllSpacesAndComments();
	if(!Input.Expect(Lang.RightFieldNameEndQuote))
	{
		Input.Rest = oldPos;
		Line = oldLine;
		return null;
	}
	return name;
}

template<typename O> void GenericTextSerializer<O>::StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags type)
{
	Output.WriteShortRaw((type & TextSerializerParams::TypeFlags_Struct)? Lang.StructInstanceOpen: Lang.TupleOpen);
	NestingLevel++;
	EndField(type);
}

inline void TextDeserializer::StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags type)
{
	if(!Expect((type & TextSerializerParams::TypeFlags_Struct)? Lang.StructInstanceOpen: Lang.TupleOpen))
	{
		if(Input.Rest.Empty()) return;
		int counter = -1;
		Input.ReadRecursiveBlock(counter, Lang.StructInstanceOpen, null, null,
			{
				{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
				{Lang.OneLineCommentBegin, "\n"},
				{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd},
				{Lang.StringQuote, Lang.StringQuote}
			},
			{
				{Lang.ArrayOpen, Lang.ArrayClose}
			}
		);
	}
	NestingLevel++;
}

template<typename O> void GenericTextSerializer<O>::StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags type)
{
	NestingLevel--;
	EndField(type);
	Output.WriteShortRaw((type & TextSerializerParams::TypeFlags_Struct)? Lang.StructInstanceClose: Lang.TupleClose);
}

inline void TextDeserializer::StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags type)
{
	NestingLevel--;
	if(!Expect((type & TextSerializerParams::TypeFlags_Struct)? Lang.StructInstanceClose: Lang.TupleClose))
	{
		int counter = 1;
		Input.ReadRecursiveBlock(counter, Lang.StructInstanceOpen, Lang.StructInstanceClose, null,
			{
				{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
				{Lang.OneLineCommentBegin, "\n"},
				{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd},
				{Lang.StringQuote, Lang.StringQuote}
			},
			{
				{Lang.ArrayOpen, Lang.ArrayClose}
			}
		);
	}
}



template<typename T> void TextDeserializer::DeserializeStruct(T& dst, ArrayRange<const StringView> fieldNames)
{
	StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags_Struct);
	TextDeserializerStructVisitor visitor{this, fieldNames, false, TextSerializerParams::TypeFlags_Struct};
	if(fieldNames.Empty())
	{
		Meta::ForEachField(dst, visitor);
		StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags_Struct);
		return;
	}

	bool expectSeparator=false;
	for(size_t i=0; ; i++)
	{
		SkipAllSpacesAndComments();
		if(Input.Rest.StartsWith(Lang.StructInstanceClose)) break;

		if(expectSeparator)
		{
			if(!NextField(TextSerializerParams::TypeFlags_Struct)) break;
		}
		expectSeparator=true;

		StringView name = FieldAssignmentBeginning();

		//TODO: учесть, что имя поля может идти после его значения
		size_t index = 0;
		if(name.Empty()) index = fieldNames.Length();
		else index = Algo::CountUntil(fieldNames, name);
		if(index==fieldNames.Length() && name!=null) //Если такого поля в структуре нет, пропускаем его
		{
			const bool finished = IgnoreField();
			if(!finished)
			{
				expectSeparator=false;
				continue;
			}
			NestingLevel--;
			return;
		}
		if(index!=fieldNames.Length() || name==null)
			dst.VisitFieldById(name!=null? index: i, *this);

		if(Lang.AddFieldNameAfterAssignment)
		{
			StringView name2 = FieldAssignmentEnding();
			if(name!=null && name2!=null && name!=name2) LogExpectError(name2, {name});
		}
	}
	StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags_Struct);
}

template<typename T> void TextDeserializer::DeserializeTuple(T& dst)
{
	StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags_Tuple);
	TextDeserializerStructVisitor visitor{this, null, false, TextSerializerParams::TypeFlags_Tuple};
	Meta::ForEachField(dst, visitor);
	StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags_Tuple);
}



template<typename O> forceinline void GenericTextSerializer<O>::SerializeBool(bool v)
{Output.WriteShortRaw(Lang.FalseTrueNames[v!=false]);}

inline bool TextDeserializer::DeserializeBool()
{
	SkipAllSpaces();
	StringView identifier = Input.ParseIdentifier();
	if(identifier==Lang.FalseTrueNames[true]) return true;
	if(identifier!=Lang.FalseTrueNames[false])
		LogExpectError(identifier, Lang.FalseTrueNames);
	return false;
}

template<typename O> void SerializeText(GenericTextSerializer<O>& serializer, const StringView& v)
{
	serializer.Output.WriteShortRaw(serializer.Lang.StringQuote);
	serializer.Output.WriteReplacedString(v, {"\n", "\r", "\t"}, {"\\n", "\\r", "\\t"});
	serializer.Output.WriteShortRaw(serializer.Lang.StringQuote);
}

inline void DeserializeText(TextDeserializer& deserializer, String& dst)
{
	if(!deserializer.Expect(deserializer.Lang.StringQuote)) return;
	StringView escapedResult = deserializer.Input.ReadUntil(deserializer.Lang.StringQuote);
	deserializer.Input.Rest.PopFirstN(deserializer.Lang.StringQuote.Length());

	dst.SetLengthUninitialized(Algo::StringMultiReplaceAsciiLength(escapedResult, {"\\n", "\\r", "\\t"}, {"\n", "\r", "\t"}));
	auto dstRange = dst.AsRange();
	Algo::StringMultiReplaceAscii(escapedResult, dstRange, {"\\n", "\\r", "\\t"}, {"\n", "\r", "\t"});


	/*Range::CountRange<char> counter;
	const KeyValuePair<StringView, StringView> replacements[] = {{"\\n", "\n"}, {"\\r", "\r"}, {"\\t", "\t"}};
	escapedResult.MultiReplaceToAdvance(counter, AsRange(replacements));
	dst.SetLengthUninitialized(counter.Counter);
	auto strData = dst.AsRange();
	escapedResult.MultiReplaceToAdvance(strData, AsRange(replacements));*/
}


template<typename O> template<typename R>
void GenericTextSerializer<O>::SerializeRange(const R& range)
{
	Output.WriteShortRaw(Lang.ArrayOpen);
	if(!range.Empty())
	{
		typedef Range::ValueTypeOf<R> T;
		const bool simpleArray = ValueType::Of<T>()!=ValueType::End;
		NestingLevel += int(!simpleArray && Lang.ArrayOpen!=null && Lang.ArrayClose!=null);
		if(simpleArray? false: (Lang.StructInstanceOpen!=null))
			EndField(TextSerializerParams::TypeFlags_StructArray);

		auto rangeCopy = range;
		SerializeText(*this, rangeCopy.First());
		rangeCopy.PopFirst();
		while(!rangeCopy.Empty())
		{
			NextField(simpleArray?
				TextSerializerParams::TypeFlags_Array:
				TextSerializerParams::TypeFlags_StructArray);
			SerializeText(*this, rangeCopy.First());
			rangeCopy.PopFirst();
		}

		NestingLevel -= int(!simpleArray && Lang.ArrayOpen!=null && Lang.ArrayClose!=null);
		if(simpleArray? false: (Lang.StructInstanceClose!=null))
			EndField(TextSerializerParams::TypeFlags_StructArray);
	}
	Output.WriteShortRaw(Lang.ArrayClose);
}

template<typename R> void TextDeserializer::DeserializeRange(R& outputRange)
{
	typedef Range::ValueTypeOf<R> T;

	if(!Expect(Lang.ArrayOpen))
	{
		int counter = -1;
		Input.ReadRecursiveBlock(counter, Lang.ArrayOpen, null, null,
			{
				{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
				{Lang.OneLineCommentBegin, "\n"},
				{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd},
				{Lang.StringQuote, Lang.StringQuote}
			},
			{
				{Lang.StructInstanceOpen, Lang.StructInstanceClose}
			}
		);
	}

	SkipAllSpacesAndComments();
	if( (Lang.ArrayClose.Empty() && !Input.Rest.StartsWith(Lang.RightFieldNameBeginQuote)) ||
		!Input.Rest.StartsWith(Lang.ArrayClose) )
	{
		outputRange.Put(Deserialize<T>());
		SkipAllSpacesAndComments();
		while( (Lang.ArrayClose.Empty() && !Input.Rest.StartsWith(Lang.RightFieldNameBeginQuote)) ||
			!Input.Rest.StartsWith(Lang.ArrayClose) )
		{
			if(NextField(TextSerializerParams::TypeFlags_Array))
				outputRange.Put(Deserialize<T>());
			else if(Input.Rest.Empty()) return;
			SkipAllSpacesAndComments();
		}
	}

	if(!Expect(Lang.ArrayClose))
	{
		int counter = 1;
		Input.ReadRecursiveBlock(counter, Lang.ArrayOpen, Lang.ArrayClose, null,
			{
				{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
				{Lang.OneLineCommentBegin, "\n"},
				{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd},
				{Lang.StringQuote, Lang.StringQuote}
			},
			{
				{Lang.StructInstanceOpen, Lang.StructInstanceClose}
			}
		);
	}
}

INTRA_WARNING_POP

}}

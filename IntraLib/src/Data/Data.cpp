
#if 0

#include "Data/Data.h"

#if INTRA_DISABLED


data::Value data::Bin::GetValue(const string& name) const
{
	auto names = name.Split("./\\:", "[]");
	return GetValue(names);
}

data::Value data::Bin::GetValue(ArrayRange<const string> names) const
{
	if(names==null) return{Variable::Null(), ValueType::Void};
	intptr index = array_names.Find(names[0]);
	if(index==-1)
	{
		index=object_names.Find(names[0]);
		if(index==-1) return{Variable::Null(), ValueType::Void};
		++names.Begin;
		return objects[index].GetValue(names);
	}
	++names.Begin;
	return arrays[index].GetValue(names);
}

data::Value data::Array::GetValue(ArrayRange<const string> names) const
{
	if(names==null) return{Variable::Null(), ValueType::Void};
	assert(names[0]=="[" && names[2]=="]");
	intptr index = (intptr)names[1].ToInt();
	names.Begin+=3;
	if(ContainObjects) return Objects[index].GetValue(names);
	assert(names==null);
	return Values[index];
}

data::Value data::Object::GetValue(ArrayRange<const string> names) const
{
	if(names==null) return{Variable::Null(), ValueType::Void};
	intptr index = array_names.Find(names[0]);
	if(index==-1)
	{
		index=value_names.Find(names[0]);
		if(index==-1) return{Variable::Null(), ValueType::Void};
		++names.Begin;
		return values[index];
	}
	++names.Begin;
	return arrays[index].GetValue(names);
}
#endif

#include "IO/File.h"

static bool is_separator(char c)
{
	return c=='(' || c==')' || c==',';
}

DataBin DataBin::FromBinaryStream(IO::IInputStream& s)
{
	DataBin bin;
	bin.data.Resize((uintptr)s.GetSize());
	bin.data = s.ReadDataAsBuffer((uintptr)s.GetSize());
	auto ptr = bin.data.Data();
	ushort signature = *(ushortLE*)ptr;
	ptr+=2;
	byte majorVersion = *ptr++;
	byte minorVersion = *ptr++;
	if(signature!=0xBCF0 && majorVersion==0)
	{
		bin.data=null;
		bin.headerOffset=0;
		bin.headerSize=0;
		bin.valuesOffset=0;
		bin.valuesSize=0;
	}
	bin.headerOffset = 8;
	return bin;
}

DataBin DataBin::FromTextStream(IO::IInputStream& s)
{
	DataBin bin;
	bin.data=null;
	bin.headerOffset=0;
	bin.headerSize=0;
	bin.valuesOffset=0;
	bin.valuesSize=0;
	return bin;
}

DataBin DataBin::FromBinaryFile(StringView filename)
{
	IO::DiskFile::Reader file(filename);
	return FromBinaryStream(file);
}

DataBin DataBin::FromTextFile(StringView filename)
{
	IO::DiskFile::Reader file(filename);
	return FromTextStream(file);
}



string DataBinWriter::GetName(uint& headerOffset) const
{
	uint oldHeaderOffset=headerOffset;
	while(headerOffset<Header.Length() && !is_separator(Header[headerOffset])) headerOffset++;
	string result = Header(oldHeaderOffset, headerOffset);
	while(headerOffset<Header.Length() && is_separator(Header[headerOffset])) headerOffset++;
	return result;
}


void DataBinWriter::AddValueBlock(ValueType type, uint count)
{
	data.AddLast(type);
	AddValueData<uint24LE>(count);
}

void DataBinWriter::AddValueBlock(StringView name, ValueType type, uint elementCount)
{
	if(Header[Header.Length()-1]!='(') Header+=',';
	Header+=name;
	AddValueBlock(type, elementCount);
}


void DataBinWriter::AddRawData(const void* arr, uintptr bytes)
{
	data.AddLast(arr, bytes);
}

byte& DataBinWriter::AddNamespaceBlock()
{
	data.AddLast(DataBin::BlockId::Namespace);
	data.AddLast<byte>(0);
	return *(data.End()-1);
}

/*void DataBinWriter::AddRefBlock(uint offset)
{
	data.AddLast(DataBin::BlockId::Ref);
	data.AddLast<uint24LE>(offset);
}*/

byte& DataBinWriter::AddStructBlock()
{
	data.AddLast(ValueType::StructureType);
	data.AddLast<byte>(0);
	return *(data.End()-1);
}

uint24LE* DataBinWriter::AddEnumBlock()
{
	data.AddLast(DataBin::BlockId::Enum);
	data.AddLast<uint24LE>(0);
	return (uint24LE*)(data.End()-3);
}

void DataBinWriter::AddStructInstanceBlock(byte importIndex, uint structDefPos, uint elementCount)
{
	data.AddLast(ValueType::StructureInstance);
	data.AddLast<byte>(importIndex);
	data.AddLast<uintLE>(structDefPos);
	data.AddLast<uintLE>(elementCount);
}




#include "Text/Parser.h"

#ifndef PARSER_NO_ERROR_CHECKING
#define ParserAssert(cond, message) ((cond) || OutErrorLog!=null && PrintError("("+ToString(GetLine())+"): "+message))
#else
#define ParserAssert(...) true
#endif
class TextDataParser: public Parser, public DataBinWriter
{
public:
	TextDataParser(StringView code): Parser(code)
	{
		//Резервируем память с запасом, чтобы гарантированно хватило места
		Header.Reserve(code.Length());
		data.Reserve(code.Length());
	}

	void ParseValue(ValueType& type, bool isArray)
	{
		byte* dst = data.End();
		bool success;
		ParseToBinary(type, isArray, dst, data.Size()-data.UsedBytes(), &success);
		data.Extend(dst-data.End());
	}

	StringView ParseBlockBeginning()
	{
		StringView name = ParseIdentifier(false);
		Token tok = PopToken();
		if(!CheckTokenIsExpected(tok, "{")) return name;
		Header += name;
		Header += '(';
		return name;
	}

	void ParseBlockEnding()
	{
		PopToken();
		SkipSemicolons();
		Header+=')';
	}
	void ParseNamespaceBody()
	{
		byte& children = AddNamespaceBlock();
		while(GetNextToken()!="}")
		{
			ParseBlock();
			children++;
		}
	}

	void ParseNamespace()
	{
		/*StringView name = */ParseBlockBeginning();
		ParseNamespaceBody();
		ParseBlockEnding();
	}

	void ParseVariableDeclarationLineAfterType(ByteBuffer& defaultValues, UniformType type, byte importIndex, uint typeIndex, uintptr arrSize)
	{
		StringView name = ParseIdentifier(false);
		if(Header[Header.Length()-1]!='(') Header += ",";
		Header+=name;
		if(type==ValueType::StructureInstance)
			AddStructInstanceBlock(importIndex, typeIndex, (uint)arrSize);
		else if(type==ValueType::StructureType) AddStructBlock();
		else AddValueBlock(type.ToValueType(), (uint)arrSize);
		Token tok = PopToken();
		if(tok==";") return;
	}

	void ParseVariableDeclarationLine(ByteBuffer& defaultValues)
	{
		UniformType type;
		uint arrSize;
		StringView typeName;
		ParseType(type, arrSize, true, &typeName);
		int typeIndex=0;
		byte importIndex=0;
		if(type==ValueType::StructureInstance)
		{
			for(typeIndex=int(Structures.Count()-1); typeIndex>=0; typeIndex--)
			{
				if(GetName(Structures[typeIndex].HeaderOffset)!=typeName) continue;
				break;
			}
		}
		ParserAssert(typeIndex!=-1, "Тип "+typeName+"не определён.");
#ifndef PARSER_NO_ERROR_CHECKING
		if(typeIndex==-1)
		{
			Ignore();
			return;
		}
#endif
		ParseVariableDeclarationLineAfterType(defaultValues, type, importIndex, typeIndex, arrSize);
	}

	void ParseStructBody()
	{
		byte& fieldCount = AddStructBlock();
		ByteBuffer defaultValues; //Значение структуры по умолчанию записывается отдельно непрерывным куском
		while(GetNextToken()!="}")
		{
			ParseVariableDeclarationLine(defaultValues);
			fieldCount++;
		}
	}

	void ParseStruct()
	{
		/*StringView name = */ParseBlockBeginning();
		ParseStructBody();
		ParseBlockEnding();
	}

	void ParseEnum()
	{

	}

	void ParseBlock()
	{
		StringView name = ParseIdentifier(false);
		bool internal = name!="public";
		if(!internal || name=="internal")
			name = ParseIdentifier(false);

		if(name=="namespace")
		{
			ParseNamespace();
			return;
		}

		if(name=="struct")
		{
			ParseStruct();
			return;
		}

		if(name=="enum")
		{
			ParseEnum();
			return;
		}

		/*bool arr=false;
		ValueType type = ValueType::FromStringGLSL(name);

		if(type!=ValueType::Void || name=="void")
		{
			if(GetNextToken()=="[")
			{
				PopToken();
				arr=true;
				SkipExpectedToken("]");
			}
			name = ParseIdentifier(false);
		}

		Header+=name;
		
		if(tok=="," || tok==";")
		{
			Header+=",";
			if(type==ValueType::Void) AddNamespaceBlock();
			else AddValueBlock(type);
			return;
		}
		if(tok=="=")
		{
			ParseValue(type, arr);
		}*/
	}
};





DataBin FromTextStream(IO::IInputStream& s)
{
	DataBin bin;
	string str = s.ReadNChars((uintptr)s.GetSize());
	TextDataParser parser(str);

	return bin;
}

#endif


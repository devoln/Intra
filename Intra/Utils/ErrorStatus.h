#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"
#include "Utils/StringView.h"
#include "Utils/FixedArray.h"
#include "Utils/Debug.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_NO_VIRTUAL_DESTRUCTOR

namespace Intra { namespace Utils {

enum class ErrorCode: byte {NoError, InvalidArguments, NotFound, OutOfMemory, AlreadyUsed, NoAccess};

//! ������� �����, ������� ���������� �� ������.
//! ������ ������� ����� �������� ���������� � ���, ��������� �� ������ � ���� �� ��� ����������.
//! ��� ���� ��� �� ����� �� ��������� �� ���� ������ � �� ��, ��� ��� �� ���� ����������.
class ErrorStatus
{
protected:
	enum class State: byte {Ok, Error, HandledError};
	State mState = State::Ok;
public:
	void* operator new(size_t size) = delete;
	void operator delete(void *p) = delete;
	ErrorStatus() {}

	ErrorStatus(const ErrorStatus&) = delete;
	ErrorStatus& operator=(const ErrorStatus&) = delete;

	virtual void Error(StringView msg, SourceInfo srcInfo=null)
	{
		mState = State::Error;
		(void)msg;
		(void)srcInfo;
	}

	forceinline bool WasError() const noexcept {return mState >= State::Error;}

	forceinline bool Handle() noexcept
	{
		if(mState != State::Error) return false;
		mState = State::HandledError;
		return true;
	}
};

//! ��. Error::Skip.
class SkipErrorStatus: public ErrorStatus
{
public:
	SkipErrorStatus() {}
	SkipErrorStatus(SkipErrorStatus&&) {}
	void Error(StringView, SourceInfo) override {}
};

//! ��������� ��� ���������� � ������������ ������.
//! �������� ��������� ���������� ��������� � ������� ������ �� �����,
//! ���� ������ �� ���� ���������� �� ������ ������� �� ������� ���������.
class FatalErrorStatus: public ErrorStatus
{
public:
	FatalErrorStatus() {}

	~FatalErrorStatus() noexcept
	{
		FatalErrorMessageAbort(mSrcInfo, CSpanOf(mMsg));
	}

	void Error(StringView msg, SourceInfo srcInfo) override
	{
		mState = State::Error;
		mSrcInfo = srcInfo;
		mMsg.SetCount(msg.Length());
		msg.CopyTo(mMsg);
	}

	forceinline SourceInfo ErrorSourceInfo() const noexcept {return mSrcInfo;}

private:
	SourceInfo mSrcInfo = {null, null, 0};
	FixedArray<char> mMsg;
};

}

using Utils::ErrorStatus;
using Utils::FatalErrorStatus;

namespace Error {
//! ���������� ��� ������. ���� � ������ ������ WasError ����� false.
//! ����������� � ������, ����� ��������� ����������� �������� �� ����� ���������� ��� ������,
//! � ����� ������������ ��������� �� ���������.
inline Utils::SkipErrorStatus Skip() {return Utils::SkipErrorStatus();}
}

}

INTRA_WARNING_POP

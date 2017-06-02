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
	enum class State: byte {Ok, Warning, Error, HandledError};
	State mState = State::Ok;
public:
	void* operator new(size_t size) = delete;
	void operator delete(void *p) = delete;
	ErrorStatus() {}

	ErrorStatus(const ErrorStatus&) = delete;
	ErrorStatus& operator=(const ErrorStatus&) = delete;

	virtual void Error(StringView msg, SourceInfo srcInfo = null)
	{
		mState = State::Error;
		(void)msg;
		(void)srcInfo;
	}

	virtual void Warning(StringView msg, SourceInfo srcInfo = null)
	{
		if(mState == State::Ok) mState = State::Warning;
		(void)msg;
		(void)srcInfo;
	}

	forceinline bool WasError() const noexcept {return mState >= State::Error;}
	forceinline bool WasUnhandledError() const noexcept {return mState == State::Error;}

	forceinline bool Handle() noexcept
	{
		if(mState != State::Error) return false;
		mState = State::HandledError;
		return true;
	}
};

//! ��. Error::Skip().
class SkipErrorStatus: public ErrorStatus
{
public:
	SkipErrorStatus() {}
	SkipErrorStatus(SkipErrorStatus&&) {}
	void Error(StringView, SourceInfo) override {}
	void Warning(StringView, SourceInfo) override {}
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
		if(mState != State::Error) return;
		FatalErrorMessageAbort(null, mMsg, false);
	}

	void Error(StringView msg, SourceInfo srcInfo) override
	{
		mState = State::Error;
		mMsg = BuildAppendDiagnosticMessage(mMsg, "ERROR",
			StringView(srcInfo.Function), StringView(srcInfo.File), srcInfo.Line, msg);
	}

	void Warning(StringView msg, SourceInfo srcInfo) override
	{
		if(mState < State::Warning) mState = State::Warning;
		mMsg = BuildAppendDiagnosticMessage(mMsg, "WARNING",
			StringView(srcInfo.Function), StringView(srcInfo.File), srcInfo.Line, msg);
	}

	StringView GetLog() const {return mMsg;}

private:
	FixedArray<char> mMsg;
};

}

using Utils::ErrorStatus;
using Utils::FatalErrorStatus;

namespace Error {
//! ���������� ��� ������. ���� � ������ ������ WasError ����� false.
//! ����������� � ������, ����� ��������� ����������� �������� �� ����� ���������� ��� ������,
//! � ����� ������������ ��������� �� ���������.
inline Utils::SkipErrorStatus& Skip()
{
	static Utils::SkipErrorStatus result;
	return result;
}

}

}

INTRA_WARNING_POP

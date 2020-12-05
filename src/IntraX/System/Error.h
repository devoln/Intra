#pragma once

#include "Intra/Functional.h"
#include "Intra/Range/StringView.h"
#include "Intra/Assert.h"

#include "IntraX/System/Debug.h"

INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_MOVE_IMPLICITLY_DELETED
/** Contains error code, which value depends on a platform.
  Implementation guarantees that:
  1) 0 is not treated as an error, any other value is an error.
  2) values in range [1; 0x7FFFFFFF) are never used by Extra on any platform so users can safely define and interpret these values on their own.
  3) on Windows Intra assigns its value to HRESULT errors, so the user can do the same, however, do not assign successful results other than S_OK!
  4) on POSIX platform its value is 0x80000000 | errno
  5) on all other platforms it is an unspecified value in range [0x80000000; 0x9FFFFFFF]
  6) Intra reserves [0xA0000000; 0xAFFFFFFF] range for itself

  Use it to store errors only. For example avoid storing HTTP 100-399 success codes in ErrorCode. They should map to 0.
  If you have to do this anyway, do not rely on contextual conversion to bool.

  The best way to create globally unique custom error codes is to determine how much errors your project has,
  then reserve log2 of this number lower bits and generate random value for higher bits.
  For example, if your project needs 128 <= n <= 256 custom errors, you can generate a random 23-bit (= 31-8) number and use it as a prefix for your error codes.
*/
struct ErrorCode
{
	enum: uint32
	{
		NoError = 0,
		//TODO: Ñheck ñarefully that windows and posix error codes correspond to each other
#ifdef _WIN32
		InvalidArguments = 0x80070057,
		FileNotFound = 0x80070002,
		OutOfMemory = 0x8007000E,
		PermissionDenied = 0x80070005,
		NotImplemented = 0x80004001,
		AlreadyExists = 0x800700B7,
		FileBusy = 0x800700AA,
		BrokenPipe = 0x800700E9,
		EndOf = 0x80070026,
		WaitTimeout = 0x80070102,
		ConnectionTimeout = 0x8007274C,
		Other = 0x80004005,
		//Add missing counterparts to the posix values listed below
		//bit 29 is customer defined so we can define our own values here
#else
		InvalidArguments = 0x80000000|22, //EINVAL
		FileNotFound = 0x80000000|2, //ENOENT
		OutOfMemory = 0x80000000|12, //ENOMEM
		PermissionDenied = 0x80000000|13, //EACCES
		NotImplemented = 0x80000000|38, //ENOSYS
		AlreadyExists = 0x80000000|17, //EEXIST
		FileBusy = 0x80000000|16, //EBUSY. TODO: Or ETXTBSY (26)?
		BrokenPipe = 0x80000000|32, //EPIPE
		EndOf = 0xA0000E0F,
		WaitTimeout = 0x80000000|62, //ETIME
		IOError = 0x80000000|5, //EIO
		TooManyOpenFiles = 0x80000000|24, //EMFILE
		NoSpaceLeft = 0x80000000|28, //ENOSPC
		ReadOnlyFS = 0x80000000|30, //EROFS
		NotADirectory = 0x80000000|20, //ENOTDIR
		IsADirectory = 0x80000000|21, //EISDIR
		Deadlock = 0x80000000|35, //EDEADLK
		NameTooLong = 0x80000000|36, //ENAMETOOLONG
		ConnectionTimeout = 0x80000000|110, //ETIMEOUT
		ProtocolError = 0x80000000|71, //EPROTO
		NetworkIsDown = 0x80000000|100, //ENETDOWN
		NetworkIsUnreachable = 0x80000000|101, //ENETUNREACH
		NetworkConnectionReset = 0x80000000|102, //ENETRESET
		ConnectionRefused = 0x80000000|111, //ECONNREFUSED
		OperationCanceled = 0x80000000|125, //ECANCELED
		Other = 0x80004005,
		//TODO: add more error codes
#endif
		InvalidStream = 0xA0000000,
		InvalidChecksum = 0xA0000001,
		UndeclaredIdentifier = 0xA0000002,
		UnexpectedToken = 0xA0000003,
		TypeError = 0xA0000004,
		NotSupported = 0xA0000005
	};
	uint32 Value = 0;

	constexpr ErrorCode() noexcept = default;
	constexpr ErrorCode(uint32 value) noexcept: Value(value) {}

	[[nodiscard]] constexpr explicit operator bool() const noexcept {return Value != 0;}

	/** Convert the error code to its string representation using platform specific API and default locale.
	  For custom values prints "ErrorCode(<Value (hex)>)" if no corresponsding range is found.
	  Advances ``dstBuf`` with number of written characters, crops result if the provided buffer doesn't have enough size.
	  @return StringView referring to written result in provided buffer.
	*/
	[[nodiscard]] StringView ToString(Span<char>& dstBuf) const noexcept
	{
		(void)dstBuf; // TODO: not implemented yet
		return "";
	}
};

/** Base class, useful to track errors of some operation.
  It contains information if there was an error and whether it has been handled.

  The pattern of this class usage is as follows.
  A top level function or an object that is responsible of error handling declares ErrorStatus or one of its derivatives in some scope.
  Then it passes an ErrorReporter referenced to it to every function that may fail.
  If some called function fails it calls Error() on it and returns.
  Then the caller uses Handle() method to get information about the very first error:
  ``if(auto err = errorStatus.Handle()) {...}``
  After call to Handle() the error is considered handled.
  Derived classes may define destructor to react to unhandled errors.
*/
struct ErrorStatus
{
protected:
	enum class State: byte {Ok, Error, HandledError};
	State mState = State::Ok;
	ErrorCode mErrorCode;
	SourceInfo mSourceInfo{};
public:
	// ErrorStatus should only be allocated on the stack.
	// Heap allocation does not make any sense.
	void* operator new(size_t) = delete;
	void operator delete(void*) = delete;

	ErrorStatus() = default;

	ErrorStatus(const ErrorStatus&) = delete;
	ErrorStatus& operator=(const ErrorStatus&) = delete;

	constexpr void Error(ErrorCode errorCode, SourceInfo srcInfo = SourceInfo()) noexcept
	{
		if(mState == State::Error || !errorCode) return; //If there was an unhandled error we won't overwrite it
		mState = State::Error;
		mErrorCode = errorCode;
		if constexpr(!Config::DisableTroubleshooting)
			mSourceInfo = srcInfo;
	}

	[[nodiscard]] constexpr bool WasError() const noexcept {return mState >= State::Error;}
	[[nodiscard]] constexpr bool WasUnhandledError() const noexcept {return mState == State::Error;}

	/**
	  @return ErrorCode for the very first unhandled error or null if there was no error.
	  Marks the error as handled.
	*/
	[[nodiscard]] constexpr ErrorCode Handle() noexcept
	{
		if(mState != State::Error) return ErrorCode{};
		mState = State::HandledError;
		return mErrorCode;
	}

	/// Makes last error unhandled.
	constexpr void HandlingFailed() noexcept
	{
		INTRA_PRECONDITION(mState != State::Ok);
		mState = State::Error;
	}
};

/// Ignores all errors. WasError(), WasUnhandledError() and Handle() always return false.
constexpr struct {} IgnoreErrors;

/** This class is used to reference to ErrorStatus or ignore all errors.
  Any function that may fail unpredictably should take an argument of type ErrorReporter.
  Even if function fails it must return some result specified in its documentation. It is usually an empty result.
  It makes possible ignoring errors or handling any error in one place without exceptions.
*/
class ErrorReporter
{
	ErrorStatus* mStatus = null;
public:
	IFunctor<void(ErrorCode, StringView, SourceInfo)>* Logger = null;

	constexpr ErrorReporter(decltype(IgnoreErrors), IFunctor<void(ErrorCode, StringView, SourceInfo)>* logger = null) noexcept: Logger(logger) {}
	constexpr ErrorReporter(ErrorStatus& status, IFunctor<void(ErrorCode, StringView, SourceInfo)>* logger = null) noexcept: mStatus(&status), Logger(logger) {}

	constexpr void Error(ErrorCode errorCode, StringView desc, SourceInfo srcInfo = SourceInfo()) noexcept
	{
		if(Logger) (*Logger)(errorCode, desc, srcInfo);
		if(mStatus) mStatus->Error(errorCode, srcInfo);
	}
};

/** If an error was not handled before this object goes out of scope causes fatal error.
  Use this instead of ErrorStatus when you want to force error handling.
*/
struct FatalErrorStatus: ErrorStatus
{
	~FatalErrorStatus() noexcept(false)
	{
		if(mState != State::Error) return;
		INTRA_DEBUGGER_BREAKPOINT;
		char bufferChars[8192]; // avoid dynamic memory allocation (Intra/Core module restriction)
		auto buffer = Span(bufferChars);
		const auto errorCodeDesc = mErrorCode.ToString(buffer);
		const auto msg = BuildDiagnosticMessage(buffer, "ERROR",
			StringView(mSourceInfo.Function), StringView(mSourceInfo.File), mSourceInfo.Line, errorCodeDesc);
		// May throw, for example Intra::Test framework replaces termination with exception to be able to run other tests.
		// So we explicitly permit exception throwing for this destructor.
		FatalErrorCallback()(msg, mSourceInfo);
	}
};
INTRA_END

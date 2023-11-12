#pragma once

#include <Intra/Functional.h>
#include <Intra/Range/StringView.h>
#include <Intra/Platform/Toolchain.h>
#include <Intra/Container/Compound.h>
#include "IntraX/System/Debug.h"


#ifdef _WIN32
#define INTRAZ_D_CODE_(name, linux, bsd, win, winapi) 0x80000000|winapi
//#define INTRAZ_D_CODE_(name, linux, bsd, win, winapi) 0x80000000|win
#elif defined(__linux__)
#define INTRAZ_D_CODE_(name, linux, bsd, win, winapi) 0x80000000|linux
#elif defined(__APPLE__) || defined(__FreeBSD__)
#define INTRAZ_D_CODE_(name, linux, bsd, win, winapi) 0x80000000|bsd
#else
#define INTRAZ_D_CODE_(name, linux, bsd, win, winapi) 0x80000000|name
#endif

//#define INTRA_DEBUG_ERROR_CODES // define to check the codes against <errno.h>
#ifdef INTRA_DEBUG_ERROR_CODES
#include <errno.h>
#ifndef ETIME
#define ETIME ETIMEDOUT
#endif
#define INTRAZ_D_CODE(name, linux, bsd, win, winapi) []{constexpr auto res = INTRAZ_D_CODE_(name, linux, bsd, win, winapi); \
	static_assert(!name || res >= 0x80001000 || res == (0x80000000|name)); return res;}()
#else
#define INTRAZ_D_CODE INTRAZ_D_CODE_
#endif

namespace Intra { INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_MOVE_IMPLICITLY_DELETED
/** Contains error code, which value depends on a platform.
  Main considerations:
  1) Values in range [0; 0x7FFFFFFF) are never used as error codes and must be interpreted as successful result. This allows to implement things like LResult
  2) Enumerate only errors that are going to be used by Intra not only with system calls. Platform-specific errors not enumerated here can still be stored in ErrorCode and converted to string.
  3) Choose platfrom-specific codes that provide the closest descriptions even if they don't match the operation. This allows to avoid introducing own codes with description strings in the binary.
  4) On Windows errors are mostly encoded as HRESULT errors in range [0x80000000, 0x8FFFFFFF]. For some errors Intra uses POSIX variants that provide better descriptions.
  5) On POSIX platforms its value is 0x80000000 | errno or 0x80010000 | <getaddrinfo error>
  6) Custom error code range [0xA0000000; 0xFFFFFFFF] can be used by the user

  Use it to store errors only. For example avoid storing HTTP 100-399 success codes in ErrorCode. They should map to 0.

  The best way to create globally unique custom error codes is to determine how much errors your project has,
  then reserve log2 of this number lower bits and generate random value for higher bits.
  For example, if your project needs 128 <= n <= 256 custom errors, you can generate a random 23-bit (= 31-8) number and use it as a prefix for your error codes.
*/
struct ErrorCode
{
	enum I: uint32
	{
		NoError = 0,

		OperationNotPermitted = INTRAZ_D_CODE(EPERM, 1, 1, 1, 0x70005),
		FileNotFound = INTRAZ_D_CODE(ENOENT, 2, 2, 2, 0x70002),
		OperationInterrupted = INTRAZ_D_CODE(EINTR, 4, 4, 4, 0x72714),
		IOError = INTRAZ_D_CODE(EIO, 5, 5, 5, 0x7045D),
		InvalidHandle = INTRAZ_D_CODE(EBADF, 9, 9, 9, 0x70006),
		NotEnoughMemory = INTRAZ_D_CODE(ENOMEM, 12, 12, 12, 0x70008),
		PermissionDenied = INTRAZ_D_CODE(EACCES, 13, 13, 13, 0x70005),
		FileBusy = INTRAZ_D_CODE(EBUSY, 16, 16, 16, 0x700AA),
		AlreadyExists = INTRAZ_D_CODE(EEXIST, 17, 17, 17, 0x700B7),
		NotADirectory = INTRAZ_D_CODE(ENOTDIR, 20, 20, 20, 0x7010B),
		IsADirectory = INTRAZ_D_CODE(EISDIR, 21, 21, 21, 0x70050),
		InvalidArgument = INTRAZ_D_CODE(EINVAL, 22, 22, 22, 0x70057),
		TooManyOpenFiles = INTRAZ_D_CODE(EMFILE, 24, 24, 24, 0x70004),
		TextFileBusy = INTRAZ_D_CODE(ETXTBSY, 26, 26, 139, 139),
		NoSpaceLeft = INTRAZ_D_CODE(ENOSPC, 28, 28, 28, 0x70027),
		IllegalSeek = INTRAZ_D_CODE(ESPIPE, 29, 29, 29, 0x70006),
		ReadOnlyFS = INTRAZ_D_CODE(EROFS, 30, 30, 30, 0x70013),
		BrokenPipe = INTRAZ_D_CODE(EPIPE, 32, 32, 32, 0x700E9),
		Deadlock = INTRAZ_D_CODE(EDEADLK, 35, 11, 36, 36),
		NameTooLong = INTRAZ_D_CODE(ENAMETOOLONG, 36, 63, 38, 0x7006F),
		NotImplemented = INTRAZ_D_CODE(ENOSYS, 38, 78, 40, 0x4001),
		NotEmpty = INTRAZ_D_CODE(ENOTEMPTY, 39, 66, 41, 0x70091),
		WaitTimeout = INTRAZ_D_CODE(ETIME, 62, 60, 137, 0x70102),
		ProtocolError = INTRAZ_D_CODE(EPROTO, 71, 92, 134, 134),
		IllegalCharacter = INTRAZ_D_CODE(EILSEQ, 82, 86, 42, 0x70246),
		NetworkIsDown = INTRAZ_D_CODE(ENETDOWN, 100, 50, 116, 0x72742),
		NetworkIsUnreachable = INTRAZ_D_CODE(ENETUNREACH, 101, 51, 118, 0x72743),
		NetworkReset = INTRAZ_D_CODE(ENETRESET, 102, 52, 117, 0x72744),
		InsufficientBuffer = INTRAZ_D_CODE(ENOBUFS, 105, 55, 119, 119),
		ConnectionTimeout = INTRAZ_D_CODE(ETIMEDOUT, 110, 60, 138, 0x7274C),
		ConnectionRefused = INTRAZ_D_CODE(ECONNREFUSED, 111, 61, 107, 0x7274D),
		ConnectionReset = INTRAZ_D_CODE(ECONNRESET, 112, 54, 108, 0x72746),
		OperationCanceled = INTRAZ_D_CODE(ECANCELED, 125, 85, 105, 0x72777),
		TryAgain = INTRAZ_D_CODE(EWOULDBLOCK, 11, 35, 11, 10035), // WSAEWOULDBLOCK: A non-blocking socket operation could not be completed immediately.

		DataCorruption = INTRAZ_D_CODE(0, 74, 97, 104, 0x70017), // ERROR_CRC (WinAPI), EINTEGRITY (*BSD), EBADMSG (other POSIX platforms)

		// Windows-specific errors that map to POSIX errors defined above with similar descriptions
		InvalidDeviceName = INTRAZ_D_CODE(ENOENT, 2, 2, 2, 0x704B0),
		ElementNotFound = INTRAZ_D_CODE(ENOENT, 2, 2, 2, 0x70490),
		PathNotFound = INTRAZ_D_CODE(ENOENT, 2, 2, 2, 0x70003),
		DeviceNotReady = INTRAZ_D_CODE(EAGAIN, 11, 35, 11, 0x72733),
		FileReadOnly = INTRAZ_D_CODE(EACCES, 13, 13, 13, 0x71779), // ERROR_FILE_READ_ONLY: The specified file is read only.
		ObjectNameExists = INTRAZ_D_CODE(EEXIST, 17, 17, 17, 0x702BA), // ERROR_OBJECT_NAME_EXISTS: An attempt was made to create an object and the object name already existed.
		BadCommand = INTRAZ_D_CODE(EINVAL, 22, 22, 22, 0x70016),
		UnsupportedType = INTRAZ_D_CODE(EINVAL, 22, 22, 22, 0x7065E), // ERROR_UNSUPPORTED_TYPE: Data of this type is not supported.
		RequestNotSupported = INTRAZ_D_CODE(ENOSYS, 38, 78, 40, 0x70032),
		UnsupportedCompression = INTRAZ_D_CODE(ENOSYS, 38, 78, 40, 0x7026A), // UNSUPPORTED_COMPRESSION: The specified compression format is unsupported.
		AssertionFailure = 0x8007029C, // ERROR_ASSERTION_FAILURE: An assertion failure has occurred.
		UnknownError = 0x80004005, // E_FAIL: Unspecified error
#undef INTRAZ_D_CODE
#undef INTRAZ_D_CODE_

		UndeclaredIdentifier = 0x9F000000,
		UnexpectedToken = 0x9F000001,
		TypeError = 0x9F000002,

		TagErrorCode // allows ErrorCode to implicitly construct from this enum
	};

	I Value = 0;
	ErrorCode() = default;
	template<CEnum E> requires requires {E::TagErrorCode;}
	INTRA_FORCEINLINE constexpr ErrorCode(E x): Value(I(uint32(x))) {}

	static INTRA_FORCEINLINE ErrorCode LastError()
	{
#ifdef _WIN32
		return {z_D::GetLastError()|0x80070000};
#else
		return {z_D::GetErrno()|0x80000000};
#endif
	}

	static INTRA_FORCEINLINE constexpr ErrorCode Wrap(int err) {return {err|0x80000000};}
	static INTRA_FORCEINLINE constexpr ErrorCode WrapGAI(int err) {
#ifdef _WIN32
		return {err|0x80070000};
#else
		return {err|0x80010000};
#endif
	}
	static INTRA_FORCEINLINE constexpr ErrorCode Wrap(ErrorCode err) {return err;}

	[[nodiscard]] INTRA_FORCEINLINE constexpr explicit operator bool() const noexcept {return Value >> 31;}

	/** Convert the error code to its string representation using platform specific API and default locale.
	  For custom values prints "ErrorCode(<Value (hex)>)" if no corresponsding range is found.
	  Advances ``dstBuf`` with number of written characters, crops result if the provided buffer doesn't have enough size.
	  @return StringView referring to written result in provided buffer.
	*/
	[[nodiscard]] StringView ToString(Span<char>& dstBuf) const noexcept
	{
#ifdef _WIN32
		if((Value >> 16) != 0x8000)
		{
			// NOTE: if not enough space in buffer, returns empty string, so make sure to provide big enough buffer (200+ is OK for most errors)
			auto res = dstBuf|Take(z_D::FormatMessageA(0x12FF, nullptr, Value, 0x409, dstBuf.Begin, dstBuf.size(), nullptr));
			dstBuf.Begin = res.End;
			return res;
		}
#else
		if((Value >> 16) == 0x8001)
		{
			const auto errStr = z_D::gai_strerror(Value & 0xFFFF);
			const auto resBegin = dstBuf.Begin;
			Span(errStr, z_D::strlen(errStr))|WriteTo(dstBuf);
			return StringView(Unsafe, resBegin, dstBuf.Begin);
		}
#endif
		// NOTE: if not enough space in buffer, returns truncated string
		z_D::strerror_s(dstBuf.Begin, dstBuf.size(), Value & 0xFFFF); // locale-specific, but on Windows always English
		const auto resBegin = dstBuf.Begin;
		dstBuf.Begin += z_D::strlen(res);
		return StringView(Unsafe, resBegin, dstBuf.Begin);
	}
};

#define INTRA_RETURN_ON_ERROR(...) {if(auto res_ = __VA_ARGS__; ErrorCode(res_) != ErrorCode::NoError) return ErrorCode(res_);}

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
	enum class State: uint8 {Ok, Error, HandledError};
	State mState = State::Ok;
	ErrorCode mErrorCode;
	SourceInfo mSourceInfo{};
public:
	// ErrorStatus should only be allocated on the stack.
	// Heap allocation does not make any sense.
	void* operator new(size_t) = delete;
	void operator delete(void*) = delete;

	ErrorStatus() = default;
	explicit constexpr ErrorStatus(ErrorCode errorCode = ErrorCode::LastError(), SourceInfo srcInfo = SourceInfo::Current()) noexcept {Error(errorCode, srcInfo);}

	ErrorStatus(const ErrorStatus&) = delete;
	ErrorStatus& operator=(const ErrorStatus&) = delete;

	constexpr void Error(ErrorCode errorCode = ErrorCode::LastError(), SourceInfo srcInfo = SourceInfo::Current()) noexcept
	{
		if(mState == State::Error || !errorCode) return; //If there was an unhandled error we won't overwrite it
		mState = State::Error;
		mErrorCode = errorCode;
		if constexpr(!Config::DisableTroubleshooting)
			mSourceInfo = srcInfo;
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr bool WasError() const noexcept {return mState >= State::Error;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr bool WasUnhandledError() const noexcept {return mState == State::Error;}

	/**
	  @return ErrorCode for the very first unhandled error or 0.
	  Marks the error as handled.
	*/
	[[nodiscard]] INTRA_FORCEINLINE constexpr ErrorCode Handle() noexcept
	{
		if(mState != State::Error) return {};
		mState = State::HandledError;
		return mErrorCode;
	}

	/// Makes last error unhandled again.
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
	ErrorStatus* mStatus = nullptr;
public:
	using ILogger = ICallback<void(ErrorCode, StringView, SourceInfo)>;
	ILogger* Logger = nullptr;

	constexpr ErrorReporter(decltype(IgnoreErrors), ILogger* logger = nullptr) noexcept: Logger(logger) {}
	constexpr ErrorReporter(ErrorStatus& status, ILogger* logger = nullptr) noexcept: mStatus(&status), Logger(logger) {}

	constexpr void Error(ErrorCode errorCode = ErrorCode::LastError(), StringView desc = {}, SourceInfo srcInfo = SourceInfo::Current()) noexcept
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
	using ErrorStatus::ErrorStatus;
	constexpr ~FatalErrorStatus() noexcept(false)
	{
		if(mState != State::Error) return;
		INTRA_DEBUGGER_BREAKPOINT;
		char bufferChars[8192]; // avoid dynamic memory allocation (Intra/Core module restriction)
		auto buffer = Span(bufferChars);
		const auto errorCodeDesc = mErrorCode.ToString(buffer);
		const auto msg = BuildDiagnosticMessage(buffer, "UNHANDLED ERROR", errorCodeDesc, mSourceInfo);
		// May throw, for example Intra::Test framework replaces termination with exception to be able to run other tests.
		// So we explicitly permit exception throwing for this destructor.
		FatalErrorCallback()(msg, mSourceInfo);
	}
};

#define INTRA_EXPECT_NO_ERROR(...) FatalErrorStatus(ErrorCode::Wrap(__VA_ARGS__))
#ifdef INTRA_DEBUG
#define INTRA_DEBUG_EXPECT_NO_ERROR(...) INTRA_EXPECT_NO_ERROR
#else
#define INTRA_DEBUG_EXPECT_NO_ERROR(...) (__VA_ARGS__)
#endif

template<typename T> class Result
{
	union
	{
		T mValue;
		char mDummy;
	};
	ErrorCode mError{};
public:
	INTRA_FORCEINLINE constexpr Result(ErrorCode c) noexcept: mError(c) {}
	template<typename... Args> INTRA_FORCEINLINE constexpr Result(Args&&... c) noexcept requires CConstructible<T, Args...>: mValue(c) {}
	
	[[nodiscard]] INTRA_FORCEINLINE constexpr ErrorCode Error() const {return mError;}

	///@{
	/// @return contained object. Must be called only if no error!
	[[nodiscard]] constexpr T& Unwrap()
	{
		INTRA_PRECONDITION(!Error());
		return mValue;
	}

	[[nodiscard]] constexpr const T& Unwrap() const
	{
		INTRA_PRECONDITION(!Error());
		return mValue;
	}
	///@}

	[[nodiscard]] INTRA_FORCEINLINE constexpr explicit operator bool() const {return !Error();}

	[[nodiscard]] INTRA_FORCEINLINE constexpr T* Data() {return AddressOf(mValue);}
	[[nodiscard]] INTRA_FORCEINLINE constexpr const T* Data() const {return AddressOf(mValue);}
	[[nodiscard]] INTRA_FORCEINLINE constexpr index_t Length() const {return bool(*this)? 1: 0;}
};

// More light-weight version. Preferable in the following cases:
// 1. ARM32: sizeof(T) <= 4
// 2. x86: sizeof(T) == 8
// 3. MSVC x64: sizeof(T) == 8
template<CBasicIntegral T> class LResult
{
	using TT = TIntOfSizeAtLeast<Max(sizeof(T), sizeof(ErrorCode)), CBasicSigned<T>>;
	TT mValueOrErrorCode: SizeofInBits<TT> - 1;
	TT mIsError: 1;
public:
	template<CEnum E> requires CConstructible<ErrorCode, E>
	INTRA_FORCEINLINE constexpr LResult(E c) noexcept: mValueOrErrorCode(TT(uint32(c))), mIsError(true) {}
	INTRA_FORCEINLINE constexpr LResult(T value) noexcept: mValueOrErrorCode(value), mIsError(false) {}

	[[nodiscard]] INTRA_FORCEINLINE constexpr bool IsError() const {return mIsError;}

	[[nodiscard]] INTRA_FORCEINLINE constexpr ErrorCode Error() const
	{
		return mIsError? uint32(mValueOrErrorCode)|0x80000000: ErrorCode::NoError;
	}

	/// @return contained object. Must be called only if no error!
	[[nodiscard]] constexpr T Unwrap() const
	{
		INTRA_PRECONDITION(!mIsError);
		return T(mValueOrErrorCode);
	}

	[[nodiscard]] constexpr T Or(T fallbackValue) const
	{
		return mIsError? fallbackValue: T(mValueOrErrorCode);
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr explicit operator bool() const {return !mIsError;}
};

} INTRA_END

#pragma once

#include "Cpp/Warnings.h"
#include "Utils/Logger.h"
#include "IO/FormattedWriter.h"
#include "Math/Vector3.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace IO {

class FormattedLogger: public ILogger
{
public:
	forceinline explicit FormattedLogger(FormattedWriter writer=null) noexcept: Writer(Cpp::Move(writer)) {}
	FormattedLogger(const FormattedLogger&) = delete;
	FormattedLogger(FormattedLogger&&) = default;
	FormattedLogger& operator=(const FormattedLogger&) = delete;
	FormattedLogger& operator=(FormattedLogger&&) = default;

	void Log(LogLevel level, StringView msg, const Utils::SourceInfo& srcInfo) override;

	FormattedWriter Writer;
	LogLevel Verbosity = LogLevel::All;
	Math::Vec3 InfoColor = {0.45f, 0.45f, 0.45f};
	Math::Vec3 SuccessColor = {0, 0.6f, 0};
	Math::Vec3 PerfWarningColor = {0.55f, 0.55f, 0.2f};
	Math::Vec3 WarningColor = {0.9f, 0.75f, 0.2f};
	Math::Vec3 ErrorColor = {1, 0.15f, 0.1f};
	Math::Vec3 CriticalErrorColor = {0.8f, 0, 0};
	bool WriteLevelType = true;
};

}}

INTRA_WARNING_POP

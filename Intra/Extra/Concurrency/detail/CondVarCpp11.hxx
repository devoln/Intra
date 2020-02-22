#include "Concurrency/CondVar.h"

#include <condition_variable>
#include <chrono>
#include <ctime>

INTRA_BEGIN
namespace Concurrency {

SeparateCondVar::SeparateCondVar()
{
	static_assert(sizeof(mData) >= sizeof(std::condition_variable), "Invalid DATA_SIZE in CondVar.h!");
	new(mData) std::condition_variable;
}

SeparateCondVar::~SeparateCondVar() {reinterpret_cast<std::condition_variable*>(mData)->~condition_variable();}

bool SeparateCondVar::wait(Mutex& mutex)
{
	std::unique_lock<std::mutex> lck(*reinterpret_cast<std::mutex*>(mutex.mData), std::adopt_lock);
	reinterpret_cast<std::condition_variable*>(mData)->wait(lck);
	lck.release();
	return true;
}

static std::chrono::system_clock::time_point absTimeToTimePoint(uint64 msecs)
{
	return std::chrono::system_clock::from_time_t(std::time_t(msecs / 1000)) + std::chrono::milliseconds(msecs % 1000);
}

bool SeparateCondVar::waitUntil(Mutex& mutex, uint64 absTimeMs)
{
	std::unique_lock<std::mutex> lck(*reinterpret_cast<std::mutex*>(mutex.mData), std::adopt_lock);
	const bool result = reinterpret_cast<std::condition_variable*>(mData)->wait_until(lck, absTimeToTimePoint(absTimeMs)) == std::cv_status::no_timeout;
	lck.release();
	return result;
}

void SeparateCondVar::Notify() {reinterpret_cast<std::condition_variable*>(mData)->notify_one();}
void SeparateCondVar::NotifyAll() {reinterpret_cast<std::condition_variable*>(mData)->notify_all();}

}}

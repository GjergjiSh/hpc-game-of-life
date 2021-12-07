// from https://www.learncpp.com/cpp-tutorial/timing-your-code/ with a bit of RAII
#include "common/Timing.h"

TimedScope::TimedScope(char* timer_name): start(clock_type::now()), name(timer_name) {};

double TimedScope::elapsed() const noexcept {
	return std::chrono::duration_cast<measurement_accuracy>(clock_type::now() - start).count();
}

TimedScope::~TimedScope() {
	auto time = elapsed();
	std::cout << "Timer \"" << name << "\" stopped after: " << time << std::endl;
};

// from https://www.learncpp.com/cpp-tutorial/timing-your-code/ with a bit of RAII

#include <chrono> // for std::chrono functions
#include <iostream>

class TimedScope
// Typical usage:
// [preparations]
// {
//   Timer started();
//   [code to time];
//   //destructor gets called at the end of scope and prints time it took
// }
{
    
private:
	// Type aliases to make accessing nested type easier
	using clock_type = std::chrono::steady_clock;
	using measurement_accuracy = std::chrono::milliseconds;

	std::chrono::time_point<clock_type> start;
	char* name;

public:
    TimedScope(char* timer_name);
    double elapsed() const noexcept;
    ~TimedScope();
};
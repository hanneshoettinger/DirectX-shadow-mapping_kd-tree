#pragma once
#include <vector>

struct time
{
	int hours = 0;
	int minutes = 0;
	int seconds = 0;
	int milliseconds = 0;

	time operator+(time rhs)
	{
		this->hours += rhs.hours;
		this->minutes += rhs.minutes;
		this->seconds += rhs.seconds;
		this->milliseconds += rhs.milliseconds;
		return (*this);
	}
	time operator/(int factor)
	{
		this->hours = this->hours/factor;
		this->minutes = this->minutes / factor;
		this->seconds = this->seconds / factor;
		this->milliseconds = this->milliseconds / factor;
		return (*this);
	}
};
class TimerFunc {
public:
	TimerFunc();
	~TimerFunc();
	void startTimer();
	void stopTimer();
private:
	time times;
	time calculateTimeAverage(std::vector<time> times);
	LARGE_INTEGER frequency;        // ticks per second
	LARGE_INTEGER t1, t2;           // ticks
	double elapsedTime;
	std::vector<time>  Timesnth;
	std::vector<time>  Timesqs;
	std::vector<time>  Timesrand;
	std::vector<time>  Timesmedian;
};
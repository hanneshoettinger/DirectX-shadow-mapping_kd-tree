#pragma once
#include "pch.h"
#include "Timer.h"
#include <stdio.h>

TimerFunc::TimerFunc()
{
}

TimerFunc::~TimerFunc()
{
}

void TimerFunc::startTimer()
{
	// get ticks per second
	QueryPerformanceFrequency(&frequency);

	// start timer
	QueryPerformanceCounter(&t1);
}

void TimerFunc::stopTimer()
{
	// stop timer
	QueryPerformanceCounter(&t2);

	// compute the elapsed time
	elapsedTime = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;

	int milliseconds = (int)(elapsedTime) % 1000;
	int seconds = (int)(elapsedTime / 1000) % 60;
	int minutes = (int)(elapsedTime / (1000 * 60)) % 60;
	int hours = (int)(elapsedTime / (1000 * 60 * 60)) % 24;

	char buffer[50];
	sprintf(buffer, "%.2d:%.2d:%.2d.%.3d; \n", hours, minutes, seconds, milliseconds);
	OutputDebugStringA(buffer);

	elapsedTime = 0;
}

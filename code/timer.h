
// indigolaunch -- (Windows) multimedia timer

#pragma once

class Timer
{
public:
	Timer() :
		m_isHighFreq(QueryPerformanceFrequency(&m_pcFrequency) != 0)
	{
		Reset();
	}

	void Reset()
	{
		if (!m_isHighFreq)
		{
			m_offset.LowPart = GetTickCount();
			m_oneOverFreq = 0.001f;
		}
		else
		{
			VERIFY(QueryPerformanceCounter(&m_offset));
			m_oneOverFreq = 1.f / (float) m_pcFrequency.QuadPart;
		}
	}

	float Get() const 
	{
		if (!m_isHighFreq)
		{
			return (float) (GetTickCount() - m_offset.LowPart) * m_oneOverFreq;
		}
		else
		{
			LARGE_INTEGER curCount;
			VERIFY(QueryPerformanceCounter(&curCount)); 
			return (float) (curCount.QuadPart - m_offset.QuadPart) * m_oneOverFreq;
		}
	}

private:
	const bool m_isHighFreq;
	LARGE_INTEGER m_pcFrequency; 
	LARGE_INTEGER m_offset;
	float m_oneOverFreq;
};

class AnimTimer
{
public:
	AnimTimer() : m_isRunning(false), m_time(0.f) {}
	~AnimTimer() {}
	
	void Run(float time)
	{
		m_timer.Reset();
		m_isRunning = true;
		m_time = time;
	}
	
	bool IsRunning()
	{
		if (m_isRunning)
		{
			if (m_timer.Get() < m_time)
			{
				return true;
			}
			else
			{
				m_isRunning = false;
				return false;
			}
		}
	
		return false;
	}

	// returns normalized time [0, 1]
	float Get() { return IsRunning() ? m_timer.Get()/m_time : 0.f; }
	
private:
	Timer m_timer;
	bool m_isRunning;
	float m_time;
};

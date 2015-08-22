
// indigolaunch -- utilities

#pragma once

// macros
#define SAFE_RELEASE(pX) if (nullptr != (pX)) (pX)->Release()

#ifdef _DEBUG
	#define ASSERT(condition) if (!(condition)) __debugbreak();
	#define VERIFY(condition) ASSERT(condition)
#else
	#define ASSERT(condition)
	#define VERIFY(condition) (condition)
#endif

// serialize constant value to std::wstring (FIXME: phase out)
template<typename T> inline std::wstring ToString(const T &X)
{
	std::wstringstream strStream;
	VERIFY((strStream << X));
	return strStream.str();
}

// interpolation trick(s)
inline float SmoothStep(float x) { return x*x*(3.f - 2.f*x); }

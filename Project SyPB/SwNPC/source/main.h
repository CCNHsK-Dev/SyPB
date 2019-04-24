
#ifndef MAIN_H
#define MAIN_H

#include <windows.h>
#include <sys/types.h>	
#include <string.h>
#include <malloc.h>
#include "amxxmodule.h"
#include <../../version.h>
#include "extdll.h"

// Error Log
int LogToFile(char *szLogText, ...);
void ErrorWindows(char *text);

extern const char *GetModName(void);

#define PTR_TO_INT(in) *(int *) (in)

enum LineType
{
	LINE_SIMPLE,
	LINE_ARROW
};

class Color
{
	//
	// Group: Red, green, blue and alpha (controls transparency (0 - transparent, 255 - opaque)) color components (0 - 255).
	//
public:
	int red, green, blue, alpha;

	//
	// Group: (Con/De)structors.
	//
public:
	inline Color(int color = 0) : red(color), green(color), blue(color), alpha(color)
	{

	}

	inline Color(int inputRed, int inputGreen, int inputBlue, int inputAlpha = 0) : red(inputRed), green(inputGreen), blue(inputBlue), alpha(inputAlpha)
	{
	}

	inline Color(const Color &right) : red(right.red), green(right.green), blue(right.blue), alpha(right.alpha)
	{
	}

	//
	// Group: Operators.
	//
public:
	// equality
	inline bool operator == (const Color &right) const
	{
		return red == right.red && green == right.green && blue == right.blue && alpha == right.alpha;
	}

	inline bool operator != (const Color &right) const
	{
		return !operator == (right);
	}

	// array access
	inline int &operator [] (int colourIndex)
	{
		return (&red)[colourIndex];
	}

	inline const int &operator [] (int colourIndex) const
	{
		return (&red)[colourIndex];
	}

	inline const Color operator / (int scaler) const
	{
		return Color(red / scaler, green / scaler, blue / scaler, alpha / scaler);
	}
};

template <typename T> class Singleton
{
protected:
	inline Singleton(void)
	{
	}

	virtual inline ~Singleton(void)
	{
	}


public:
	static inline T &GetReference(void)
	{
		static T reference;

		return reference;
	}

	static inline T *GetObjectPtr(void)
	{
		return &GetReference();
	}
};

#endif // MAIN_H
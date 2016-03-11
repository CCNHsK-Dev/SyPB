/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#ifndef VECTOR_H
#define VECTOR_H

//=========================================================
// 2DVector - used for many pathfinding and many other 
// operations that are treated as planar rather than 3d.
//=========================================================
class Vector2D
{
public:
	inline Vector2D(void)									{ }
	inline Vector2D(float X, float Y)						{ x = X; y = Y; }
	inline Vector2D operator+(const Vector2D& v)	const	{ return Vector2D(x+v.x, y+v.y);	}
	inline Vector2D operator-(const Vector2D& v)	const	{ return Vector2D(x-v.x, y-v.y);	}
	inline Vector2D operator*(float fl)				const	{ return Vector2D(x*fl, y*fl);	}
	inline Vector2D operator/(float fl)				const	{ return Vector2D(x/fl, y/fl);	}
	
	inline float Length(void)						const	{ return sqrt(x*x + y*y );		}

	inline Vector2D Normalize ( void ) const
	{
		Vector2D vec2;

		float flLen = Length();
		if ( flLen == 0 )
		{
			return Vector2D( 0, 0 );
		}
		else
		{
			flLen = 1 / flLen;
			return Vector2D( x * flLen, y * flLen );
		}
	}

	vec_t	x, y;
};

inline float DotProduct(const Vector2D& a, const Vector2D& b) { return( a.x*b.x + a.y*b.y ); }
inline Vector2D operator*(float fl, const Vector2D& v)	{ return v * fl; }


namespace Math
{
	const float MATH_ONEPSILON = 0.01f;
	const float MATH_EQEPSILON = 0.001f;
	const float MATH_FLEPSILON = 1.192092896e-07f;

	const float MATH_PI = 3.1415926f;

	const float MATH_D2R = MATH_PI / 180.0f;
	const float MATH_R2D = 180.0f / MATH_PI;

	inline bool FltZero(float entry)
	{
		return fabsf(entry) < MATH_ONEPSILON;
	}

	inline bool FltEqual(float entry1, float entry2)
	{
		return fabsf(entry1 - entry2) < MATH_EQEPSILON;
	}

	inline float RadianToDegree(float radian)
	{
		return radian * MATH_R2D;
	}

	inline float DegreeToRadian(float degree)
	{
		return degree * MATH_D2R;
	}

	inline float AngleMod(float angle)
	{
		return 360.0f / 65536.0f * (static_cast <int> (angle * (65536.0f / 360.0f)) & 65535);
	}

	inline float AngleNormalize(float angle)
	{
		return 360.0f / 65536.0f * (static_cast <int> ((angle + 180.0f) * (65536.0f / 360.0f)) & 65535) - 180.0f;
	}

	void inline SineCosine(float radians, float &sine, float &cosine)
	{
#if defined (PLATFORM_WIN32)
		__asm
		{
			fld uint32_t ptr[radians]
			fsincos

				mov edx, uint32_t ptr[cosine]
				mov eax, uint32_t ptr[sine]

				fstp uint32_t ptr[edx]
				fstp uint32_t ptr[eax]
		}
#else
		sine = sinf(radians);
		cosine = cosf(radians);
#endif
	}
}

//=========================================================
// 3D Vector
//=========================================================
class Vector						// same data-layout as engine's vec3_t,
{								//		which is a vec_t[3]
public:
	// Construction/destruction
	inline Vector(void)								{ }
	inline Vector(float X, float Y, float Z)		{ x = X; y = Y; z = Z;						}
	//inline Vector(double X, double Y, double Z)		{ x = (float)X; y = (float)Y; z = (float)Z;	}
	//inline Vector(int X, int Y, int Z)				{ x = (float)X; y = (float)Y; z = (float)Z;	}
	inline Vector(const Vector& v)					{ x = v.x; y = v.y; z = v.z;				} 
	inline Vector(float rgfl[3])					{ x = rgfl[0]; y = rgfl[1]; z = rgfl[2];	}

	// Operators
	inline Vector operator-(void) const				{ return Vector(-x,-y,-z);				}
	inline int operator==(const Vector& v) const	{ return x==v.x && y==v.y && z==v.z;	}
	inline int operator!=(const Vector& v) const	{ return !(*this==v);					}
	inline Vector operator+(const Vector& v) const	{ return Vector(x+v.x, y+v.y, z+v.z);	}
	inline Vector operator-(const Vector& v) const	{ return Vector(x-v.x, y-v.y, z-v.z);	}
	inline Vector operator*(float fl) const			{ return Vector(x*fl, y*fl, z*fl);		}
	inline Vector operator/(float fl) const			{ return Vector(x/fl, y/fl, z/fl);		}
	
	// Methods
	inline void CopyToArray(float* rgfl) const		{ rgfl[0] = x, rgfl[1] = y, rgfl[2] = z; }
	inline float Length(void) const					{ return sqrt(x*x + y*y + z*z); }
	operator float *()								{ return &x; } // Vectors will now automatically convert to float * when needed
	operator const float *() const					{ return &x; } // Vectors will now automatically convert to float * when needed
	inline Vector Normalize(void) const
	{
		float flLen = Length();
		if (flLen == 0) return Vector(0,0,1); // ????
		flLen = 1 / flLen;
		return Vector(x * flLen, y * flLen, z * flLen);
	}

	inline Vector2D Make2D ( void ) const
	{
		Vector2D	Vec2;

		Vec2.x = x;
		Vec2.y = y;

		return Vec2;
	}
	inline float Length2D(void) const					{ return sqrt(x*x + y*y); }

	inline void BuildVectors(Vector *forward, Vector *right, Vector *upward) const
	{
		float sinePitch = 0.0f, cosinePitch = 0.0f, sineYaw = 0.0f, cosineYaw = 0.0f, sineRoll = 0.0f, cosineRoll = 0.0f;

		Math::SineCosine(Math::DegreeToRadian(x), sinePitch, cosinePitch);	// compute the sine and cosine of the pitch component
		Math::SineCosine(Math::DegreeToRadian(y), sineYaw, cosineYaw); // compute the sine and cosine of the yaw component
		Math::SineCosine(Math::DegreeToRadian(z), sineRoll, cosineRoll); // compute the sine and cosine of the roll component

		if (forward != NULL)
		{
			forward->x = cosinePitch * cosineYaw;
			forward->y = cosinePitch * sineYaw;
			forward->z = -sinePitch;
		}

		if (right != NULL)
		{
			right->x = -sineRoll * sinePitch * cosineYaw + cosineRoll * sineYaw;
			right->y = -sineRoll * sinePitch * sineYaw - cosineRoll * cosineYaw;
			right->z = -sineRoll * cosinePitch;
		}

		if (upward != NULL)
		{
			upward->x = cosineRoll * sinePitch * cosineYaw + sineRoll * sineYaw;
			upward->y = cosineRoll * sinePitch * sineYaw - sineRoll * cosineYaw;
			upward->z = cosineRoll * cosinePitch;
		}
	}

	// Members
	vec_t x, y, z;
};
inline Vector operator*(float fl, const Vector& v)	{ return v * fl; }
inline float DotProduct(const Vector& a, const Vector& b) { return(a.x*b.x+a.y*b.y+a.z*b.z); }
inline Vector CrossProduct(const Vector& a, const Vector& b) { return Vector( a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x ); }


#endif
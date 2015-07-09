//
// mathlib.h
//

#ifndef MATHLIBRARY_H
#define MATHLIBRARY_H

// Use this definition globally
#define ON_EPSILON        0.01
#define EQUAL_EPSILON     0.001

#define FEqual(v1, v2) ( fabs(v1 - v2) < EQUAL_EPSILON )

#pragma warning (disable: 4239)

inline float LengthSquared(const Vector &vec)
{
   // squared length, no sqrt involved so faster
   return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
}

inline float UTIL_AngleMod(float a)
{
   // this function adds or substracts 360 enough times needed to the given angle in
   // order to set it into the range [0, 360) and returns the resulting angle. Letting
   // the engine have a hand on angles that are outside these bounds may cause the
   // game to freeze by screwing up the engine math code.

   return (360.0 / 65536) * ((int)(a * (65536.0 / 360.0)) & 65535);
}

inline float AngleNormalize(float angle)
{
   // this function adds or substracts 360 enough times needed to the given angle in
   // order to set it into the range [-180, 180) and returns the resulting angle. Letting
   // the engine have a hand on angles that are outside these bounds may cause the game
   // to freeze by screwing up the engine math code.

   return (360.0 / 65536) * ((int)((angle + 180) * (65536.0 / 360.0)) & 65535) - 180;
}

inline void ClampAngles(Vector &vecAngles)
{
   vecAngles.x = AngleNormalize(vecAngles.x);
   vecAngles.y = AngleNormalize(vecAngles.y);
   vecAngles.z = 0;
}

void inline SinCos( float rad, float *flSin, float *flCos )
{
#ifdef __GNUC__
   register double __cosr, __sinr;
   __asm __volatile__ ("fsincos" : "=t" (__cosr), "=u" (__sinr) : "0" (rad));
   *flSin = __sinr;
   *flCos = __cosr;
#else
#ifndef _MSC_VER
   *flSin = sin(rad);
   *flCos = cos(rad);
#else
   __asm
   {
      fld DWORD PTR[rad]
      fsincos
      mov edx, DWORD PTR[flCos]
      mov eax, DWORD PTR[flSin]
      fstp DWORD PTR[edx]
      fstp DWORD PTR[eax]
   }
#endif
#endif
}

inline float UTIL_AngleDiff( float destAngle, float srcAngle )
{
   return AngleNormalize(destAngle - srcAngle);
}

inline float UTIL_VecToYaw( const Vector &vec )
{
   // the purpose of this function is to convert a spatial location determined by the vector
   // passed in into an absolute Y angle (yaw) from the origin of the world.

   if (vec.x == 0.0 && vec.y == 0.0)
      return 0;
   else
      return atan2(vec.y, vec.x) * (180 / M_PI);
}

inline Vector UTIL_VecToAngles( const Vector &vec )
{
   // the purpose of this function is to convert a spatial location determined by the vector
   // passed in into absolute angles from the origin of the world.

   float yaw, pitch;

   if (vec.x == 0.0 && vec.y == 0.0)
   {
      yaw = 0.0;
      pitch = (vec.z > 0.0) ? 90 : 270;
   }
   else
   {
      yaw = atan2(vec.y, vec.x) * (180 / M_PI);
      pitch = atan2(vec.z, vec.Length2D()) * (180 / M_PI);
   }

   return Vector(pitch, yaw, 0);
}

inline void UTIL_MakeVectors( const Vector &vecAngles )
{
   float sp = 0, cp = 0, sy = 0, cy = 0, sr = 0, cr = 0;
   float angle = vecAngles.x * (M_PI / 180);
   SinCos(angle, &sp, &cp);
   angle = vecAngles.y * (M_PI / 180);
   SinCos(angle, &sy, &cy);
   angle = vecAngles.z * (M_PI / 180);
   SinCos(angle, &sr, &cr);

   gpGlobals->v_forward.x = cp * cy;
   gpGlobals->v_forward.y = cp * sy;
   gpGlobals->v_forward.z = -sp;
   gpGlobals->v_right.x = -sr * sp * cy + cr * sy;
   gpGlobals->v_right.y = -sr * sp * sy - cr * cy;
   gpGlobals->v_right.z = -sr * cp;
   gpGlobals->v_up.x = cr * sp * cy + sr * sy;
   gpGlobals->v_up.y = cr * sp * sy - sr * cy;
   gpGlobals->v_up.z = cr * cp;
}

#endif

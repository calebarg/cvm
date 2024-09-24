/////////////////////////////////////////////////////////////////////////////////////////
//  COPYRIGHT (c) 2024 Schweitzer Engineering Laboratories, Inc.
//  SEL Confidential
/////////////////////////////////////////////////////////////////////////////////////////
/// @file base_math.cpp
/// @author Caleb Barger 
/// @date August 16, 2024
/////////////////////////////////////////////////////////////////////////////////////////

struct Vec2_S8
{
   S8 x;
   S8 y;

   Vec2_S8 operator+(Vec2_S8 rhs)
   {
      return {x + rhs.x, y + rhs.y};
   }

   Vec2_S8 operator-(Vec2_S8 rhs)
   {
      return {x - rhs.x, y - rhs.y};
   }

   Vec2_S8 operator*(Vec2_S8 rhs)
   {
      return {x * rhs.x, y * rhs.y};
   }

   B8 operator==(Vec2_S8 rhs)
   {
      return (B8)((x == rhs.x) && (y == rhs.y));
   }

   B8 operator!=(Vec2_S8 rhs)
   {
      return (B8)((x != rhs.x) || (y != rhs.y));
   }
};

struct Vec3_U8
{
   U8 x;
   U8 y;
   U8 z;
};

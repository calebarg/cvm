//
// base_ascii.c
//
// Caleb Barger
// 03/21/2024
//

internal B8 is_alpha(U8 c)
{
   B8 result = 0;
   if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
      result = 1;
   return result;
}

internal B8 is_digit(U8 c)
{
   B8 result = 0;
   if (c >= '0' && c <= '9')
      result = 1;
   return result;
}

internal B8 is_alpha_num(U8 c)
{
   B8 result = 0;
   if (is_digit(c) || is_alpha(c))
      result = 1;
   return result;
}

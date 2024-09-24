////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2024 Schweitzer Engineering Laboratories, Inc.
//  SEL Confidential
////////////////////////////////////////////////////////////////////////////////////////////////////

internal void clear_line()
{
   printf("\x1b[2K");
}

internal void move_cursor(U32 row, U32 col)
{
   printf("\x1b[%u;%uH", row, col);
}

internal void set_color(U8 r, U8 g, U8 b)
{
   printf("\x1b[38;2;%u;%u;%um", r, g, b);
}

internal void hide_cursor()
{
   printf("\x1b[?25l");
}

internal void show_cursor()
{
   printf("\x1b[?25h");
}

internal void alternate_screen_buffer()
{
   printf("\x1b[?1049h");
}

internal void main_screen_buffer()
{
   printf("\x1b[?1049l");
}

internal void dec_character_set()
{
   printf("\x1b(0");
}

internal void ascii_character_set()
{
   printf("\x1b(B");
}

internal void enable_blink()
{
   printf("\x1b[5m");
}

internal void disable_blink()
{
   printf("\x1b[25m");
}

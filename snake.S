//
// snake.S 
//
// Caleb Barger
// 10/18/24
//

b start

display_update:
  mov w10, w2
  sub w10, w10, w1
  strb w1, [x10]
  ret

// BUG(calebarg): Using r28, and r29 
// as input registers was not working as expected
// could be a variety of issues.  

// w14 - snake row
// w15 - snake col
wrap_snake_p:
  cmp w14, 0x0     
  b.lt wrap_to_bottom 
  cmp w14, 0xa     
  b.ge wrap_to_top 
  cmp w15, 0x0
  b.lt wrap_to_right
	cmp w15, 0x14  
	b.ge wrap_to_left
	b wrap_done

wrap_to_bottom:
  mov w14, 0x9 
  b wrap_done
wrap_to_top:
  mov w14, 0x0 
  b wrap_done
wrap_to_right:
  mov w15, 0x13
  b wrap_done
wrap_to_left:
  mov w15, 0x0
wrap_done:
  ret

start:
  mov w0, 0x0
  mov w1, 0x1 

  // NOTE(calebarg): Display is 10 rows x 20 cols

  mov w2, #312 // Display memory start

  // Up    - 0x41 
  // Down  - 0x42
  // Left  - 0x44  
  // Right - 0x43 
  mov w3, 0x0 // Snake direction (This value is polled from mem) 
  mov w4, 0x0 // Snake head row 
  mov w5, 0x0 // Snake head col
  mov w6, 0x1 // Snake len 
  mov w9, 0x14 // Board cols
game_loop:

  // Read input byte (located 2 bytes before display memory start)
  mov w10, w2
  sub w10, w10, #2 
  ldrb w3, [x10]  

  cmp w3, 0x41  
  b.eq dir_case_up
  cmp w3, 0x42
  b.eq dir_case_down
  cmp w3, 0x44
  b.eq dir_case_left
  // Otherwise assume right. 
  mov w3, 0x43 
  b dir_case_right
dir_case_up:
  sub w4, w4, w1
  b dir_switch_done
dir_case_down:
  add w4, w4, w1
  b dir_switch_done
dir_case_left:
  sub w5, w5, w1
  b dir_switch_done
dir_case_right:
  add w5, w5, w1
dir_switch_done:
  
  mov w14, w4
  mov w15, w5
  bl wrap_snake_p
  mov w4, w14
  mov w5, w15

  madd w10, w4, w9, w5
  add w10, w10, w2 
  ldrb w14, [x10]  

  madd w10, w4, w9, w5
  add w10, w10, w2 
  strb w3, [x10]  

  cmp w14, 0x0
  b.eq no_food 
  cmp w14, 0x1 // NORMALLY THIS IS GAME OVER HACK FOR NOW
  b.eq no_food 
  cmp w14, 0x2
  b.eq gobble 

no_food: 
  mov w7, w4 // Snake clear row
  mov w8, w5 // Snake clear col
  mov w16, w6
tail_trace:
  madd w10, w7, w9, w8
  add w10, w10, w2
  ldrb w3, [x10]

  cmp w3, 0x41 // Up
  b.eq adv_down 
  cmp w3, 0x42 // Down 
  b.eq adv_up 
  cmp w3, 0x44 // Left 
  b.eq adv_right 
  b adv_left
adv_down:
  add w7, w7, w1
  b adv_done
adv_up:
  sub w7, w7, w1
  b adv_done
adv_right:
  add w8, w8, w1
  b adv_done
adv_left:
  sub w8, w8, w1
adv_done:
  mov w14, w7
  mov w15, w8
  bl wrap_snake_p
  mov w7, w14
  mov w8, w15

  sub w16, w16, w1 
  cbnz w16, tail_trace

  madd w10, w7, w9, w8
  add w10, w10, w2 
  strb w0, [x10]  

  b occupancy_check_end
gobble:
  add w6, w6, w1 
occupancy_check_end:
  bl display_update 
  b game_loop

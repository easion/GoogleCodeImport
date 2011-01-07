#define _       ((unsigned) 0)          /* off bits */
#define X       ((unsigned) 1)          /* on bits */
#define MASK(a,b,c,d,e,f,g) \
        (((((((((((((a * 2) + b) * 2) + c) * 2) + d) * 2) \
        + e) * 2) + f) * 2) + g) << 9)
#define xterm_width	12
static unsigned short xterm_bits[] = { MASK(X,X,X,X,X,X,X),
                        MASK(_,_,X,_,X,_,_),
                        MASK(_,_,_,X,_,_,_),
                        MASK(_,_,_,X,_,_,_),
                        MASK(_,_,_,X,_,_,_),
                        MASK(_,_,_,X,_,_,_),
                        MASK(_,_,_,X,_,_,_),
                        MASK(_,_,_,X,_,_,_),
                        MASK(_,_,_,X,_,_,_),
                        MASK(_,_,_,X,_,_,_),
                        MASK(_,_,X,_,X,_,_),
                        MASK(X,X,X,X,X,X,X)};
static unsigned short xterm_mask[] = { MASK(X,X,X,X,X,X,X),
                        MASK(_,X,X,X,X,X,_),
                        MASK(_,_,X,X,X,_,_),
                        MASK(_,_,X,X,X,_,_),
                        MASK(_,_,X,X,X,_,_),
                        MASK(_,_,X,X,X,_,_),
                        MASK(_,_,X,X,X,_,_),
                        MASK(_,_,X,X,X,_,_),
                        MASK(_,_,X,X,X,_,_),
                        MASK(_,_,X,X,X,_,_),
                        MASK(_,X,X,X,X,X,_),
                        MASK(X,X,X,X,X,X,X)};

#define Down_width 16
#define Down_height 16 
#define Down_x_hot 8
#define Down_y_hot 15
static unsigned short Down_bits[] = {
0x0000, 0x0000, 0x0380, 0x0380, 
0x0380, 0x0380, 0x0380, 0x0380, 
0x3ff8, 0x1ff0, 0x0fe0, 0x0fe0, 
0x07c0, 0x0380, 0x0380, 0x0100};
#define Right_width 16
#define Right_height 16 
#define Right_x_hot 12
#define Right_y_hot 8
static unsigned short Right_bits[] = {
0x0000, 0x0000, 0x0080, 0x00c0,
0x00F0, 0x00F8, 0x3ffe, 0x3fff,
0x3ffe, 0x00F8, 0x00F0, 0x00c0, 
0x0080, 0x0000, 0x0000, 0x0000};

#define Left_width 16
#define Left_height 16 
#define Left_x_hot 1
#define Left_y_hot 8
static unsigned short Left_bits[] = {
0x0000, 0x0000, 0x0100, 0x0300,
0x0F00, 0x1F00, 0x7FFC, 0xfffc,
0x7ffc, 0x1f00, 0x0f00, 0x0300, 
0x0100, 0x0000, 0x0000, 0x0000};

#define left_ptr_width 16
#define left_ptr_height 16
static unsigned short left_ptr_bits[] = {
0xe000, 0x9800, 0x8600, 0x4180,
0x4060, 0x2018, 0x2004, 0x107c,
0x1020, 0x0910, 0x0988, 0x0544,
0x0522, 0x0211, 0x000a, 0x0004};
static unsigned short left_ptrmsk_bits[] = {
0xe000, 0xf800, 0xfe00, 0x7f80,
0x7fe0, 0x3ff8, 0x3ffc, 0x1ffc,
0x1fe0, 0x0ff0, 0x0ff8, 0x077c,
0x073e, 0x021f, 0x000e, 0x0004};


#define star_width 16
#define star_height 16
#define star_x_hot 7
#define star_y_hot 7
static unsigned short star_bits[] = {
   0x0000, 0x0080, 0x0080, 0x0888, 0x0490, 0x02a0, 
   0x0140, 0x3e3e, 0x0140, 0x02a0, 0x0490, 0x0888, 
   0x0080, 0x0080, 0x0000, 0x0000};
static unsigned short starMask_bits[] = {
   0x01c0, 0x01c0, 0x1ddc, 0x1ffc, 0x1ffc, 0x0ff8, 
   0x7fff, 0x7fff, 0x7fff, 0x0ff8, 0x1ffc, 0x1ffc, 
   0x1ddc, 0x01c0, 0x01c0, 0x0000};

/*
  0000000000000000   
  0000000000000000  
  0000000010000000  
  0000000011000000  
  0000000011110000  
  0000000011111000  
  0011111111111110
  0011111111111111  
  0011111111111110  
  0000000011111000  
  0000000011110000  
  0000000011000000  
  0000000010000000  
  0000000000000000  
  0000000000000000     
  0000000000000000     

  0000000000000000   
  0000000000000000  
  0000000000000000  
  0000100000010000  
  0001100000011000  
  0011100000011100  
  0111111111111110
  1111100000011111  
  0111111111111110  
  0011100000011100  
  0001100000011000  
  0000100000010000  
  0000000000000000  
  0000000000000000  
  0000000000000000     
  0000000000000000     
*/

#define double_arrow_width 16
#define double_arrow_height 16
#define double_arrow_x_hot 8
#define double_arrow_y_hot 8
static unsigned short double_arrow_bits[] = {
0x0000, 0x0000, 0x0000, 0x0810,
0x1818, 0x381c, 0x7ffe, 0xf81f,
0x7ffe, 0x381c, 0x1810, 0x0810,
0x0000, 0x0000, 0x0000, 0x0000};

#define double_v_arrow_width 16
#define double_v_arrow_height 16
#define double_v_arrow_x_hot 8
#define double_v_arrow_y_hot 8
static unsigned short double_v_arrow_bits[] = {
0x0100, 0x0380, 0x07c0, 0x0fe0,
0x1ff0, 0x0280, 0x0280, 0x0280,
0x0280, 0x0280, 0x0280, 0x1ff0,
0x0fe0, 0x07c0, 0x0380, 0x0100 };

#define hand1_width 16
#define hand1_height 16
#define hand1_x_hot  6
#define hand1_y_hot  0
static unsigned short hand1_bits[] = {
0x0600, 0x0900, 0x0900, 0x09C0,
0x0938, 0x0926, 0x0925, 0xE925,
0x9805, 0x8801, 0x4001, 0x2001,
0x2001, 0x1002, 0x1004, 0x0FF8
};

static unsigned short hand1Mask_bits[] = {
0x0600, 0x0F00, 0x0F00, 0x0FC0,
0x0FF8, 0x0FFE, 0x0FFF, 0xFFFF,
0xFFFF, 0xFFFF, 0x7FFF, 0x3FFF,
0x3FFF, 0x1FFE, 0x1FFC, 0x0FF8
};

#define right_ptr_width 16
#define right_ptr_height 16
#define right_ptr_x_hot 12
#define right_ptr_y_hot 1
static unsigned short right_ptr_bits[] = {
  0x0000, 0x0008, 0x0018, 0x0038, 0x0078, 0x00f8, 
  0x01f8, 0x01f8, 0x07f8, 0x00f8, 0x00d8, 0x0188, 
  0x0180, 0x0300, 0x0300, 0x0000};

static unsigned short right_ptrmsk_bits[] = {
 0x000c, 0x001c, 0x003c, 0x007c, 0x00fc, 0x01fc, 
 0x03fc, 0x07fc, 0x0ffc, 0x0ffc, 0x01fc, 0x03dc, 
 0x03cc, 0x0780, 0x0780, 0x0300};

#define cntr_ptr_width 16
#define cntr_ptr_height 16
#define cntr_ptr_x_hot 7
#define cntr_ptr_y_hot 1
static unsigned short cntr_ptr_bits[] = {
   0x0000, 0x0180, 0x0180, 0x03c0, 0x03c0, 0x07e0,
   0x07e0, 0x0ff0, 0x0ff0, 0x1998, 0x1188, 0x0180, 
   0x0180, 0x0180, 0x0180, 0x0000};
static unsigned short cntr_ptrmsk_bits[] = {
   0x03c0, 0x03c0, 0x07e0, 0x07e0, 0x0ff0, 0x0ff0, 
   0x1ff8, 0x1ff8, 0x3ffc, 0x3ffc, 0x3ffc, 0x3bdc, 
   0x03c0, 0x03c0, 0x03c0, 0x03c0};
/*
 0000000000000000
 0000000110000000
 0000000110000000
 0000001111000000 
 0000011111100000
 0000011111100000
 0000111111110000
 0000111111110000
 0001100110011000
 0001000110001000
 0000000110000000
 0000000110000000
 0000000110000000
 0000000110000000
 0000000000000000

 0000001111000000
 0000001111000000
 0000011111100000
 0000011111100000
 0000111111110000
 0000111111110000
 0001111111111000
 0011111111111100
 0011111111111100
 0011101111011100
 0000001111000000
 0000001111000000
 0000001111000000
 0000001111000000
*/

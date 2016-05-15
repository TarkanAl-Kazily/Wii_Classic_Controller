#include <Keyboard.h>
#include <Mouse.h>
#include <Wire.h>
#include <string.h>

#undef int
#include <stdio.h>
#include <stdint.h>
#define CLASSIC_BYTE_COUNT (6)

//LeftX, LeftY, RightX, RightY, LeftT, RightT, ButtonD, ButtonL, ButtonU, ButtonR, ButtonM, ButtonH, ButtonP, ButtonY, ButtonX, ButtonA, ButtonB, ButtonLT, ButtonRT, ButtonZL, ButtonZR
/*	  Bit
Byte	7	6	5	4	3	2	1	0
0	    RX<4:3>  |                  LX<5:0>
1	RX<2:1>	     |                  LY<5:0>
2	RX<0> |   LT<4:3>   |          	RY<4:0>
3	LT<2:0>	            |           RT<4:0>
4	BDR	BDD	BLT	B-	BH	B+	BRT	Null
5	BZL	BB	BY	BA	BX	BZR	BDL	BDU
*/

struct ClassicController {  
  int8_t LeftX;
  int8_t LeftY;
  int8_t RightX;
  int8_t RightY;
  int8_t LeftT;
  int8_t RightT;
  boolean ButtonD;
  boolean ButtonL;
  boolean ButtonU;
  boolean ButtonR;
  boolean ButtonM;
  boolean ButtonH;
  boolean ButtonP;
  boolean ButtonY;
  boolean ButtonX;
  boolean ButtonA;
  boolean ButtonB;
  boolean ButtonLT;
  boolean ButtonRT;
  boolean ButtonZL;
  boolean ButtonZR;
};

void setup ()
{
  Serial.begin (19200);
  //while (!Serial);  // Wait for Leonardo console
  Wire.begin ();		// join i2c bus with address 0x52
  Serial.print ("Wire has begun\n");
  nunchuck_init (); // send the initilization handshake
  Serial.print ("Finished nunchuck_init\n");
  Mouse.begin();
  Keyboard.begin();
  Serial.print ("Finished setup\n");
  
}

/**
 * @brief Write bytes out to a device over I2C
 *
 * @param buf is a pointer to a string of bytes
 * @param len is the length of the string
 */
void wire_write(uint8_t *buf, size_t len)
{
  if ((buf != NULL) && (len > 0)) {
    Wire.beginTransmission(*buf++);
    while (--len != 0) {
      Wire.write(*buf++);
    }
    Wire.endTransmission();
  }
}

uint8_t init_string_1[] = {0x52, 0xf0, 0x55}; // Comment about what this string does
uint8_t init_string_2[] = {0x52, 0xfb, 0x00}; // Comment about what this string does
uint8_t zero_string[] = {0x52, 0x00};

void nunchuck_init ()
{
  wire_write(init_string_1, sizeof(init_string_1));
  wire_write(init_string_2, sizeof(init_string_2));  
}

void send_zero ()
{
  wire_write(zero_string, sizeof(zero_string));
}

void loop ()
{
  struct ClassicController myCC_state;
  uint8_t rawbytes[CLASSIC_BYTE_COUNT];		// array to store arduino output
  size_t cnt = 0;
  
  send_zero (); // send the request for next bytes
  delay (20);
  Wire.requestFrom (0x52, CLASSIC_BYTE_COUNT);	// request data from nunchuck
  while (Wire.available ()) {
    rawbytes[cnt++] = Wire.read();
  }

  // If we recieved the 6 bytes, then do something with them
  if (cnt >= CLASSIC_BYTE_COUNT) {
    classic_decode_bytes(rawbytes, cnt, &myCC_state);
    classic_do_stuff(&myCC_state);
  }
}

void handle_key(boolean new_state, boolean old_state, char key)
{
  if (new_state && !old_state) {
    Keyboard.press(key);
  }
  else if (!new_state && old_state) {
    Keyboard.release(key);
  }
}
  
void handle_mouse(boolean new_state, boolean old_state, char key)
{
  if (new_state && !old_state) {
    Mouse.press(key);
    //Serial.println((int)key);
  }
  else if (!new_state && old_state) {
    Mouse.release(key);
  }
}  
  
void handle_move(int8_t new_state, int8_t old_state, char key, boolean neg)
{
  if (neg) {
    new_state *= -1;
    old_state *= -1;
  }
  if ((new_state > 10) && (old_state <= 10)) {
    Keyboard.press(key);
  }
  else if ((new_state <= 5) && (old_state > 5)) {
    Keyboard.release(key);
  }
}

int8_t mouse_speed[] = {-15, -13, -10, -7, -5, -4, -4, -4, -3, -3, -3, -2, -2, -2, -2, -1, 0, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 7, 10, 13, 15};

void classic_do_stuff(struct ClassicController *myStruct) 
{
  static struct ClassicController savedStruct;
  
  if (myStruct == NULL) {
    return;
  }
  
  if (myStruct->ButtonL && !myStruct->ButtonR) {
    Mouse.move(mouse_speed[myStruct->RightX + 16] , -mouse_speed[myStruct->RightY + 16], 1);
  }
  else if (!myStruct->ButtonL && myStruct->ButtonR) {
    Mouse.move(mouse_speed[myStruct->RightX + 16] , -mouse_speed[myStruct->RightY + 16], -1);
  }
  else Mouse.move(mouse_speed[myStruct->RightX + 16] , -mouse_speed[myStruct->RightY + 16], 0);

#define JOYSTATE(x) myStruct->x, savedStruct.x
  handle_key(JOYSTATE(ButtonZR), ' ');
  handle_key(JOYSTATE(ButtonY), 'e');
  handle_mouse(JOYSTATE(ButtonRT), MOUSE_RIGHT);
  handle_mouse(JOYSTATE(ButtonLT), MOUSE_LEFT);
  handle_key(JOYSTATE(ButtonP), 0x1B);
  handle_key(JOYSTATE(ButtonD), 'q');
  
  handle_move(JOYSTATE(LeftY), 'w', false);
  handle_move(JOYSTATE(LeftY), 's', true);
  handle_move(JOYSTATE(LeftX), 'd', false);
  handle_move(JOYSTATE(LeftX), 'a', true);
  
  savedStruct=*myStruct;
  
}

void classic_decode_bytes(uint8_t *buf, size_t len, struct ClassicController *myStruct)
{
  
  if ((buf == NULL) || (len < CLASSIC_BYTE_COUNT) || (myStruct == NULL)) {
    return;
  }
  
  myStruct->LeftX = (buf[0] & (64 - 1)) - 32; // 0 to 63
  myStruct->LeftY = (buf[1] & (64 - 1)) - 32; // 0 to 63 -> -32 to 31
  
  myStruct->RightX = (((buf[2] >> 7) & 1) + ((buf[1] >> 6) & 3)*2 + ((buf[0] >> 6) & 3)*8) - 16; // 0 to 31 -> -16 to 15
  myStruct->RightY = (buf[2] & (32 - 1)) - 16; // 0 to 31 -> -16 to 15
  
  Serial.print(myStruct->RightX);
  Serial.print('\n');
  Serial.print(myStruct->RightY);
 
  myStruct->LeftT = (((buf[2] >> 5) & 3)*8 + ((buf[3] >> 5) & 7));
  myStruct->RightT = (buf[3] & (32 - 1));
 
  myStruct->ButtonD = ((buf[4] & (1 << 6)) == 0);
  myStruct->ButtonL = ((buf[5] & (1 << 1)) == 0);
  myStruct->ButtonU = ((buf[5] & (1 << 0)) == 0);
  myStruct->ButtonR = ((buf[4] & (1 << 7)) == 0);
  myStruct->ButtonM = ((buf[4] & (1 << 4)) == 0);
  myStruct->ButtonH = ((buf[4] & (1 << 3)) == 0);
  myStruct->ButtonP = ((buf[4] & (1 << 2)) == 0);
  myStruct->ButtonY = ((buf[5] & (1 << 5)) == 0);
  myStruct->ButtonX = ((buf[5] & (1 << 3)) == 0);
  myStruct->ButtonA = ((buf[5] & (1 << 4)) == 0);
  myStruct->ButtonB = ((buf[5] & (1 << 6)) == 0);
  myStruct->ButtonLT = ((buf[4] & (1 << 5)) == 0);
  myStruct->ButtonRT = ((buf[4] & (1 << 1)) == 0);
  myStruct->ButtonZL = ((buf[5] & (1 << 7)) == 0);
  myStruct->ButtonZR = ((buf[5] & (1 << 2)) == 0);
}

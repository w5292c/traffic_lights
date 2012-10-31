//!@file main.c

#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>

/*
 * HW configuration:
 * Connector pin | LED-IDs  | IC Pin# | Pin name | Comment
 * =======================================================
 *             1 | South  + |      12 |      PB0 |
 *             2 | East   + |      13 |      PB1 |
 *             3 | Noth   + |      14 |      PB2 |
 *             4 | West   + |      15 |      PB3 |
 *             5 | Yellow - |       3 |      PD1 |
 *             6 | Red    - |       6 |      PD2 |
 *             7 | Green  - |       2 |      PD0 |
 */

/*
 * Bit 0: South
 * Bit 1: East
 * Bit 2: Noth
 * Bit 3: West
 *
 * TheLedsBitmap[0]: Bits: 00..03: Green;
 * TheLedsBitmap[1]: Bits: 04..07: Yellow;
 * TheLedsBitmap[2]: Bits: 08..11: Red;
 */
volatile static uint16_t TheLedsBitmap = 0x0A05;

typedef enum {
  EStateOff,
  EState1,
  EState2,
  EState3,
  EState4,
  EState5,
  EState6
} TState;

volatile static uint16_t TheTimer = 1;
volatile static uint8_t TheState = EStateOff;

static void func(void) {
  static uint16_t timer = 0;
  timer--;

  switch (TheState) {
  case EStateOff:
    timer = 15000;
    TheLedsBitmap = 0x050A;
    TheState = EState1;
    break;
  case EState1:
    if (!timer) {
      TheState = EState2;
      timer = 4000;
    }
    break;
  case EState2:
    if (timer) {
      if (timer % 800 > 400) {
        TheLedsBitmap = 0x050A;
      } else {
        TheLedsBitmap = 0x0500;
      }
    } else {
      timer = 3000;
      TheLedsBitmap = 0x05A0;
      TheState = EState3;
    }
    break;
  case EState3:
    if (!timer) {
      timer = 15000;
      TheLedsBitmap = 0x0A05;
      TheState = EState4;
    }
    break;
  case EState4:
    if (!timer) {
      timer = 4000;
      TheState = EState5;
    }
    break;
  case EState5:
    if (timer) {
      if (timer % 800 > 400) {
        TheLedsBitmap = 0x0A05;
      } else {
        TheLedsBitmap = 0x0A00;
      }
    } else {
      TheLedsBitmap = 0x0A50;
      timer = 3000;
      TheState = EState6;
    }
    break;
  case EState6:
    if (!timer) {
      TheState = EStateOff;
    }
  default:
    break;
  }
}

ISR(TIMER0_OVF_vect)
{
  const int factor = 1;
  static uint16_t n = 0;
  static uint16_t k = 0;

  func();

  if (++k < factor) {
    return;
  }
  k = 0;

  const int ledIndex = (n % 4);
  const uint8_t ledBitmap = ((TheLedsBitmap >> (ledIndex << 2)) & 0x0FU);
  switch (ledIndex) {
  case 0:
    PORTB = 0x00;
    PORTD = ~0x01;
    PORTB = ledBitmap;
    break;
  case 1:
    PORTB = 0x00;
    PORTD = ~0x02;
    PORTB = ledBitmap;
    break;
  case 2:
    PORTB = 0x00;
    PORTD = ~0x04;
    PORTB = ledBitmap;
    break;
  default:
    PORTB = 0x00;
    PORTD = ~0x00;
    break;
  }

  // update the index for the next ISR
  if (1000/factor < ++n) {
    n = 0;
  }
}

static void init_io(void)
{
  /* Output pins: PB0..PB3 */
  DDRB = 0x0F;
  PORTB = 0x0F;

  /* Output pins: PD0..PD2 */
  DDRD  = 0x07;
  PORTD = 0x07;
}

// *************************************************************************************************
// LOCAL FUNCTIONS IMPLEMENTATION
// *************************************************************************************************
static void init_timer_0(void)
{
  TCCR0B = (1<<CS00)|(1<<CS00);
  TIFR   = 1<<TOV0;
  TIMSK  = 1<<TOIE0;
}

int main (void)
{
  init_io ();
  init_timer_0 ();

  /* Enable interrupts */
  sei();

  for(;;)
  {
    sleep_mode();
  }

  return 0;
}

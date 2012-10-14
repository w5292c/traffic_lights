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
 * TheLedsBitmap[0]: Bits: 0..3: Green;
 * TheLedsBitmap[1]: Bits: 0..3: Yellow;
 * TheLedsBitmap[2]: Bits: 0..3: Red;
 * TheLedsBitmap[2]: Bits: 0..3: Red;
 */
volatile static uint16_t TheLedsBitmap = 0x5A5AU;
ISR(TIMER0_OVF_vect)
{
  const int factor = 1;
  static uint16_t n = 0;
  static uint16_t k = 0;
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
    TheLedsBitmap = ~TheLedsBitmap;
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

//!@file main.c

#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>

/*
 * HW configuration 1 (LEDs):
 * Connector pin | LED-IDs  | IC Pin# | Pin name | Comment
 * =======================================================
 *             1 | South  + |      12 |      PB0 | White-orange wire
 *             2 | East   + |      13 |      PB1 | Orange wire
 *             3 | Noth   + |      14 |      PB2 | White-green wire
 *             4 | West   + |      15 |      PB3 | Green wire
 *             5 | Yellow - |       3 |      PD1 | White-brown wire
 *             6 | Red    - |       6 |      PD2 | Brown wire
 *             7 | Green  - |       2 |      PD0 | Blue wire
 */

/*
 * HW configuration 2 (power management):
 * IC pin | Pin name | Connected to | Comment
 * =======================================================
 *     16 |      PB4 |   POWER LED+ | OUT
 *      7 |      PD3 | POWER BUTTON | IN/PULL UP
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
 * TheLedsBitmap[3]: Bit :     12: Power LED
 */
volatile static uint16_t TheLedsBitmap = 0x0000U;
volatile static  uint8_t ThePowerLed = 0;

typedef enum {
  EPwrReqNone,
  EPwrReqPowerButtonPressed,
  EPwrReqPowerButtonPressedLong,
  EPwrReqPowerButtonConfirmed,
  EPwrReqPowerButtonReleased,
  EPwrReqPowerDown,
  EPwrReqPowerUp
} TPwrReq;

volatile static uint8_t ThePowerRequest = EPwrReqNone;

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

static void power_man(void)
{
  static uint16_t counter = 0;
  const uint8_t powerButton = !(PIND & 0x08U);

  switch (ThePowerRequest) {
  case EPwrReqNone:
    if (powerButton) {
      counter = 4000;
      ThePowerRequest = EPwrReqPowerButtonPressed;
    } else {
      ThePowerLed = 0;
    }
    break;
  case EPwrReqPowerButtonPressed:
    if (powerButton) {
      if (!counter--) {
        ThePowerLed = 1;
        ThePowerRequest = EPwrReqPowerButtonPressedLong;
      }
    } else {
      ThePowerRequest = EPwrReqNone;
    }
    break;
  case EPwrReqPowerButtonPressedLong:
    if (!powerButton) {
      counter = 4000;
      ThePowerRequest = EPwrReqPowerButtonReleased;
    }
    break;
  case EPwrReqPowerButtonReleased:
    if (!powerButton) {
      if (!counter--) {
        ThePowerLed = 0;
        TheLedsBitmap = 0x0000U;
        ThePowerRequest = EPwrReqPowerDown;
      }
    } else {
      ThePowerRequest = EPwrReqNone;
    }
    break;
  case EPwrReqPowerUp:
    if (!powerButton) {
      ThePowerLed = 0;
      ThePowerRequest = EPwrReqNone;
    }
    break;
  default:
  case EPwrReqPowerDown:
    break;
  }
}

ISR(TIMER0_OVF_vect)
{
  const int factor = 1;
  static uint16_t n = 0;
  static uint16_t k = 0;

  if (EPwrReqPowerDown != ThePowerRequest) {
    func();
  }
  power_man();

  if (++k < factor) {
    return;
  }
  k = 0;

  const int ledIndex = (n % 4);
  const uint8_t ledBitmap = ((TheLedsBitmap >> (ledIndex << 2)) & 0x0FU) |
                            (ThePowerLed ? 0x10U : 0x00U);
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

ISR(INT1_vect)
{
  /* disable INT1 interrupt */
  GIMSK &= ~(1U << INT1);
  set_sleep_mode (SLEEP_MODE_IDLE);
  sleep_disable ();
  ThePowerLed = 1;
  ThePowerRequest = EPwrReqPowerUp;
  TheState = EStateOff;
}

static void init_io(void)
{
  /* Output pins: PB0..PB3 */
  DDRB = 0x1F;
  PORTB = 0x1F;

  /* Output pins: PD0..PD2, PD3: enable pull-ups */
  DDRD  = 0x07;
  PORTD = 0x0F;
}

static void init_int1(void)
{
  /* disable interrupt on INT1 pin */
  GIMSK &= ~(1U << INT1);
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
  init_int1 ();
  init_timer_0 ();

  /* Enable interrupts */
  sei();

  for(;;) {
    switch (ThePowerRequest) {
    case EPwrReqPowerUp:
      ThePowerLed = 1;
      break;
    case EPwrReqPowerDown:
      /* turn off all the LEDs */
      PORTB = 0;
      /* enable external interrupt */
      GIMSK |= (1U << INT1);
      /* clear SM0, SM1 and SE flags */
      set_sleep_mode (SLEEP_MODE_PWR_DOWN);
      sleep_enable ();
      ThePowerRequest = EPwrReqNone;
      sleep_cpu();
      break;
    default:
      break;
    }

    sleep_mode();
  }

  return 0;
}

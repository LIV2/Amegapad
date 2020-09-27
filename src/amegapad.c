/*
Amegapad, a Megadrive gamepad for Amiga implemented on the Atmel ATTINY861A.
Copyright (C) 2020 Matt Harlum

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define M_UP     PA0
#define M_DOWN   PA1
#define M_LEFT   PA2
#define M_RIGHT  PA3
#define M_AB     PA4
#define M_STARTC PA5
#define M_SELECT PA6

#define A_UP     PA7
#define A_DOWN   PB2
#define A_LEFT   PB3
#define A_RIGHT  PB4
#define A_1      PB5
#define A_2      PB6

#define BTN_UP     (1<<0)
#define BTN_DOWN   (1<<1)
#define BTN_LEFT   (1<<2)
#define BTN_RIGHT  (1<<3)
#define BTN_B      (1<<4)
#define BTN_C      (1<<5)
#define BTN_A      (1<<6)
#define BTN_START  (1<<7)

#define BTN_Z      (1<<0)
#define BTN_Y      (1<<1)
#define BTN_X      (1<<2)
#define BTN_MODE   (1<<3)

/*
 Autofire configuration.
 If configuring one of X Y Z as an autofire button then the relevant state is buttonstate2
 Otherwise use buttonstate1
*/
#define AUTOFIRE_A_BUTTON BTN_Y
#define AUTOFIRE_A_STATE  buttonstate2
#define AUTOFIRE_B_BUTTON BTN_C
#define AUTOFIRE_B_STATE  buttonstate1

#define AF_FRAMES  3

uint8_t buttonstate1;
uint8_t buttonstate2;
uint8_t counter;

void setup(void)
{
  cli();
  PORTA |= (1<<M_UP|1<<M_DOWN|1<<M_LEFT|1<<M_RIGHT|1<<M_AB|1<<M_STARTC); // Pull-up enabled

  PORTA |= (1<<A_UP);
  PORTB |= (1<<A_LEFT|1<<A_RIGHT|1<<A_DOWN|1<<A_1|1<<A_2);

  DDRA |= (1<<M_SELECT);

  OCR0A = 78; // 100HZ twice a PAL frame
  TCNT0H = 0;
  TCNT0L = 1;
  TCCR0A |= (1<<WGM00);  // CTC mode
  TCCR0B |= (1<<CS02|1<<CS00); // /1024 prescaler
 
  TIMSK |= (1<<OCIE0A);
  sei();
}
 
void setButton(uint8_t button, uint8_t pin) {
  if (buttonstate1 & button) {
    DDRB &= ~(1<<pin);  // Input
    PORTB |= (1<<pin);  // High - Pull-up enable
  } else {
    PORTB &= ~(1<<pin); // Low
    DDRB |= (1<<pin);   // Output
  }
}

void setFire(uint8_t pin, uint8_t state) {
  if (state) {
    PORTB &= ~(1<<pin); // Low
    DDRB |= (1<<pin);   // Output
  } else {
    DDRB &= ~(1<<pin);  // Input
    PORTB |= (1<<pin);  // High - Pull-up enable
  }
}

void setSelect(uint8_t state) {
  if (state % 2) {
    PORTA |= (1<<M_SELECT);
  } else {
    PORTA &= ~(1<<M_SELECT);
  }
}

ISR (TIMER0_COMPA_vect) {
  counter++;
  for (uint8_t state=0;state<=7;state++) {
    setSelect(state);
    switch(state)
    {
      case 0:
      case 1:
        buttonstate1 = 0;
        break;
      case 2:
        // Buttons: A, Start
        buttonstate1 |= ((PINA & 0x30) << 2);
        break;
      case 3:
        // Buttons: Up,Down,Left,Right,B,C
        buttonstate1 |= (PINA & 0x3F);
        break;
      case 4:
        if (((PINA & 0x03) == 0x03)) {
          // Not a 6-button controller, skip the rest
          state = 6;
          buttonstate2 = 0xFF;
        }
        break;
      case 5:
        buttonstate2 = (PINA & 0x0F);
        break;
      case 6:
        break;
      case 7:
        if (buttonstate1 & BTN_UP) {
          DDRA &= ~(1<<A_UP);  // Input
          PORTA |= (1<<A_UP);  // High - Pull-up enable
        } else {
          PORTA &= ~(1<<A_UP); // Low
          DDRA |= (1<<A_UP);   // Output
        }
        setButton(BTN_DOWN,A_DOWN);
        setButton(BTN_LEFT,A_LEFT);
        setButton(BTN_RIGHT,A_RIGHT);
        if (!(AUTOFIRE_A_STATE & AUTOFIRE_A_BUTTON)) {
          setFire(A_1, counter % AF_FRAMES);
        } else {
          setButton(BTN_A,A_1);
        }
        if (!(AUTOFIRE_B_STATE & AUTOFIRE_B_BUTTON)) {
          setFire(A_2, counter % AF_FRAMES);
        } else {
          setButton(BTN_B,A_2);
        }
        break;
      default:
        break;
    }
  }
  _delay_us(10);
}
 
int main(void)
{
  setup();
  while (1)
  {
  }
}

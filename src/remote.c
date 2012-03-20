/*
 * Copyright (C) 2012-2013 Julien Desfossez
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define F_CPU 8000000
#define PUSHBUTTON 2

#include <inttypes.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <avr/interrupt.h>

void send_one(void)
{
	TCCR0A = 0x42;
	TCCR0B = 0x01;
	OCR0A = 99;
	_delay_us(1175);
	TCCR0B = 0x00;
	TCCR0A = 0x00;
	_delay_us(650);
}

void send_zero(void)
{
	TCCR0A = 0x42;
	TCCR0B = 0x01;
	OCR0A = 99;
	_delay_us(575);
	TCCR0B = 0x00;
	TCCR0A = 0x00;
	_delay_us(650);
}

void startup(void)
{
	TCCR0A = 0x42;
	TCCR0B = 0x01;
	OCR0A = 99;
	_delay_us(2320);
	TCCR0B = 0x00;
	TCCR0A = 0x00;
	_delay_us(650);
}

void sony(void)
{
	uint8_t i, j;
	uint8_t seq[] = {1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};

	for (i = 0; i < 3; i++) {
		startup();
		for (j = 0; j < sizeof(seq); j++)
			seq[j] ? send_one() : send_zero();
		_delay_ms(10);
	}
}

void sony_delayed(void)
{
	uint8_t i, j;
	uint8_t seq[] = {1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};

	for (i = 0; i < 3; i++) {
		startup();
		for (j = 0; j < sizeof(seq); j++)
			seq[j] ? send_one() : send_zero();
		_delay_ms(10);
	}
}

uint8_t wait_125ms(uint8_t t) // wait for 125 ms, waiting not interruptible
{
	uint8_t i;
	uint8_t new_state;

	WDTCR=0b11000011; // 125ms s
	for (i = 0; i < t; i++)	{
		PORTB |= (1 << PUSHBUTTON); // button pull-up --> allow INT0 again
		asm volatile("nop"::);  //one clock delay before reading PINB
		new_state = PINB;
		PORTB &= ~(1 << PUSHBUTTON);
		if (!(~new_state & (1 << PUSHBUTTON))) {
			return 1;
		}
		sleep_mode();
	}
	return 0;
}

uint8_t wait_sec(uint8_t sec)
{
	uint8_t i, ret;

	for (i = 0; i < sec; i++) {
		ret = wait_125ms(8);
		if (ret)
			return ret;
	}
	return 0;
}

EMPTY_INTERRUPT(WDT_vect);         // watchdog interrupt for waking up

int main(void)
{
	uint8_t delayt, ret;
	uint8_t delayed = 1;

	DDRB |= (1 << 0); // Set B0 as output
	DDRB |= (0 << PUSHBUTTON); // Set B2 as input

	PORTB = (0 << PUSHBUTTON); // button pull-up
	sei(); // enable interrupts

#if 0
	// Old example kept here as PWM documentation :

	// start up Timer0 in CTC Mode at about 38KHz to drive the IR emitter on output OC0A:
	//   8-bit Timer0 OC0A (PB0, pin 5) is set up for CTC mode, toggling output on each compare
	//   Fclk = Clock = 8MHz
	//   Prescale = 1
	//   OCR0A = 104
	//   F = Fclk / (2 * Prescale * (1 + OCR0A) ) = 38KHz
	//

		TCCR0A = 0x42;//01000010  // COM0A1:0=01 to toggle OC0A on Compare Match
		// COM0B1:0=00 to disconnect OC0B
		// bits 3:2 are unused
		// WGM01:00=10 for CTC Mode (WGM02=0 in TCCR0B)
		TCCR0B = 0x01;//00000001  // FOC0A=0 (no force compare)
		// F0C0B=0 (no force compare)
		// bits 5:4 are unused
		// WGM2=0 for CTC Mode (WGM01:00=10 in TCCR0A)
		// CS02:00=001 for divide by 1 prescaler (this starts Timer0)
		OCR0A = 99;  // to output 40KHz on OC0A (PB0, pin 5)

		_delay_ms(1000);
		// turn off Timer0 to stop 38KHz pulsing of IR
		TCCR0B = 0x00;//00000000  // CS02:CS00=000 to stop Timer0 (turn off IR emitter)
		TCCR0A = 0x00;//00000000  // COM0A1:0=00 to disconnect OC0A from PB0 (pin 5)

		_delay_ms(1000);
#endif

	delayt = 1;

	while (1) {
		if (delayed)
			sony_delayed();
		else
			sony();

		ret = wait_sec(delayt);
		if (ret) {
			if (delayed && delayt == 1)
				delayed = 0;
			else
				delayt++;
		}
	}

	return 0;
}

#include <stdlib.h>
#include <string.h>

#include "input.h"

// joystick

Joystick* joyCreate() {
	Joystick* joy = (Joystick*)malloc(sizeof(Joystick));
	joy->type = XJ_KEMPSTON;
	joy->state = 0;
	return joy;
}

void joyDestroy(Joystick* joy) {
	free(joy);
}

void joyPress(Joystick* joy, int mask) {
	if ((joy->state | mask) != joy->state) x_input_bump();
	joy->state |= mask;
}

void joyRelease(Joystick* joy, int mask) {
	if (joy->state & mask) x_input_bump();
	joy->state &= ~mask;
}

unsigned char joyInput(Joystick* joy) {
	unsigned char res = 0xff;
	joy->used = 1;
	switch (joy->type) {
		case XJ_KEMPSTON:
			// the interface pulls d5..d7 low: idle reads 0, as its manual says
			res = joy->extbuttons ? joy->state : (joy->state & 0x1f);
			break;
	}
	return res;
}

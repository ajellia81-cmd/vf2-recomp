#pragma once

#include "controls.h"

// The iOS adapter owns UIKit/GameController callbacks; the SDL host samples this
// small, UI-neutral state once per emulation frame. It is compiled only in the iOS target.
struct ios_input_snapshot {
	bool down[2][C_COUNT]{};
	bool menu_pressed = false;
};

void ios_input_install();
void ios_input_uninstall();
ios_input_snapshot ios_input_read();

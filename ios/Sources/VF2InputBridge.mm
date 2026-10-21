#import <GameController/GameController.h>
#import <UIKit/UIKit.h>

#include "../../src/host/ios_input.h"

namespace {

ios_input_snapshot g_input;
bool g_installed = false;
bool menu_was[2]{};
GCController *slots[2];

void sample(GCExtendedGamepad *pad, int player, ios_input_snapshot &input)
{
	if (!pad) return;
	auto button = [&](GCControllerButtonInput *b, control_action action) {
		input.down[player][action] |= b && b.pressed;
	};
	for (GCControllerDirectionPad *d in @[pad.dpad, pad.leftThumbstick]) {
		button(d.up, C_UP); button(d.down, C_DOWN);
		button(d.left, C_LEFT); button(d.right, C_RIGHT);
	}
	button(pad.buttonX, C_GUARD); button(pad.buttonA, C_PUNCH); button(pad.buttonB, C_KICK);
	button(pad.buttonY, C_GUARD_PUNCH); button(pad.leftShoulder, C_GUARD_KICK);
	button(pad.rightShoulder, C_PUNCH_KICK); button(pad.leftTrigger, C_GUARD_KICK);
	button(pad.rightTrigger, C_ALL_THREE); button(pad.buttonMenu, C_START);
	const bool menu = pad.buttonOptions.pressed;
	input.menu_pressed |= menu && !menu_was[player];
	menu_was[player] = menu;
}

} // namespace

@interface VF2TouchButton : UIButton
@property(nonatomic) control_action action;
@end
@implementation VF2TouchButton
@end

@interface VF2TouchOverlay : UIView
@end

static VF2TouchOverlay *g_overlay;

@implementation VF2TouchOverlay
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event
{
	UIView *hit = [super hitTest:point withEvent:event];
	return hit == self ? nil : hit; // Menu gestures reach SDL through unused overlay space.
}

- (VF2TouchButton *)button:(NSString *)title action:(control_action)action frame:(CGRect)frame
{
	for (VF2TouchButton *existing in self.subviews) {
		if (existing.action == action) { existing.frame = frame; return existing; }
	}
	VF2TouchButton *button = [VF2TouchButton buttonWithType:UIButtonTypeSystem];
	button.action = action;
	button.frame = frame;
	button.layer.cornerRadius = frame.size.width * 0.5;
	button.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.35];
	[button setTitle:title forState:UIControlStateNormal];
	[button setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
	[button addTarget:self action:@selector(down:) forControlEvents:UIControlEventTouchDown | UIControlEventTouchDragEnter];
	[button addTarget:self action:@selector(up:) forControlEvents:UIControlEventTouchUpInside | UIControlEventTouchUpOutside | UIControlEventTouchCancel | UIControlEventTouchDragExit];
	[self addSubview:button];
	return button;
}
- (void)down:(VF2TouchButton *)sender
{
	if (sender.action == C_MENU) g_input.menu_pressed = true;
	else g_input.down[0][sender.action] = true;
}
- (void)up:(VF2TouchButton *)sender
{
	if (sender.action != C_MENU) g_input.down[0][sender.action] = false;
}
- (void)layoutSubviews
{
	[super layoutSubviews];
	const CGFloat d = 58.0, gap = 8.0;
	const CGRect b = UIEdgeInsetsInsetRect(self.bounds, self.safeAreaInsets);

	[self button:@"↑" action:C_UP frame:CGRectMake(CGRectGetMinX(b) + d + gap, CGRectGetMaxY(b) - 3*d - 2*gap, d, d)];
	[self button:@"←" action:C_LEFT frame:CGRectMake(CGRectGetMinX(b), CGRectGetMaxY(b) - 2*d - gap, d, d)];
	[self button:@"↓" action:C_DOWN frame:CGRectMake(CGRectGetMinX(b) + d + gap, CGRectGetMaxY(b) - 2*d - gap, d, d)];
	[self button:@"→" action:C_RIGHT frame:CGRectMake(CGRectGetMinX(b) + 2*(d + gap), CGRectGetMaxY(b) - 2*d - gap, d, d)];
	[self button:@"G" action:C_GUARD frame:CGRectMake(CGRectGetMaxX(b) - 3*d - 2*gap, CGRectGetMaxY(b) - 2*d - gap, d, d)];
	[self button:@"P" action:C_PUNCH frame:CGRectMake(CGRectGetMaxX(b) - 2*d - gap, CGRectGetMaxY(b) - 3*d - 2*gap, d, d)];
	[self button:@"K" action:C_KICK frame:CGRectMake(CGRectGetMaxX(b) - 2*d - gap, CGRectGetMaxY(b) - d, d, d)];
	[self button:@"Coin" action:C_COIN frame:CGRectMake(CGRectGetMaxX(b) - 256, CGRectGetMinY(b) + 10, 76, 38)];
	[self button:@"Start" action:C_START frame:CGRectMake(CGRectGetMaxX(b) - 170, CGRectGetMinY(b) + 10, 76, 38)];
	[self button:@"Menu" action:C_MENU frame:CGRectMake(CGRectGetMaxX(b) - 84, CGRectGetMinY(b) + 10, 76, 38)];
}
@end

void ios_input_install(SDL_Window *window)
{
	if (g_installed) return;
	UIWindow *native = (__bridge UIWindow *)SDL_GetPointerProperty(SDL_GetWindowProperties(window),
		SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, nullptr);
	if (!native) return;
	g_installed = true;
	g_overlay = [[VF2TouchOverlay alloc] initWithFrame:native.bounds];
	g_overlay.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	g_overlay.backgroundColor = UIColor.clearColor;
	g_overlay.multipleTouchEnabled = YES;
	[native addSubview:g_overlay];
}

void ios_input_reset()
{
	g_input = {};
	menu_was[0] = menu_was[1] = false;
}

void ios_input_set_menu(bool open)
{
	for (VF2TouchButton *button in g_overlay.subviews)
		button.hidden = open && button.action != C_MENU;
	if (open) g_input = {};
}

void ios_input_uninstall()
{
	ios_input_reset();
	[g_overlay removeFromSuperview];
	g_overlay = nil;
	slots[0] = slots[1] = nil;
	g_installed = false;
}

ios_input_snapshot ios_input_read()
{
	// UIKit and the SDL iOS host run on the main thread. Polling avoids callback races,
	// overlapping-button releases, stale disconnect state, and duplicate observer ownership.
	ios_input_snapshot copy = g_input;
	g_input.menu_pressed = false;
	NSArray<GCController *> *connected = GCController.controllers;
	for (int p = 0; p < 2; ++p) {
		if (slots[p] && ![connected containsObject:slots[p]]) {
			slots[p] = nil;
			menu_was[p] = false;
		}
	}
	for (GCController *controller in connected) {
		if (!controller.extendedGamepad || controller == slots[0] || controller == slots[1]) continue;
		for (int p = 0; p < 2; ++p) if (!slots[p]) { slots[p] = controller; break; }
	}
	for (int p = 0; p < 2; ++p) sample(slots[p].extendedGamepad, p, copy);
	return copy;
}

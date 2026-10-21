#import <GameController/GameController.h>
#import <UIKit/UIKit.h>

#include "../../src/host/ios_input.h"

namespace {

ios_input_snapshot g_input;
bool g_installed = false;
id controller_observer = nil;

void bind_button(GCControllerButtonInput *button, int player, control_action action)
{
	if (!button) return;
	button.valueChangedHandler = ^(GCControllerButtonInput *, float, BOOL pressed) {
		g_input.down[player][action] = pressed;
	};
}

void bind_dpad(GCControllerDirectionPad *dpad, int player)
{
	if (!dpad) return;
	bind_button(dpad.up, player, C_UP);
	bind_button(dpad.down, player, C_DOWN);
	bind_button(dpad.left, player, C_LEFT);
	bind_button(dpad.right, player, C_RIGHT);
}

void attach(GCController *controller, int player)
{
	GCExtendedGamepad *pad = controller.extendedGamepad;
	if (!pad || player < 0 || player > 1) return;
	bind_dpad(pad.dpad, player);
	bind_dpad(pad.leftThumbstick, player);
	bind_button(pad.buttonX, player, C_GUARD);
	bind_button(pad.buttonA, player, C_PUNCH);
	bind_button(pad.buttonB, player, C_KICK);
	bind_button(pad.buttonY, player, C_GUARD_PUNCH);
	bind_button(pad.leftShoulder, player, C_GUARD_KICK);
	bind_button(pad.rightShoulder, player, C_PUNCH_KICK);
	bind_button(pad.leftTrigger, player, C_GUARD_KICK);
	bind_button(pad.rightTrigger, player, C_ALL_THREE);
	bind_button(pad.buttonMenu, player, C_START);
	if (pad.buttonOptions) {
		pad.buttonOptions.valueChangedHandler = ^(GCControllerButtonInput *, float, BOOL pressed) {
			if (pressed) g_input.menu_pressed = true;
		};
	}
}

} // namespace

@interface VF2TouchButton : UIButton
@property(nonatomic) control_action action;
@end
@implementation VF2TouchButton
@end

@interface VF2TouchOverlay : UIView
@end

@implementation VF2TouchOverlay
- (VF2TouchButton *)button:(NSString *)title action:(control_action)action frame:(CGRect)frame
{
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
	for (UIView *v in self.subviews) [v removeFromSuperview];
	[self button:@"↑" action:C_UP frame:CGRectMake(CGRectGetMinX(b) + d + gap, CGRectGetMaxY(b) - 3*d - 2*gap, d, d)];
	[self button:@"←" action:C_LEFT frame:CGRectMake(CGRectGetMinX(b), CGRectGetMaxY(b) - 2*d - gap, d, d)];
	[self button:@"↓" action:C_DOWN frame:CGRectMake(CGRectGetMinX(b) + d + gap, CGRectGetMaxY(b) - 2*d - gap, d, d)];
	[self button:@"→" action:C_RIGHT frame:CGRectMake(CGRectGetMinX(b) + 2*(d + gap), CGRectGetMaxY(b) - 2*d - gap, d, d)];
	[self button:@"G" action:C_GUARD frame:CGRectMake(CGRectGetMaxX(b) - 3*d - 2*gap, CGRectGetMaxY(b) - 2*d - gap, d, d)];
	[self button:@"P" action:C_PUNCH frame:CGRectMake(CGRectGetMaxX(b) - 2*d - gap, CGRectGetMaxY(b) - 3*d - 2*gap, d, d)];
	[self button:@"K" action:C_KICK frame:CGRectMake(CGRectGetMaxX(b) - 2*d - gap, CGRectGetMaxY(b) - d, d, d)];
	[self button:@"Start" action:C_START frame:CGRectMake(CGRectGetMaxX(b) - 170, CGRectGetMinY(b) + 10, 76, 38)];
	[self button:@"Menu" action:C_MENU frame:CGRectMake(CGRectGetMaxX(b) - 84, CGRectGetMinY(b) + 10, 76, 38)];
}
@end

static void install_overlay()
{
	dispatch_async(dispatch_get_main_queue(), ^{
		UIWindow *window = nil;
		for (UIScene *scene in UIApplication.sharedApplication.connectedScenes)
			if (scene.activationState == UISceneActivationStateForegroundActive && [scene isKindOfClass:UIWindowScene.class]) {
				window = ((UIWindowScene *)scene).windows.firstObject;
				if (window) break;
			}
		if (!window) return;
		VF2TouchOverlay *overlay = [[VF2TouchOverlay alloc] initWithFrame:window.bounds];
		overlay.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
		overlay.backgroundColor = UIColor.clearColor;
		[window addSubview:overlay];
	});
}

void ios_input_install()
{
	if (g_installed) return;
	g_installed = true;
	int player = 0;
	for (GCController *controller in GCController.controllers) attach(controller, player++);
	controller_observer = [[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidConnectNotification object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note) {
		for (int p = 0; p < 2; p++) if (!g_input.down[p][C_START]) { attach(note.object, p); break; }
	}];
	install_overlay();
}

void ios_input_uninstall()
{
	g_input = {};
	g_installed = false;
	[[NSNotificationCenter defaultCenter] removeObserver:controller_observer];
	controller_observer = nil;
}

ios_input_snapshot ios_input_read()
{
	ios_input_snapshot copy = g_input;
	g_input.menu_pressed = false; // edge-triggered: exactly one host frame sees a menu request
	return copy;
}

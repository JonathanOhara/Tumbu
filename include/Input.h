#ifndef __Input_h_
#define __Input_h_

#include <OgreInput.h>

/**
 * Keyboard codes and gamepad mapping (SDL2 through OgreBites).
 *
 * Letter and digit keys compare directly with character literals (evt.keysym.sym == 'w').
 * Gamepads use SDL's GameController layout (Xbox naming; PlayStation pads map to the same positions).
 */
namespace TumbuInput{
	// SDL keycodes OgreBites does not name.
	const OgreBites::Keycode KEY_LCTRL		= (1 << 30) | 0xE0;
	const OgreBites::Keycode KEY_LSHIFT		= OgreBites::SDLK_LSHIFT;
	const OgreBites::Keycode KEY_ESCAPE		= OgreBites::SDLK_ESCAPE;
	const OgreBites::Keycode KEY_SPACE		= OgreBites::SDLK_SPACE;
	const OgreBites::Keycode KEY_RETURN		= OgreBites::SDLK_RETURN;
	const OgreBites::Keycode KEY_PAGEUP		= OgreBites::SDLK_PAGEUP;
	const OgreBites::Keycode KEY_PAGEDOWN	= OgreBites::SDLK_PAGEDOWN;
	const OgreBites::Keycode KEY_HOME		= OgreBites::SDLK_HOME;
	const OgreBites::Keycode KEY_END		= OgreBites::SDLK_END;
	const OgreBites::Keycode KEY_F12		= OgreBites::SDLK_F12;

	// SDL_GameControllerButton values.
	enum PadButton{
		PAD_A = 0, PAD_B, PAD_X, PAD_Y, PAD_BACK, PAD_GUIDE, PAD_START, PAD_LEFTSTICK, PAD_RIGHTSTICK,
		PAD_LEFTSHOULDER, PAD_RIGHTSHOULDER, PAD_DPAD_UP, PAD_DPAD_DOWN, PAD_DPAD_LEFT, PAD_DPAD_RIGHT
	};

	// SDL_GameControllerAxis values; sticks range -32768..32767, triggers 0..32767.
	enum PadAxis{
		AXIS_LEFTX = 0, AXIS_LEFTY, AXIS_RIGHTX, AXIS_RIGHTY, AXIS_TRIGGERLEFT, AXIS_TRIGGERRIGHT
	};
	const int AXIS_DEADZONE = 8000;

	// Game actions on the gamepad (the 2011 pad used buttons 0/1/2 special/punch/kick, 4 run, 5 guard, 6/7 camera).
	const int PAD_SPECIAL	= PAD_Y;
	const int PAD_PUNCH		= PAD_X;
	const int PAD_KICK		= PAD_A;
	const int PAD_RUN		= PAD_LEFTSHOULDER;
	const int PAD_GUARD		= PAD_RIGHTSHOULDER;
	const int PAD_MENU		= PAD_START;	// same as ESC
	const int PAD_OK		= PAD_A;		// dialogs: same as SPACE
	const int PAD_CANCEL	= PAD_B;		// dialogs: same as ESC
}

#endif // #ifndef __Input_h_

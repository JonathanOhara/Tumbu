#ifndef __FlyCamera_h_
#define __FlyCamera_h_

#include <Ogre.h>
#include <OgreInput.h>

#include "Enums.h"

/**
 * Free "fly" camera for looking around the arena: F during a match. While flying the match is frozen (game
 * state FLYING): robots, physics and the clock stop, and the chase camera lets go of the view. Leaving puts
 * the camera back where it was.
 *
 * Keys: WASD move, mouse look (arrow keys too), Space up, C down, Shift faster, F or Esc back to the match.
 * On by default; `flyCamera 0` in camera.object removes it (flySpeed sets the speed in units per second).
 */
class FlyCamera: public OgreBites::InputListener, public Ogre::FrameListener{
public:
	FlyCamera( Ogre::Camera* camera );
	virtual ~FlyCamera(void);

	/// camera.object `flyCamera` (1 when missing).
	static bool isEnabled(void);
	bool isFlying(void) const { return flying; }

	bool frameRenderingQueued( const Ogre::FrameEvent &evt );
	bool keyPressed( const OgreBites::KeyboardEvent &evt );
	bool keyReleased( const OgreBites::KeyboardEvent &evt );
	bool mouseMoved( const OgreBites::MouseMotionEvent &evt );

private:
	void start(void);
	void stop(void);
	void applyOrientation(void);

	Ogre::Camera* camera;
	bool flying;
	bool stopRequested;

	// The camera's parent node, as the chase camera left it (restored when flying stops).
	Ogre::Vector3 savedPosition;
	Ogre::Quaternion savedOrientation;
	Ogre::Vector3 position;
	Ogre::Radian yaw, pitch;

	bool moveForward, moveBack, moveLeft, moveRight, moveUp, moveDown, fast;
	bool lookLeft, lookRight, lookUp, lookDown;
	Ogre::Real speed;
};

#endif // #ifndef __FlyCamera_h_

#ifndef __Sky_h_
#define __Sky_h_

#include <Ogre.h>

#include "Clock.h"
#include "TUMBU.h"


class Clock;

/** The painted toon sky (Tumbu/ToonSky, media/tumbu/shading/sky.frag) on a big box around the arena. Its colours, sun and moon
 *  come from the lighting rig (Lighting passes them as shared shader values), so the visible sun always matches the
 *  shadows, on Direct3D 11 and OpenGL alike. It replaced Caelum and the 2011 skydome textures (2026-10). */
class Sky: public Ogre::FrameListener{
public:
	Sky( Ogre::SceneManager* sceneMgr );
	virtual ~Sky(void);
	static Sky* getInstance(void);

	void setClock( Clock* _clock );
	/// Sky quality "Low": the bands, sun, moon and stars, no clouds and no twinkling.
	void skyLowQuality();
	/// Sky quality "High": with the drifting clouds.
	void skyHighQuality(Ogre::Camera* camera);
	/// 0 Low, 1 High (Lighting passes it to the sky shader).
	int getQuality(void);

	bool frameRenderingQueued(const Ogre::FrameEvent &evt);

	/// Kept for Lighting: the sky shader reads the light direction itself (shared sunDirection).
	void setLightDirection( const Ogre::Vector3 &direction, bool isMoon );

private:
	void showSky(void);

	Ogre::Light* light;
	Ogre::SceneNode* skyNode;
	Ogre::ManualObject* skyObject;
	Ogre::SceneManager* mSceneMgr;
	Clock* clock;
	int quality;

	static Sky* instance;
};

#endif // #ifndef __Sky_h_

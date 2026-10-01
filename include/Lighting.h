#ifndef __Lighting_h_
#define __Lighting_h_

#include <Ogre.h>
#include <vector>

class Clock;
class Sky;

/**
 * Lighting rig of a match: the sun, the ambient light, the shared toon-shading parameters ("TumbuLighting",
 * used by the robot and arena shaders) and the post-processing compositor (god rays, bloom, tone mapping and grading).
 * The values come from media/configuration/lighting.object, as keyframes that the game clock blends through
 * the day.
 */
class Lighting: public Ogre::FrameListener, public Ogre::CompositorInstance::Listener{
public:
	Lighting( Ogre::SceneManager* sceneMgr );
	virtual ~Lighting(void);
	static Lighting* getInstance(void);

	/// Creates the "TumbuLighting" shared parameters. Must run before the shader scripts are parsed.
	static void declareSharedParameters(void);

	void setClock( Clock* _clock );
	/// The directional light that plays the sun (and the moon at night); it also casts the shadows.
	void setSun( Ogre::Light* _sun );
	/// The visible sky (High quality) follows the lighting sun by day and puts the moon there at night.
	void setSky( Sky* _sky );
	/// Applies the lighting of the given time of day (0..24 hours).
	void update( float hours );
	/// Renders the viewport through the post-processing compositor until this object is deleted.
	void enablePostProcessing( Ogre::Viewport* viewport );

	/// Direction the sunlight travels (from the sun towards the ground).
	Ogre::Vector3 getLightDirection(void);
	Ogre::ColourValue getSunColour(void);
	Ogre::ColourValue getAmbientColour(void);

	bool frameRenderingQueued(const Ogre::FrameEvent &evt);
	/// Before the god-ray pass: camera and shadow-map matrices, and the shadow map itself.
	void notifyMaterialRender( Ogre::uint32 passId, Ogre::MaterialPtr &material );

private:
	struct Keyframe{
		Ogre::Real hour, sunElevation, sunAzimuth, rimStrength, exposure, shaftStrength;
		Ogre::ColourValue sunColour, skyColour, groundColour, shadowColour, rimColour;
	};

	Keyframe loadKeyframe( const Ogre::String &name );
	Keyframe blend( const Keyframe &a, const Keyframe &b, Ogre::Real t );
	void apply( const Keyframe &k );

	Ogre::SceneManager* mSceneMgr;
	Ogre::Light* sun;
	Sky* sky;
	Clock* clock;
	Ogre::Viewport* postProcessViewport;
	Ogre::CompositorInstance* postProcess;

	std::vector<Keyframe> keyframes;
	Keyframe current;

	Ogre::Real
		rampThreshold,
		rampSoftness,
		rimPower,
		specularSoftness,
		saturation,
		contrast,
		vignette,
		shadowBias,
		shadowSoftness,
		shadowNormalOffset,
		bloomThreshold,
		bloomSoftKnee,
		moonFrom,
		moonUntil,
		bloomStrength,
		aoAmbient,
		aoDirect,
		aoTint,
		shaftStrength,
		shaftDistance,
		shaftAnisotropy,
		shaftSteps;

	static Lighting* instance;
};

#endif // #ifndef __Lighting_h_

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
	/// The robot preview (inventory, new part) renders with neutral lighting, whatever the time of day or the arena's
	/// lights: save the shared values and set neutral ones before it renders, restore them afterwards.
	static void beginNeutralLighting(void);
	static void endNeutralLighting(void);
	/// SMAA's area and search lookup textures (Tumbu/SMAA/AreaTex, SearchTex). Must run before the resource groups load.
	static void createSMAATextures(void);

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
	/// Sun position for the lens flare in the final pass.
	void setLensFlare( Ogre::MaterialPtr &material );
	/// Matrices, radius and strength for the screen-space AO pass.
	void setAmbientOcclusion( Ogre::MaterialPtr &material );
	/// How much of the sun disc the camera sees (physics rays), eased over time.
	void updateSunVisibility( Ogre::Real time );
	/// Robots' feet positions for the contact shadows (shared parameters contactShadowA/B).
	void updateContactShadows(void);
	/// The torches' light this frame: each one's intensity (keyframe lamps x its flicker) and the shared time.
	void updateLamps( Ogre::Real time );
	Ogre::Real sunVisibility;

private:
	struct Keyframe{
		Ogre::Real hour, sunElevation, sunAzimuth, rimStrength, exposure, shaftStrength, heroFillStrength, neon, lamps, stars, cloudCover;
		Ogre::ColourValue sunColour, skyColour, groundColour, shadowColour, rimColour, heroFillColour, outlineTint,
			skyZenith, skyMid, skyHorizon, cloudLit, cloudShade;
	};

	Keyframe loadKeyframe( const Ogre::String &name );
	Keyframe blend( const Keyframe &a, const Keyframe &b, Ogre::Real t );
	void apply( const Keyframe &k );
	/// SMAA anti-aliasing after the post-processing (the Options setting, read at match start).
	void enableAntiAliasing( bool enable );
	/// The arena's night lights from lamps.object (torch positions and neon segments) into the shared parameters.
	void loadLamps(void);

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
		neonStrength,
		sunDiscBrightness,
		shaftDistance,
		shaftAnisotropy,
		shaftSteps,
		lensFlare,
		contactShadow,
		contactShadowRadius,
		fogStart,
		fogDensity,
		fogMax,
		fogBrightness,
		dustSunlight,
		dustShadow,
		ssaoRadius,
		ssaoStrength,
		heroFillSunlit,
		heroRimShadow,
		lampStrength,
		lampReach,
		lampBands,
		lampBack,
		lampFlicker,
		flameBrightness,
		neonLight,
		neonLightReach,
		neonLightBands,
		lampTime,
		neonRadius,
		neonTop;
	/// The arena's night lights (lamps.object): the torches' light positions; colours from lighting.object.
	std::vector<Ogre::Vector3> lamps;
	Ogre::ColourValue lampColour, neonLightColour;
	/// Direction of the robots' fill light in camera axes: weights of the camera's right, up and backwards
	/// (towards the viewer) vectors, from heroFillYaw / heroFillPitch.
	Ogre::Vector3 heroFillAxes;
	/// Robot metal (see lighting.object): reflection band brightness (sky, bright band, horizon line, ground), band
	/// heights + streak strength, and glint strength, glint size, fresnel minimum, how much metal dims the diffuse.
	Ogre::Vector4 metalEnv, metalShape, metalExtra;

	static Lighting* instance;
};

#endif // #ifndef __Lighting_h_

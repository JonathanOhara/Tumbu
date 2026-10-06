#include "Lighting.h"
#include "Clock.h"
#include "Sky.h"
#include "ConfigScript.h"
#include "TUMBU.h"
#include "Demo.h"
#include "Physics.h"
#include "smaa/AreaTex.h"
#include "smaa/SearchTex.h"
Lighting* Lighting::instance = NULL;
static const char* SHARED_PARAMS = "TumbuLighting";
static const char* POST_PROCESS = "Tumbu/PostProcess";
static const char* SMAA = "Tumbu/SMAA";
//-------------------------------------------------------------------------------------
static ConfigNode* requireChild( ConfigNode* node, const Ogre::String &script, const Ogre::String &key ){
	ConfigNode* child = node->findChild( key );
	if( child == NULL ){
		OGRE_EXCEPT( Ogre::Exception::ERR_ITEM_NOT_FOUND, "lighting.object: '" + key + "' is missing in 'lighting " + script + "'", "Lighting" );
	}
	return child;
}
//-------------------------------------------------------------------------------------
static ConfigNode* requireScript( const Ogre::String &name ){
	ConfigNode* node = ConfigScriptLoader::getSingleton().getConfigScript( "lighting", name );
	if( node == NULL ){
		OGRE_EXCEPT( Ogre::Exception::ERR_ITEM_NOT_FOUND, "lighting.object: 'lighting " + name + "' does not exist", "Lighting" );
	}
	return node;
}
//-------------------------------------------------------------------------------------
static Ogre::ColourValue readColour( ConfigNode* node, const Ogre::String &script, const Ogre::String &key ){
	ConfigNode* child = requireChild( node, script, key );
	return Ogre::ColourValue( child->getValueF( 0 ), child->getValueF( 1 ), child->getValueF( 2 ) );
}
//-------------------------------------------------------------------------------------
static Ogre::Vector4 toVector4( const Ogre::ColourValue &c, Ogre::Real w ){
	return Ogre::Vector4( c.r, c.g, c.b, w );
}
//-------------------------------------------------------------------------------------
// Robots' fill light direction in camera axes (right, up, backwards towards the viewer): yaw degrees to the
// camera's right and pitch degrees above it.
static Ogre::Vector3 heroFillDirection( Ogre::Real yaw, Ogre::Real pitch ){
	Ogre::Radian y = Ogre::Degree( yaw );
	Ogre::Radian p = Ogre::Degree( pitch );
	return Ogre::Vector3( Ogre::Math::Sin( y ) * Ogre::Math::Cos( p ), Ogre::Math::Sin( p ), Ogre::Math::Cos( y ) * Ogre::Math::Cos( p ) );
}
//-------------------------------------------------------------------------------------
Lighting::Lighting( Ogre::SceneManager* sceneMgr ){
	mSceneMgr = sceneMgr;
	sun = NULL;
	sky = NULL;
	clock = NULL;
	postProcessViewport = NULL;
	postProcess = NULL;
	sunVisibility = 0;

	const Ogre::String name = "configuration";
	ConfigNode* cfg = requireScript( name );
	rampThreshold		= requireChild( cfg, name, "rampThreshold" )->getValueF();
	rampSoftness		= requireChild( cfg, name, "rampSoftness" )->getValueF();
	rimPower			= requireChild( cfg, name, "rimPower" )->getValueF();
	specularSoftness	= requireChild( cfg, name, "specularSoftness" )->getValueF();
	saturation			= requireChild( cfg, name, "saturation" )->getValueF();
	contrast			= requireChild( cfg, name, "contrast" )->getValueF();
	vignette			= requireChild( cfg, name, "vignette" )->getValueF();
	shadowBias			= requireChild( cfg, name, "shadowBias" )->getValueF();
	shadowSoftness		= requireChild( cfg, name, "shadowSoftness" )->getValueF();
	shadowNormalOffset	= requireChild( cfg, name, "shadowNormalOffset" )->getValueF();
	bloomThreshold		= requireChild( cfg, name, "bloomThreshold" )->getValueF();
	bloomSoftKnee		= requireChild( cfg, name, "bloomSoftKnee" )->getValueF();
	bloomStrength		= requireChild( cfg, name, "bloomStrength" )->getValueF();
	aoAmbient			= requireChild( cfg, name, "aoAmbient" )->getValueF();
	aoDirect			= requireChild( cfg, name, "aoDirect" )->getValueF();
	aoTint				= requireChild( cfg, name, "aoTint" )->getValueF();
	moonFrom			= requireChild( cfg, name, "moonFrom" )->getValueF();
	moonUntil			= requireChild( cfg, name, "moonUntil" )->getValueF();
	shaftStrength		= requireChild( cfg, name, "shaftStrength" )->getValueF();
	shaftDistance		= requireChild( cfg, name, "shaftDistance" )->getValueF();
	shaftAnisotropy		= requireChild( cfg, name, "shaftAnisotropy" )->getValueF();
	shaftSteps			= std::min( 32.0f, std::max( 4.0f, requireChild( cfg, name, "shaftSteps" )->getValueF() ) );
	lensFlare			= requireChild( cfg, name, "lensFlare" )->getValueF();
	contactShadow		= requireChild( cfg, name, "contactShadow" )->getValueF();
	contactShadowRadius	= requireChild( cfg, name, "contactShadowRadius" )->getValueF();
	fogStart			= requireChild( cfg, name, "fogStart" )->getValueF();
	fogDensity			= requireChild( cfg, name, "fogDensity" )->getValueF();
	fogMax				= requireChild( cfg, name, "fogMax" )->getValueF();
	fogBrightness		= requireChild( cfg, name, "fogBrightness" )->getValueF();
	dustSunlight		= requireChild( cfg, name, "dustSunlight" )->getValueF();
	dustShadow			= requireChild( cfg, name, "dustShadow" )->getValueF();
	ConfigNode* neonNode	= cfg->findChild( "neonStrength" );	// optional: the arena's neon (ring ropes, the T's tube)
	neonStrength		= neonNode != NULL ? neonNode->getValueF() : 4.0f;
	sunDiscBrightness	= requireChild( cfg, name, "sunDiscBrightness" )->getValueF();
	ssaoRadius			= requireChild( cfg, name, "ssaoRadius" )->getValueF();
	ssaoStrength		= requireChild( cfg, name, "ssaoStrength" )->getValueF();
	heroFillSunlit		= requireChild( cfg, name, "heroFillSunlit" )->getValueF();
	heroRimShadow		= requireChild( cfg, name, "heroRimShadow" )->getValueF();
	heroFillAxes		= heroFillDirection( requireChild( cfg, name, "heroFillYaw" )->getValueF(), requireChild( cfg, name, "heroFillPitch" )->getValueF() );
	ConfigNode* bands	= requireChild( cfg, name, "metalBands" );
	ConfigNode* heights	= requireChild( cfg, name, "metalBandHeights" );
	ConfigNode* glint	= requireChild( cfg, name, "metalGlint" );
	metalEnv	= Ogre::Vector4( bands->getValueF( 0 ), bands->getValueF( 1 ), bands->getValueF( 2 ), bands->getValueF( 3 ) );
	metalShape	= Ogre::Vector4( heights->getValueF( 0 ), heights->getValueF( 1 ), heights->getValueF( 2 ), requireChild( cfg, name, "metalStreak" )->getValueF() );
	metalExtra	= Ogre::Vector4( glint->getValueF( 0 ), glint->getValueF( 1 ), requireChild( cfg, name, "metalFresnel" )->getValueF(), requireChild( cfg, name, "metalDiffuse" )->getValueF() );

	std::vector<Ogre::String> &names = requireChild( cfg, name, "keyframes" )->getValues();
	for( size_t i = 0; i < names.size(); i++ ){
		keyframes.push_back( loadKeyframe( names[i] ) );
		if( i > 0 && keyframes[i].hour <= keyframes[i - 1].hour ){
			OGRE_EXCEPT( Ogre::Exception::ERR_INVALIDPARAMS, "lighting.object: the keyframes must be listed in time order ('" + names[i] + "')", "Lighting" );
		}
	}
	if( keyframes.empty() ){
		OGRE_EXCEPT( Ogre::Exception::ERR_INVALIDPARAMS, "lighting.object: no keyframes", "Lighting" );
	}
	current = keyframes[0];
}
//-------------------------------------------------------------------------------------
Lighting::~Lighting(void){
	if( postProcessViewport != NULL ){
		postProcess->removeListener( this );
		// Disabled, not removed: the next match enables it again with the same render targets (no rebuild).
		Ogre::CompositorManager::getSingleton().setCompositorEnabled( postProcessViewport, POST_PROCESS, false );
		enableAntiAliasing( false );
	}
	instance = NULL;
}
//-------------------------------------------------------------------------------------
Lighting* Lighting::getInstance(){
	if( instance == NULL ){
		instance = new Lighting( TUMBU::getInstance()->mSceneMgr );
	}
	return instance;
}
//-------------------------------------------------------------------------------------
void Lighting::declareSharedParameters(void){
	Ogre::GpuSharedParametersPtr params = Ogre::GpuProgramManager::getSingleton().createSharedParameters( SHARED_PARAMS );
	const char* names[] = { "sunDirection", "sunColour", "skyColour", "groundColour", "shadowColour", "rimColour", "toonParams", "shadowParams", "postParams", "bloomParams", "aoParams", "shadowOffset", "shaftParams", "contactShadowA", "contactShadowB", "fogParams", "dustParams",
		// Special-attack effects (EffectsManager): energy lights and the screen flash.
		"energyLightPos0", "energyLightPos1", "energyLightPos2", "energyLightPos3",
		"energyLightColour0", "energyLightColour1", "energyLightColour2", "energyLightColour3", "screenFlash",
		// Robot "hero lighting": the fill light that follows the camera, and the rim in shadow.
		"heroFillColour", "heroFillParams", "heroRimParams", "outlineTint",
		// Arena neon (ring ropes, the T's tube): x = how much it glows (keyframe neon), y = brightness of the core.
		"neonParams",
		// Arena detail: x = 1 for the stone's parallax and self-shadows, 0 for the normal map only (shadows off).
		"detailParams",
		// The painted toon sky (sky.frag): band colours, cloud colours (w = cover), x moon y stars z quality w sun brightness.
		"skyZenith", "skyMid", "skyHorizon", "cloudLit", "cloudShade", "skyParams",
		// Robot metal: the toon sky reflection and the streak.
		"metalEnv", "metalShape", "metalExtra" };
	for( size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++ ){
		params->addConstantDefinition( names[i], Ogre::GCT_FLOAT4 );
	}
	// Neutral values until a match applies lighting.object (the inventory preview may render robots first).
	params->setNamedConstant( "sunDirection", Ogre::Vector4( 0.3f, 0.8f, 0.5f, 0 ) );
	params->setNamedConstant( "sunColour", Ogre::Vector4( 1, 1, 1, 1 ) );
	params->setNamedConstant( "skyColour", Ogre::Vector4( 0.4f, 0.4f, 0.45f, 1 ) );
	params->setNamedConstant( "groundColour", Ogre::Vector4( 0.25f, 0.25f, 0.25f, 1 ) );
	params->setNamedConstant( "shadowColour", Ogre::Vector4( 0.45f, 0.45f, 0.6f, 1 ) );
	params->setNamedConstant( "rimColour", Ogre::Vector4( 1, 1, 1, 0.3f ) );
	params->setNamedConstant( "toonParams", Ogre::Vector4( 0.5f, 0.06f, 3.5f, 0.05f ) );
	params->setNamedConstant( "shadowParams", Ogre::Vector4( 0, 0, 0, 0 ) );
	params->setNamedConstant( "postParams", Ogre::Vector4( 1, 1, 1, 0 ) );
	params->setNamedConstant( "bloomParams", Ogre::Vector4( 1.5f, 0.5f, 0, 0 ) );
	params->setNamedConstant( "aoParams", Ogre::Vector4( 1, 0.5f, 0.5f, 0 ) );
	params->setNamedConstant( "shadowOffset", Ogre::Vector4( 0, 0, 0, 0 ) );
	params->setNamedConstant( "shaftParams", Ogre::Vector4( 0, 40, 0.5f, 16 ) );
	params->setNamedConstant( "contactShadowA", Ogre::Vector4( 0, 0, 0, 0 ) );
	params->setNamedConstant( "contactShadowB", Ogre::Vector4( 0, 0, 0, 0 ) );
	params->setNamedConstant( "fogParams", Ogre::Vector4( 1000, 0, 0, 1 ) );
	params->setNamedConstant( "dustParams", Ogre::Vector4( 0, 0, 0, 0 ) );
	for( int i = 0; i < 4; i++ ){
		params->setNamedConstant( "energyLightPos" + Ogre::StringConverter::toString( i ), Ogre::Vector4( 0, 0, 0, 0 ) );
		params->setNamedConstant( "energyLightColour" + Ogre::StringConverter::toString( i ), Ogre::Vector4( 0, 0, 0, 0 ) );
	}
	params->setNamedConstant( "screenFlash", Ogre::Vector4( 1, 1, 1, 0 ) );
	Ogre::Vector3 fill = heroFillDirection( 40, 35 );
	params->setNamedConstant( "heroFillColour", Ogre::Vector4( 0.2f, 0.2f, 0.22f, 1 ) );
	params->setNamedConstant( "heroFillParams", Ogre::Vector4( fill.x, fill.y, fill.z, 0.25f ) );
	params->setNamedConstant( "heroRimParams", Ogre::Vector4( 0.35f, 0, 0, 0 ) );
	params->setNamedConstant( "outlineTint", Ogre::Vector4( 1, 1, 1, 1 ) );
	params->setNamedConstant( "neonParams", Ogre::Vector4( 0, 4, 0, 0 ) );
	params->setNamedConstant( "detailParams", Ogre::Vector4( 1, 0, 0, 0 ) );
	params->setNamedConstant( "skyZenith", Ogre::Vector4( 0.3f, 0.5f, 0.85f, 1 ) );
	params->setNamedConstant( "skyMid", Ogre::Vector4( 0.45f, 0.68f, 0.95f, 1 ) );
	params->setNamedConstant( "skyHorizon", Ogre::Vector4( 0.8f, 0.9f, 0.98f, 1 ) );
	params->setNamedConstant( "cloudLit", Ogre::Vector4( 1, 1, 1, 0 ) );
	params->setNamedConstant( "cloudShade", Ogre::Vector4( 0.7f, 0.78f, 0.92f, 1 ) );
	params->setNamedConstant( "skyParams", Ogre::Vector4( 0, 0, 0, 2.5f ) );
	params->setNamedConstant( "metalEnv", Ogre::Vector4( 1.7f, 2.5f, 0.35f, 1.5f ) );
	params->setNamedConstant( "metalShape", Ogre::Vector4( 0.38f, 0.06f, -0.1f, 0.75f ) );
	params->setNamedConstant( "metalExtra", Ogre::Vector4( 1.6f, 0.965f, 0.55f, 0.55f ) );
}
//-------------------------------------------------------------------------------------
Lighting::Keyframe Lighting::loadKeyframe( const Ogre::String &name ){
	ConfigNode* node = requireScript( name );
	Keyframe k;
	k.hour			= requireChild( node, name, "hour" )->getValueF();
	k.sunElevation	= requireChild( node, name, "sunElevation" )->getValueF();
	k.sunAzimuth	= requireChild( node, name, "sunAzimuth" )->getValueF();
	k.rimStrength	= requireChild( node, name, "rimStrength" )->getValueF();
	k.exposure		= requireChild( node, name, "exposure" )->getValueF();
	ConfigNode* shafts = node->findChild( "shaftStrength" );	// optional: 1 when missing
	k.shaftStrength	= shafts != NULL ? shafts->getValueF() : 1.0f;
	k.heroFillStrength	= requireChild( node, name, "heroFillStrength" )->getValueF();
	ConfigNode* neon = node->findChild( "neon" );	// optional: 0 (the neon is off) when missing
	k.neon			= neon != NULL ? neon->getValueF() : 0.0f;
	ConfigNode* stars = node->findChild( "stars" );	// optional: 0 (no stars) when missing
	k.stars			= stars != NULL ? stars->getValueF() : 0.0f;
	k.cloudCover	= requireChild( node, name, "cloudCover" )->getValueF();
	k.skyZenith		= readColour( node, name, "skyZenith" );
	k.skyMid		= readColour( node, name, "skyMid" );
	k.skyHorizon	= readColour( node, name, "skyHorizon" );
	k.cloudLit		= readColour( node, name, "cloudLit" );
	k.cloudShade	= readColour( node, name, "cloudShade" );
	k.heroFillColour	= readColour( node, name, "heroFillColour" );
	ConfigNode* tint = node->findChild( "outlineTint" );	// optional: white (no tint) when missing
	k.outlineTint	= tint != NULL ? readColour( node, name, "outlineTint" ) : Ogre::ColourValue::White;
	k.sunColour		= readColour( node, name, "sunColour" );
	k.skyColour		= readColour( node, name, "skyColour" );
	k.groundColour	= readColour( node, name, "groundColour" );
	k.shadowColour	= readColour( node, name, "shadowColour" );
	k.rimColour		= readColour( node, name, "rimColour" );
	return k;
}
//-------------------------------------------------------------------------------------
Lighting::Keyframe Lighting::blend( const Keyframe &a, const Keyframe &b, Ogre::Real t ){
	Keyframe k;
	k.hour			= a.hour;
	k.sunElevation	= Ogre::Math::lerp( a.sunElevation, b.sunElevation, t );
	// Azimuth along the shorter way round.
	Ogre::Real turn = std::fmod( b.sunAzimuth - a.sunAzimuth + 540.0f, 360.0f ) - 180.0f;
	k.sunAzimuth	= a.sunAzimuth + turn * t;
	k.rimStrength	= Ogre::Math::lerp( a.rimStrength, b.rimStrength, t );
	k.exposure		= Ogre::Math::lerp( a.exposure, b.exposure, t );
	k.shaftStrength	= Ogre::Math::lerp( a.shaftStrength, b.shaftStrength, t );
	k.heroFillStrength	= Ogre::Math::lerp( a.heroFillStrength, b.heroFillStrength, t );
	k.neon			= Ogre::Math::lerp( a.neon, b.neon, t );
	k.stars			= Ogre::Math::lerp( a.stars, b.stars, t );
	k.cloudCover	= Ogre::Math::lerp( a.cloudCover, b.cloudCover, t );
	k.skyZenith		= Ogre::Math::lerp( a.skyZenith, b.skyZenith, t );
	k.skyMid		= Ogre::Math::lerp( a.skyMid, b.skyMid, t );
	k.skyHorizon	= Ogre::Math::lerp( a.skyHorizon, b.skyHorizon, t );
	k.cloudLit		= Ogre::Math::lerp( a.cloudLit, b.cloudLit, t );
	k.cloudShade	= Ogre::Math::lerp( a.cloudShade, b.cloudShade, t );
	k.heroFillColour	= Ogre::Math::lerp( a.heroFillColour, b.heroFillColour, t );
	k.outlineTint	= Ogre::Math::lerp( a.outlineTint, b.outlineTint, t );
	k.sunColour		= Ogre::Math::lerp( a.sunColour, b.sunColour, t );
	k.skyColour		= Ogre::Math::lerp( a.skyColour, b.skyColour, t );
	k.groundColour	= Ogre::Math::lerp( a.groundColour, b.groundColour, t );
	k.shadowColour	= Ogre::Math::lerp( a.shadowColour, b.shadowColour, t );
	k.rimColour		= Ogre::Math::lerp( a.rimColour, b.rimColour, t );
	return k;
}
//-------------------------------------------------------------------------------------
void Lighting::setClock( Clock* _clock ){
	clock = _clock;
	if( clock != NULL ){
		update( clock->getHours() );
	}
}
//-------------------------------------------------------------------------------------
void Lighting::setSun( Ogre::Light* _sun ){
	sun = _sun;
	sun->setType( Ogre::Light::LT_DIRECTIONAL );
	sun->setCastShadows( TUMBU::getInstance()->isCastShadows() );
	apply( current );
}
//-------------------------------------------------------------------------------------
void Lighting::setSky( Sky* _sky ){
	sky = _sky;
	apply( current );
}
//-------------------------------------------------------------------------------------
void Lighting::update( float hours ){
	// The two keyframes around the hour; after the last one it blends back into the first (next day).
	size_t next = 0;
	while( next < keyframes.size() && keyframes[next].hour <= hours ){
		next++;
	}
	const Keyframe &b = keyframes[next % keyframes.size()];
	const Keyframe &a = keyframes[( next + keyframes.size() - 1 ) % keyframes.size()];

	Ogre::Real span = b.hour - a.hour;
	Ogre::Real elapsed = hours - a.hour;
	if( span <= 0 ) span += 24;
	if( elapsed < 0 ) elapsed += 24;
	Ogre::Real t = Ogre::Math::saturate( elapsed / span );
	// Ease in and out, so the light rests at each keyframe instead of turning at a constant speed.
	t = t * t * ( 3 - 2 * t );

	current = blend( a, b, t );
	current.hour = hours;
	apply( current );
}
//-------------------------------------------------------------------------------------
Ogre::Vector3 Lighting::getLightDirection(void){
	Ogre::Radian elevation = Ogre::Degree( current.sunElevation );
	Ogre::Radian azimuth = Ogre::Degree( current.sunAzimuth );
	Ogre::Vector3 towardsSun( Ogre::Math::Cos( elevation ) * Ogre::Math::Sin( azimuth ),
		Ogre::Math::Sin( elevation ),
		Ogre::Math::Cos( elevation ) * Ogre::Math::Cos( azimuth ) );
	return -towardsSun.normalisedCopy();
}
//-------------------------------------------------------------------------------------
Ogre::ColourValue Lighting::getSunColour(void){
	return current.sunColour;
}
//-------------------------------------------------------------------------------------
Ogre::ColourValue Lighting::getAmbientColour(void){
	return ( current.skyColour + current.groundColour ) * 0.5f;
}
//-------------------------------------------------------------------------------------
void Lighting::apply( const Keyframe &k ){
	Ogre::Vector3 towardsSun = -getLightDirection();

	Ogre::GpuSharedParametersPtr params = Ogre::GpuProgramManager::getSingleton().getSharedParameters( SHARED_PARAMS );
	params->setNamedConstant( "sunDirection", Ogre::Vector4( towardsSun.x, towardsSun.y, towardsSun.z, 0 ) );
	params->setNamedConstant( "sunColour", toVector4( k.sunColour, 1 ) );
	params->setNamedConstant( "skyColour", toVector4( k.skyColour, 1 ) );
	params->setNamedConstant( "groundColour", toVector4( k.groundColour, 1 ) );
	params->setNamedConstant( "shadowColour", toVector4( k.shadowColour, 1 ) );
	params->setNamedConstant( "rimColour", toVector4( k.rimColour, k.rimStrength ) );
	params->setNamedConstant( "toonParams", Ogre::Vector4( rampThreshold, rampSoftness, rimPower, specularSoftness ) );
	params->setNamedConstant( "postParams", Ogre::Vector4( k.exposure, saturation, contrast, vignette ) );
	params->setNamedConstant( "bloomParams", Ogre::Vector4( bloomThreshold, bloomSoftKnee, bloomStrength, 0 ) );
	params->setNamedConstant( "aoParams", Ogre::Vector4( aoAmbient, aoDirect, aoTint, contactShadow ) );
	params->setNamedConstant( "fogParams", Ogre::Vector4( fogStart, fogDensity, fogMax, fogBrightness ) );
	// Robots only: the fill light that follows the camera, and how much rim stays on the side the sun misses.
	params->setNamedConstant( "heroFillColour", toVector4( k.heroFillColour * k.heroFillStrength, 1 ) );
	params->setNamedConstant( "heroFillParams", Ogre::Vector4( heroFillAxes.x, heroFillAxes.y, heroFillAxes.z, heroFillSunlit ) );
	params->setNamedConstant( "heroRimParams", Ogre::Vector4( heroRimShadow, 0, 0, 0 ) );
	params->setNamedConstant( "outlineTint", toVector4( k.outlineTint, 1 ) );
	params->setNamedConstant( "neonParams", Ogre::Vector4( k.neon, neonStrength, 0, 0 ) );
	params->setNamedConstant( "metalEnv", metalEnv );
	params->setNamedConstant( "metalShape", metalShape );
	params->setNamedConstant( "metalExtra", metalExtra );
	// The dust follows the god rays' strength through the day (stronger at dawn and sunset, faint at night).
	params->setNamedConstant( "dustParams", Ogre::Vector4( dustSunlight * k.shaftStrength, dustShadow, 0, 0 ) );

	// The shaders sample the shadow map only when the scene renders one (Options: shadows).
	bool shadows = sun != NULL && sun->getCastShadows() && mSceneMgr->isShadowTechniqueTextureBased();
	Ogre::Real texelSize = shadows ? 1.0f / mSceneMgr->getShadowTextureConfigList()[0].width : 0.0f;
	params->setNamedConstant( "shadowParams", Ogre::Vector4( shadows ? 1.0f : 0.0f, shadowBias, texelSize, shadowSoftness ) );
	params->setNamedConstant( "shadowOffset", Ogre::Vector4( shadowNormalOffset, 0, 0, 0 ) );
	// The low setting (shadows off) also drops the stone's parallax and self-shadows; the normal map stays.
	params->setNamedConstant( "detailParams", Ogre::Vector4( shadows ? 1.0f : 0.0f, 0, 0, 0 ) );
	params->setNamedConstant( "shaftParams", Ogre::Vector4( shaftStrength * k.shaftStrength, shaftDistance, shaftAnisotropy, shaftSteps ) );

	// Materials lit by the shader generator (terrain, particles) use the scene's ambient light and the sun.
	Ogre::ColourValue ambient = getAmbientColour();
	mSceneMgr->setAmbientLight( ambient );
	if( sun != NULL ){
		sun->getParentSceneNode()->setDirection( -towardsSun, Ogre::Node::TS_WORLD );
		sun->setDiffuseColour( k.sunColour );
		sun->setSpecularColour( k.sunColour * 0.5f );
	}
	// The painted sky: between moonFrom and moonUntil (across midnight) the light plays the moon.
	bool moon = k.hour >= moonFrom || k.hour < moonUntil;
	params->setNamedConstant( "skyZenith", toVector4( k.skyZenith, 1 ) );
	params->setNamedConstant( "skyMid", toVector4( k.skyMid, 1 ) );
	params->setNamedConstant( "skyHorizon", toVector4( k.skyHorizon, 1 ) );
	params->setNamedConstant( "cloudLit", toVector4( k.cloudLit, k.cloudCover ) );
	params->setNamedConstant( "cloudShade", toVector4( k.cloudShade, 1 ) );
	params->setNamedConstant( "skyParams", Ogre::Vector4( moon ? 1.0f : 0.0f, k.stars, sky != NULL ? (Ogre::Real) sky->getQuality() : 1.0f, sunDiscBrightness ) );
}
//-------------------------------------------------------------------------------------
void Lighting::enablePostProcessing( Ogre::Viewport* viewport ){
	// The compositor stays on the viewport between matches (disabled), so it is created only once.
	Ogre::CompositorManager &manager = Ogre::CompositorManager::getSingleton();
	Ogre::CompositorInstance* compositor = manager.hasCompositorChain( viewport ) ? manager.getCompositorChain( viewport )->getCompositor( POST_PROCESS ) : NULL;
	if( compositor == NULL ){
		compositor = manager.addCompositor( viewport, POST_PROCESS );
	}
	if( compositor == NULL ){
		Ogre::LogManager::getSingleton().logError( "Lighting: the post-processing compositor is not supported, the scene renders without tone mapping" );
		return;
	}
	compositor->addListener( this );
	postProcess = compositor;
	Ogre::CompositorManager::getSingleton().setCompositorEnabled( viewport, POST_PROCESS, true );
	postProcessViewport = viewport;
	Ogre::LogManager::getSingleton().logMessage( "Lighting: post-processing enabled" );
	enableAntiAliasing( TUMBU::getInstance()->isAntiAliasingEnabled() );
}
//-------------------------------------------------------------------------------------
void Lighting::createSMAATextures(void){
	// SMAA's precomputed lookup textures (third_party/smaa), made from the reference bytes: no image codec, no gamma,
	// no mipmaps. Created once at start-up (BaseApplication::loadResources) and kept for the whole run.
	Ogre::TextureManager &manager = Ogre::TextureManager::getSingleton();
	if( manager.resourceExists( "Tumbu/SMAA/AreaTex", "Game" ) ){
		return;
	}
	// The area texture is two-channel (RG8), which Ogre's Direct3D 11 renderer does not map: it converted it to RGBA
	// itself and the weights came out wrong (dashed outlines). So it is expanded to RGBA here, the same on every renderer.
	std::vector<unsigned char> rgba( AREATEX_WIDTH * AREATEX_HEIGHT * 4 );
	for( size_t i = 0; i < AREATEX_WIDTH * AREATEX_HEIGHT; i++ ){
		rgba[i * 4] = areaTexBytes[i * 2];
		rgba[i * 4 + 1] = areaTexBytes[i * 2 + 1];
		rgba[i * 4 + 2] = 0;
		rgba[i * 4 + 3] = 255;
	}
	Ogre::TexturePtr area = manager.createManual( "Tumbu/SMAA/AreaTex", "Game", Ogre::TEX_TYPE_2D,
		AREATEX_WIDTH, AREATEX_HEIGHT, 0, Ogre::PF_BYTE_RGBA );
	area->getBuffer()->blitFromMemory( Ogre::PixelBox( AREATEX_WIDTH, AREATEX_HEIGHT, 1, Ogre::PF_BYTE_RGBA, &rgba[0] ) );
	Ogre::TexturePtr search = manager.createManual( "Tumbu/SMAA/SearchTex", "Game", Ogre::TEX_TYPE_2D,
		SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, 0, Ogre::PF_R8 );
	search->getBuffer()->blitFromMemory( Ogre::PixelBox( SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, 1, Ogre::PF_R8, (void*) searchTexBytes ) );
}
//-------------------------------------------------------------------------------------
void Lighting::enableAntiAliasing( bool enable ){
	// SMAA 1x (postprocess.compositor, Tumbu/SMAA): chained after Tumbu/PostProcess, whose final pass then renders
	// into SMAA's colour texture instead of the window. Off, the chain is exactly Tumbu/PostProcess.
	Ogre::CompositorManager &manager = Ogre::CompositorManager::getSingleton();
	Ogre::CompositorInstance* smaa = manager.getCompositorChain( postProcessViewport )->getCompositor( SMAA );
	if( enable && smaa == NULL ){
		Ogre::Pass* weights = Ogre::MaterialManager::getSingleton().getByName( "Tumbu/SMAA/Weights", "Game" )->getTechnique( 0 )->getPass( 0 );
		weights->getTextureUnitState( "area" )->setTextureName( "Tumbu/SMAA/AreaTex" );
		weights->getTextureUnitState( "search" )->setTextureName( "Tumbu/SMAA/SearchTex" );
		smaa = manager.addCompositor( postProcessViewport, SMAA );
		if( smaa == NULL ){
			Ogre::LogManager::getSingleton().logError( "Lighting: the SMAA compositor is not supported, the match renders without anti-aliasing" );
			return;
		}
	}
	if( smaa != NULL ){
		manager.setCompositorEnabled( postProcessViewport, SMAA, enable );
	}
	Ogre::LogManager::getSingleton().logMessage( Ogre::String( "Lighting: anti-aliasing " ) + ( enable ? "SMAA" : "off" ) );
}
//-------------------------------------------------------------------------------------
bool Lighting::frameRenderingQueued( const Ogre::FrameEvent &evt ){
	if( clock != NULL && TUMBU::getInstance()->isPlaying() ){
		update( clock->getHours() );
	}
	updateSunVisibility( evt.timeSinceLastFrame );
	updateContactShadows();
	return true;
}
//-------------------------------------------------------------------------------------
void Lighting::notifyMaterialRender( Ogre::uint32 passId, Ogre::MaterialPtr &material ){
	if( postProcessViewport == NULL ){
		return;
	}
	if( passId == 20 ){
		setLensFlare( material );
		return;
	}
	if( passId == 30 ){
		setAmbientOcclusion( material );
		return;
	}
	if( passId != 10 ){
		return;
	}
	// God-ray pass (postprocess.compositor, identifier 10): set here, right before it renders, so the camera and
	// the focused shadow camera of this very frame are used (no lag when the camera moves).
	Ogre::Pass* pass = material->getBestTechnique()->getPass( 0 );
	Ogre::GpuProgramParametersSharedPtr params = pass->getFragmentProgramParameters();
	Ogre::Camera* camera = postProcessViewport->getCamera();
	Ogre::Matrix4 viewProj = camera->getProjectionMatrixWithRSDepth() * camera->getViewMatrix();
	params->setNamedConstant( "invViewProj", viewProj.inverse() );
	// No "OpenGL upside down" flip when rebuilding positions from the depth texture: it mirrored every view ray on
	// OpenGL (same length, so distances and fog looked right) and the god rays marched into the sunlit air above.
	Ogre::Vector3 eye = camera->getDerivedPosition();
	params->setNamedConstant( "camPos", Ogre::Vector4( eye.x, eye.y, eye.z, 0 ) );

	if( mSceneMgr->isShadowTechniqueTextureBased() && sun != NULL && sun->getCastShadows() ){
		Ogre::TexturePtr shadowTexture = mSceneMgr->getShadowTexture( 0 );
		Ogre::Camera* shadowCamera = shadowTexture->getBuffer()->getRenderTarget()->getViewport( 0 )->getCamera();
		Ogre::Matrix4 shadowViewProj = Ogre::Matrix4::CLIPSPACE2DTOIMAGESPACE *
			shadowCamera->getProjectionMatrixWithRSDepth() * shadowCamera->getViewMatrix();
		params->setNamedConstant( "shadowViewProj", shadowViewProj );
		pass->getTextureUnitState( 1 )->_setTexturePtr( shadowTexture );
	}
}
//-------------------------------------------------------------------------------------
void Lighting::setLensFlare( Ogre::MaterialPtr &material ){
	// Final pass (identifier 20): where the sun is on screen, times how much of it is visible (sunVisibility). No
	// flare for the moon, or when the sun is behind the camera.
	Ogre::GpuProgramParametersSharedPtr params = material->getBestTechnique()->getPass( 0 )->getFragmentProgramParameters();
	Ogre::Camera* camera = postProcessViewport->getCamera();
	bool moon = current.hour >= moonFrom || current.hour < moonUntil;
	Ogre::Vector3 sunPoint = camera->getDerivedPosition() - getLightDirection() * 1000.0f;
	Ogre::Vector4 clip = camera->getProjectionMatrix() * ( camera->getViewMatrix() * Ogre::Vector4( sunPoint.x, sunPoint.y, sunPoint.z, 1.0f ) );
	Ogre::Vector4 flare( 0, 0, 0, 0 );
	if( !moon && lensFlare > 0 && clip.w > 0 ){
		flare.x = ( clip.x / clip.w ) * 0.5f + 0.5f;
		flare.y = 0.5f - ( clip.y / clip.w ) * 0.5f;
		flare.z = lensFlare * sunVisibility;
		flare.w = Ogre::Root::getSingleton().getRenderSystem()->getName().find( "OpenGL" ) != Ogre::String::npos ? 1.0f : 0.0f;
	}
	params->setNamedConstant( "flareSun", flare );
}
//-------------------------------------------------------------------------------------
void Lighting::updateSunVisibility( Ogre::Real time ){
	// Rays from the camera towards the sun (centre and four points on the disc) against the physics world: the
	// coliseum, the floor and the robots hide the sun. Eased over time, so the flare fades in and out.
	Demo* demo = TUMBU::getInstance()->getDemo();
	if( postProcessViewport == NULL || demo == NULL || demo->getPhysicWorld() == NULL ){
		return;
	}
	btCollisionWorld* world = demo->getPhysicWorld()->getBulletCollisionWorld();
	Ogre::Camera* camera = postProcessViewport->getCamera();
	Ogre::Vector3 eye = camera->getDerivedPosition();
	Ogre::Vector3 towardsSun = -getLightDirection();
	Ogre::Vector3 side = towardsSun.perpendicular();
	Ogre::Vector3 up = towardsSun.crossProduct( side );
	const Ogre::Real reach = 200.0f, spread = 2.0f;	// spread: about half a degree at 200 units
	const Ogre::Vector3 offsets[] = { Ogre::Vector3::ZERO, side * spread, -side * spread, up * spread, -up * spread };
	int clear = 0;
	for( int i = 0; i < 5; i++ ){
		btVector3 from = Physics::OgreBtConverter::to( eye );
		btVector3 to = Physics::OgreBtConverter::to( eye + towardsSun * reach + offsets[i] );
		btCollisionWorld::ClosestRayResultCallback hit( from, to );
		world->rayTest( from, to, hit );
		if( !hit.hasHit() ){
			clear++;
		}
	}
	Ogre::Real target = clear / 5.0f;
	sunVisibility += ( target - sunVisibility ) * std::min( 1.0f, time * 8.0f );
}
//-------------------------------------------------------------------------------------
void Lighting::updateContactShadows(void){
	// The robots' feet positions for the soft contact shadows of the arena shader (TumbuToon.h tumbuContact).
	Demo* demo = TUMBU::getInstance()->getDemo();
	Robot* robots[2] = { demo != NULL ? demo->mainChar : NULL, demo != NULL ? demo->enemy : NULL };
	const char* names[2] = { "contactShadowA", "contactShadowB" };
	Ogre::GpuSharedParametersPtr params = Ogre::GpuProgramManager::getSingleton().getSharedParameters( SHARED_PARAMS );
	for( int i = 0; i < 2; i++ ){
		Ogre::Vector4 value( 0, 0, 0, 0 );
		if( robots[i] != NULL && robots[i]->robotNode != NULL ){
			Ogre::Vector3 feet = robots[i]->robotNode->_getDerivedPosition();
			value = Ogre::Vector4( feet.x, feet.y, feet.z, contactShadowRadius );
		}
		params->setNamedConstant( names[i], value );
	}
}
//-------------------------------------------------------------------------------------
void Lighting::setAmbientOcclusion( Ogre::MaterialPtr &material ){
	// Screen-space AO pass (identifier 30): camera matrices of this frame, radius and strength.
	Ogre::GpuProgramParametersSharedPtr params = material->getBestTechnique()->getPass( 0 )->getFragmentProgramParameters();
	Ogre::Camera* camera = postProcessViewport->getCamera();
	Ogre::Matrix4 projection = camera->getProjectionMatrixWithRSDepth();
	params->setNamedConstant( "invViewProj", ( projection * camera->getViewMatrix() ).inverse() );
	Ogre::Vector3 eye = camera->getDerivedPosition();
	params->setNamedConstant( "camPos", Ogre::Vector4( eye.x, eye.y, eye.z, 1 ) );
	// projection[1][1] = 1 / tan(fov / 2): a world size at distance 1 covers that much of the screen height
	// (x 0.5 in texture coordinates).
	params->setNamedConstant( "ssaoParams", Ogre::Vector4( ssaoRadius, ssaoStrength, projection[1][1] * 0.5f, 0 ) );
}

#include "Lighting.h"
#include "Clock.h"
#include "ConfigScript.h"
#include "TUMBU.h"
Lighting* Lighting::instance = NULL;
static const char* SHARED_PARAMS = "TumbuLighting";
static const char* POST_PROCESS = "Tumbu/PostProcess";
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
Lighting::Lighting( Ogre::SceneManager* sceneMgr ){
	mSceneMgr = sceneMgr;
	sun = NULL;
	clock = NULL;
	postProcessViewport = NULL;

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
	bloomThreshold		= requireChild( cfg, name, "bloomThreshold" )->getValueF();
	bloomSoftKnee		= requireChild( cfg, name, "bloomSoftKnee" )->getValueF();
	bloomStrength		= requireChild( cfg, name, "bloomStrength" )->getValueF();

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
		Ogre::CompositorManager::getSingleton().setCompositorEnabled( postProcessViewport, POST_PROCESS, false );
		Ogre::CompositorManager::getSingleton().removeCompositor( postProcessViewport, POST_PROCESS );
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
	const char* names[] = { "sunDirection", "sunColour", "skyColour", "groundColour", "shadowColour", "rimColour", "toonParams", "shadowParams", "postParams", "bloomParams" };
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

	// The shaders sample the shadow map only when the scene renders one (Options: shadows).
	bool shadows = sun != NULL && sun->getCastShadows() && mSceneMgr->isShadowTechniqueTextureBased();
	Ogre::Real texelSize = shadows ? 1.0f / mSceneMgr->getShadowTextureConfigList()[0].width : 0.0f;
	params->setNamedConstant( "shadowParams", Ogre::Vector4( shadows ? 1.0f : 0.0f, shadowBias, texelSize, shadowSoftness ) );

	// Materials lit by the shader generator (terrain, particles) use the scene's ambient light and the sun.
	Ogre::ColourValue ambient = getAmbientColour();
	mSceneMgr->setAmbientLight( ambient );
	if( sun != NULL ){
		sun->getParentSceneNode()->setDirection( -towardsSun, Ogre::Node::TS_WORLD );
		sun->setDiffuseColour( k.sunColour );
		sun->setSpecularColour( k.sunColour * 0.5f );
	}
}
//-------------------------------------------------------------------------------------
void Lighting::enablePostProcessing( Ogre::Viewport* viewport ){
	Ogre::CompositorInstance* compositor = Ogre::CompositorManager::getSingleton().addCompositor( viewport, POST_PROCESS );
	if( compositor == NULL ){
		Ogre::LogManager::getSingleton().logError( "Lighting: the post-processing compositor is not supported, the scene renders without tone mapping" );
		return;
	}
	Ogre::CompositorManager::getSingleton().setCompositorEnabled( viewport, POST_PROCESS, true );
	postProcessViewport = viewport;
	Ogre::LogManager::getSingleton().logMessage( "Lighting: post-processing enabled" );
}
//-------------------------------------------------------------------------------------
bool Lighting::frameRenderingQueued( const Ogre::FrameEvent &evt ){
	if( clock != NULL && TUMBU::getInstance()->isPlaying() ){
		update( clock->getHours() );
	}
	return true;
}

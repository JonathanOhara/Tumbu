#include "ConfigScript.h"
#include "Part.h"
#include "TUMBU.h"
//-------------------------------------------------------------------------------------
unsigned int Part::instances = 0;
// Bones a robot mesh may use: the size of boneMatrices in media/tumbu/robots/RobotSkinning.h (keep both equal).
static const size_t ROBOT_MAX_BONES = 32;
//-------------------------------------------------------------------------------------
Part::Part(int _partType, Ogre::String _setName, Ogre::SceneNode* _parentNode, Ogre::SceneManager* _sceneManager){
	partType = _partType;
	setName = _setName;
	node = _parentNode;
	sceneManager = _sceneManager;

	id = Part::instances++;
	active = false;	
	entity = NULL;
	auraEntity = NULL;

	ConfigNode* cfg;
	cfg = ConfigScriptLoader::getSingleton().getConfigScript( setName, "set" );
	displayName = cfg->findChild("name")->getValue(0);

	switch( partType ){
		case HEAD:
			partName = "head";
		break;
		case BODY:
			partName = "body";
		break;
		case RIGHT_ARM:
			partName = "rightArm";
		break;
		case LEFT_ARM:
			partName = "leftArm";
		break;
		case LEGS:
			partName = "legs";
		break;
	}

	meshName = Ogre::String(partName).append("_").append( setName.substr(setName.length() - 3)).append( ".mesh" );
}
//-------------------------------------------------------------------------------------
Part::Part(int _partType, Ogre::String _setName){
	partType = _partType;
	setName = _setName;
	node = NULL;
	sceneManager = NULL;

	id = Part::instances++;
	active = false;	
	entity = NULL;
	auraEntity = NULL;

	ConfigNode* cfg;
	cfg = ConfigScriptLoader::getSingleton().getConfigScript( setName, "set" );
	displayName = cfg->findChild("name")->getValue(0);

	switch( partType ){
		case HEAD:
			partName = "head";
		break;
		case BODY:
			partName = "body";
		break;
		case RIGHT_ARM:
			partName = "rightArm";
		break;
		case LEFT_ARM:
			partName = "leftArm";
		break;
		case LEGS:
			partName = "legs";
		break;
	}

	meshName = Ogre::String(partName).append("_").append( setName.substr(setName.length() - 3)).append( ".mesh" );
}
//-------------------------------------------------------------------------------------
Part::~Part(void){
	if(active){
		unbuild();
	}
}
//-------------------------------------------------------------------------------------
void Part::setNodeAndSceneManager( Ogre::SceneNode* _parentNode, Ogre::SceneManager* _sceneManager ){
	node = _parentNode;
	sceneManager = _sceneManager;
}
//-------------------------------------------------------------------------------------
void Part::unbuild(){
	active = false;
	// The aura shares the part's skeleton, so it goes first.
	if( auraEntity != NULL ){
		auraEntity->stopSharingSkeletonInstance();
		sceneManager->destroyEntity( auraEntity );
		auraEntity = NULL;
	}
	sceneManager->destroyEntity( entity );
	entity = NULL;
	delete collisionShape;
}
//-------------------------------------------------------------------------------------
void Part::build(){
	active = true;
	ConfigNode* animationConfig;
	bool castShadows = TUMBU::getInstance()->isCastShadows();

	entity = sceneManager->createEntity( Ogre::String(meshName).append("_").append( Ogre::StringConverter::toString(id) ),	meshName );
	node->attachObject( entity );
	// The robot shaders skin on the GPU with room for ROBOT_MAX_BONES bones per mesh; a mesh that uses more would
	// read past the bone array.
	for( unsigned short i = 0; i < entity->getMesh()->getNumSubMeshes(); i++ ){
		Ogre::SubMesh* sub = entity->getMesh()->getSubMesh( i );
		size_t bones = sub->useSharedVertices ? entity->getMesh()->sharedBlendIndexToBoneIndexMap.size() : sub->blendIndexToBoneIndexMap.size();
		if( bones > ROBOT_MAX_BONES ){
			Ogre::LogManager::getSingleton().logError( "Part: " + meshName + " uses " + Ogre::StringConverter::toString( bones ) +
				" bones, the robot shaders have room for " + Ogre::StringConverter::toString( ROBOT_MAX_BONES ) + " (RobotSkinning.h, Part.cpp): it will be deformed" );
		}
	}
	
	loadParameters();
	loadPoisition();

	entity->setCastShadows( castShadows );
	entity->getSkeleton()->setBlendMode(Ogre::ANIMBLEND_CUMULATIVE);

	// Ki aura shell (Robot::updateAura): the same mesh drawn again with the aura material, sharing the part's
	// skeleton so it follows every animation. Hidden until a special charges.
	auraEntity = sceneManager->createEntity( entity->getName() + "/aura", meshName );
	auraEntity->shareSkeletonInstanceWith( entity );
	auraEntity->setMaterialName( "Tumbu/KiAura" );
	auraEntity->setCastShadows( false );
	auraEntity->setVisible( false );
	node->attachObject( auraEntity );

	Ogre::String animNames[] = ANIMATION_ARRAY;
	for( int i = 0; i < NUM_ANIMS; i++){
		animationArray[i] = entity->getAnimationState( animNames[i] );

		animationConfig = ConfigScriptLoader::getSingleton().getConfigScript( "animation", animNames[i] );

		animationArray[i]->setLoop(animationConfig->findChild("loop")->getValueB(0) );
	}

	animConverter	= new Physics::AnimatedMeshToShapeConverter( entity );
	collisionShape	= animConverter->createBox();
	collisionShape->getBulletShape()->setLocalScaling( btVector3( 0.6f, 0.5f, 0.6f) );
	delete animConverter;
}
//-------------------------------------------------------------------------------------
void Part::loadPoisition(){
	if(node != NULL){
		node->setPosition( calculePosition() );
	}
}
//-------------------------------------------------------------------------------------
void Part::loadParameters(){
	ConfigNode* cfg;
	cfg = ConfigScriptLoader::getSingleton().getConfigScript( setName, partName );

	hp			= cfg->findChild("hp")->getValueF(0);
	ap			= cfg->findChild("ap")->getValueF(0);
	attack		= cfg->findChild("attack")->getValueF(0);
	defense		= cfg->findChild("defense")->getValueF(0);
	velocity	= cfg->findChild("velocity")->getValueF(0);
}
//-------------------------------------------------------------------------------------
Ogre::Vector3 Part::calculePosition(){
	ConfigNode* animationConfig;
	position = Ogre::Vector3::ZERO;

	animationConfig = ConfigScriptLoader::getSingleton().getConfigScript( setName, partName );

	position = Ogre::Vector3(
		animationConfig->findChild("position")->getValueF(0),
		animationConfig->findChild("position")->getValueF(1),
		animationConfig->findChild("position")->getValueF(2)
	);

	/*
	switch( partType ){
		case HEAD:
		break;
		case BODY:
		break;
		case RIGHT_ARM:
		break;
		case LEFT_ARM:
		break;
		case LEGS:
		break;
	}
	*/
	return position;
}
//-------------------------------------------------------------------------------------
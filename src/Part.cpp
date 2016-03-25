#include "ConfigScript.h"
#include "Part.h"
#include "TUMBU.h"
//-------------------------------------------------------------------------------------
unsigned int Part::instances = 0;
//-------------------------------------------------------------------------------------
Part::Part(int _partType, Ogre::String _setName, Ogre::SceneNode* _parentNode, Ogre::SceneManager* _sceneManager){
	partType = _partType;
	setName = _setName;
	node = _parentNode;
	sceneManager = _sceneManager;

	id = Part::instances++;
	active = false;	

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
	sceneManager->destroyEntity( entity );
	delete collisionShape;
}
//-------------------------------------------------------------------------------------
void Part::build(){
	active = true;
	ConfigNode* animationConfig;
	bool castShadows = TUMBU::getInstance()->isCastShadows();

	entity = sceneManager->createEntity( Ogre::String(meshName).append("_").append( Ogre::StringConverter::toString(id) ),	meshName );
	node->attachObject( entity );
	
	loadParameters();
	loadPoisition();

	entity->setCastShadows( castShadows );
	entity->getSkeleton()->setBlendMode(Ogre::ANIMBLEND_CUMULATIVE);

	Ogre::String animNames[] = ANIMATION_ARRAY;
	for( int i = 0; i < NUM_ANIMS; i++){
		animationArray[i] = entity->getAnimationState( animNames[i] );

		animationConfig = ConfigScriptLoader::getSingleton().getConfigScript( "animation", animNames[i] );

		animationArray[i]->setLoop(animationConfig->findChild("loop")->getValueB(0) );
	}

	animConverter	= new OgreBulletCollisions::AnimatedMeshToShapeConverter( entity );
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
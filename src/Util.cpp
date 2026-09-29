#include "Util.h"
//-------------------------------------------------------------------------------------
Util::Util(){
}
//-------------------------------------------------------------------------------------
Util::~Util(void){
}
//-------------------------------------------------------------------------------------
Physics::RigidBody* Util::createBarrel(int barrelIndex, Ogre::SceneManager* mSceneMgr, Physics::DynamicsWorld* world, Ogre::Camera* mCamera){
	
	Physics::RigidBody *defaultBody;
	Physics::BoxCollisionShape *sceneBoxShape;

	Ogre::SceneNode* node;
	Ogre::Entity *entity;
	Ogre::String nome;
	Ogre::Vector3 size;

	size = Ogre::Vector3::ZERO;
	node = mSceneMgr->getRootSceneNode()->createChildSceneNode();

	nome = "barrel_number_";
	nome.append( Ogre::StringConverter::toString( barrelIndex ) );
	
	// Criando o barril
	entity = mSceneMgr->createEntity(nome, "barril.mesh");
	node->attachObject(entity);
	node->setScale(1, 1, 1);

	//Pegando os boundings do objeto
	size = entity->getBoundingBox().getSize(); 
	size /= 1.9f; // diminuindo o tamanho
		
	sceneBoxShape = new Physics::BoxCollisionShape(size);
	
	defaultBody = new Physics::RigidBody(
		"defaultBoxRigid" + Ogre::StringConverter::toString(barrelIndex), 
		world
	);
	
	defaultBody->setShape(   node,	   sceneBoxShape,
					   0.1f,         // dynamic body restitution
					   0.6f,         // dynamic body friction
					   80,          // dynamic bodymass
					   Ogre::Vector3(mCamera->getDerivedPosition().x, 2, mCamera->getDerivedPosition().z),      // starting position of the box
					   Ogre::Quaternion(0,0,0,1));// orientation of the box     

	defaultBody->setLinearVelocity(mCamera->getDerivedDirection().normalisedCopy() * 7.0f ); // velocidade

	barrelIndex++;

	return defaultBody;
}
 
void Util::updateShapeFromEntity ( Robot* charUp, Part* part ){
	Physics::AnimatedMeshToShapeConverter* animConverter = new Physics::AnimatedMeshToShapeConverter(part->entity);

	Physics::CollisionShape* shape = animConverter->createConvex();
	delete animConverter;
	
	charUp->charShape = new Physics::CompoundCollisionShape();

	switch(part->partType){
	case HEAD:
		charUp->charShape->addChildShape(shape,								Ogre::Vector3( part->position ));
		charUp->charShape->addChildShape(charUp->body->collisionShape,		Ogre::Vector3( charUp->body->position ));
		charUp->charShape->addChildShape(charUp->rightArm->collisionShape,	Ogre::Vector3( charUp->rightArm->position ));
		charUp->charShape->addChildShape(charUp->leftArm->collisionShape,	Ogre::Vector3( charUp->leftArm->position ));
		charUp->charShape->addChildShape(charUp->legs->collisionShape,		Ogre::Vector3( charUp->legs->position ));
		break;
	case BODY:
		charUp->charShape->addChildShape(charUp->head->collisionShape,		Ogre::Vector3( charUp->head->position ));
		charUp->charShape->addChildShape(shape,								Ogre::Vector3( part->position ));
		charUp->charShape->addChildShape(charUp->rightArm->collisionShape,	Ogre::Vector3( charUp->rightArm->position ));
		charUp->charShape->addChildShape(charUp->leftArm->collisionShape,	Ogre::Vector3( charUp->leftArm->position ));
		charUp->charShape->addChildShape(charUp->legs->collisionShape,		Ogre::Vector3( charUp->legs->position ));
		break;
	case RIGHT_ARM:
		charUp->charShape->addChildShape(charUp->head->collisionShape,		Ogre::Vector3( charUp->head->position ));
		charUp->charShape->addChildShape(charUp->body->collisionShape,		Ogre::Vector3( charUp->body->position ));
		charUp->charShape->addChildShape(shape,								Ogre::Vector3( part->position ));
		charUp->charShape->addChildShape(charUp->leftArm->collisionShape,	Ogre::Vector3( charUp->leftArm->position ));
		charUp->charShape->addChildShape(charUp->legs->collisionShape,		Ogre::Vector3( charUp->legs->position ));
		break;
	case LEFT_ARM:
		charUp->charShape->addChildShape(charUp->head->collisionShape,		Ogre::Vector3( charUp->head->position ));
		charUp->charShape->addChildShape(charUp->body->collisionShape,		Ogre::Vector3( charUp->body->position ));
		charUp->charShape->addChildShape(charUp->rightArm->collisionShape,	Ogre::Vector3( charUp->rightArm->position ));
		charUp->charShape->addChildShape(shape,								Ogre::Vector3( part->position ));
		charUp->charShape->addChildShape(charUp->legs->collisionShape,		Ogre::Vector3( charUp->legs->position ));
		break;
	case LEGS:
		charUp->charShape->addChildShape(charUp->head->collisionShape,		Ogre::Vector3( charUp->head->position ));
		charUp->charShape->addChildShape(charUp->body->collisionShape,		Ogre::Vector3( charUp->body->position ));
		charUp->charShape->addChildShape(charUp->rightArm->collisionShape,	Ogre::Vector3( charUp->rightArm->position ));
		charUp->charShape->addChildShape(charUp->leftArm->collisionShape,	Ogre::Vector3( charUp->leftArm->position ));
		charUp->charShape->addChildShape(shape,								Ogre::Vector3( part->position ));
		break;
	}
}

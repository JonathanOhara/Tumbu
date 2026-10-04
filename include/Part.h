#ifndef __Part_h_
#define __Part_h_

#include <Ogre.h>
#include <math.h>

#include "Physics.h"


#define CHAR_PARTS 5
#define NUM_ANIMS 14

#define HEAD 0
#define BODY 1
#define RIGHT_ARM 2
#define LEFT_ARM 3
#define LEGS 4

#define ANIMATION_ARRAY {"walk", "right_punch", "guard", "no_pose", "pre_special_jyn", "pos_special_jyn", "left_punch", "right_kick", "run", "dash_back", "dash_left", "dash_right", "left_up_deflect", "right_up_deflect"}

class Part{
public:
	Part(int _partType, Ogre::String _setName, Ogre::SceneNode* _parentNode, Ogre::SceneManager* _sceneManager);
	Part(int _partType, Ogre::String _setName);
	virtual ~Part(void);
	
	void setNodeAndSceneManager( Ogre::SceneNode* _parentNode, Ogre::SceneManager* _sceneManager );

	void unbuild();
	void build();

	void loadParameters();
	void loadPoisition();

	int id,
		partType;

	bool active;

	Ogre::SceneNode
		*node;

	Ogre::Entity
		*entity,
		*auraEntity;	// ki aura shell: the same mesh with Tumbu/KiAura, sharing the skeleton (Robot::updateAura)
	
	Ogre::String 
		partName,
		meshName,
		setName,
		displayName;

	Ogre::AnimationState
		*animationArray[NUM_ANIMS];

	Physics::CollisionShape
		*collisionShape;

	Physics::AnimatedMeshToShapeConverter
		*animConverter;	

	double 
		hp,
		ap,
		attack,
		defense,
		velocity;

	Ogre::Vector3 position;

	static unsigned int instances;
protected:

private:
	Ogre::Vector3 calculePosition();

	Ogre::SceneManager* sceneManager;
};

#endif // #ifndef __Part_h_
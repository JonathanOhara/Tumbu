#ifndef __Robot_h_
#define __Robot_h_

#include "GameObject.h"
#include "SpecialManager.h"
#include "Part.h"
#include "SpecialManager.h"
#include "SoundManager.h"
#include "Skill.h"
#include "CollisionDetectionListener.h"

#define ANIM_WALK 0
#define ANIM_RIGHT_PUNCH 1
#define ANIM_GUARD 2
#define ANIM_NO_POSE 3
#define ANIM_PRE_SPECIAL_JYN 4
#define ANIM_POS_SPECIAL_JYN 5
#define ANIM_LEFT_PUNCH 6
#define ANIM_RIGHT_KICK 7
#define ANIM_RUN 8
#define ANIM_DASH_BACK 9
#define ANIM_DASH_LEFT 10
#define ANIM_DASH_RIGHT 11
#define ANIM_LEFT_UP_DEFLECT 12
#define ANIM_RIGHT_UP_DEFLECT 13

class Robot: public Ogre::FrameListener{
public:
	Robot(void);
	virtual ~Robot(void);

	virtual bool frameRenderingQueued(const Ogre::FrameEvent &evt) = 0;

	enum MoveKey{ UNDEFINED, NONE, RUNNING, UP, LEFT, DOWN, UP_LEFT, UP_RIGHT, DOWN_LEFT, DOWN_RIGHT, RIGHT, GUARD, KICK, PUNCH, JYN };

	virtual void movePressed(MoveKey _moveKey) = 0;
	virtual void moveReleased(MoveKey _moveKey) = 0;

	virtual void updateMovement(const Ogre::Real time) = 0;
	virtual void updateAttack(const Ogre::Real time) = 0;
	virtual void updateDefense(const Ogre::Real time) = 0;
	virtual void updateRegeneration(const Ogre::Real time) = 0;
	
	virtual void unbuildParts() = 0;

	virtual void buildAll() = 0;
	virtual void buildNodes() = 0;
	virtual void buildParts() = 0;
	virtual void buildCharacter() = 0;

	virtual void resetAllAnimations() = 0;
	virtual void stopAllSkills() = 0;

	virtual void addPart( int _part, Ogre::String meshName ) = 0;
	virtual void addPart( Part *_part ) = 0;
	virtual Part* removePart( int _part ) = 0;

	virtual void setNode(Ogre::SceneNode* node) = 0;
	virtual void setActiveCameraNode(Ogre::SceneNode* cameraNode) = 0;

	virtual float sofrerDano(float dano) = 0;
	virtual float criarDano(float dano) = 0;

	virtual void addHp( float _hp ) = 0;
	virtual void addAp( float _ap ) = 0;

	virtual void addHpInPercent( float hpPercent ) = 0;
	virtual void addApInPercent( float apPercent ) = 0;

	virtual void updateHpPercent() = 0;
	virtual void updateApPercent() = 0;

	virtual float getMaxHp() = 0;
	virtual float getMaxAp() = 0;
	virtual float getMaxAttack() = 0;
	virtual float getMaxDefense() = 0;
	virtual float getMaxVelocity() = 0;

	virtual void emmitSound( Ogre::String nodeName, Ogre::String musicName, Ogre::String musicFile, bool loop, bool stream, Ogre::Real delay ) = 0;

	virtual void addEnemy( Robot* enemy) = 0;

	Ogre::String robotName;

	float 
		RUN_SPEED,
		TURN_SPEED,
		CHAR_RESTITUTION,
		CHAR_FRICTION,
		CHAR_MASS,
		HP_REGENERATION_PER_SECOND,
		AP_REGENERATION_PER_SECOND;

	//Defined in moves.object
	int 
		KICK_DAMAGE,
		PUNCH_DAMAGE,
		JYN_DAMAGE;

	Part
		*head,
		*body,
		*rightArm,
		*leftArm,
		*legs;

	std::vector<Part*> 
		headList,
		bodyList,
		rightArmList,
		leftArmList,
		legsList;
		
	Ogre::SceneNode
		*robotNode,
		*robotPhysicsNode,
		*headNode,
		*bodyNode,
		*rightArmNode,
		*leftArmNode,
		*legsNode;

	OgreBulletDynamics::RigidBody 
		*charRigidBody;

	Ogre::ParticleSystem* particleSystem;

	OgreBulletCollisions::CompoundCollisionShape 
		*charShape;

	bool 
		charBuilded,
		partsBuilded;

	Skill *punch;
	Skill *kick;
	Skill *jyn;
	list<Skill*> skillList;

	/** Atributos para animação*/	
	bool 
		isAttacking,
		isGuard,
		isRunning;

	list<Robot*> enemyList;

	Ogre::Vector3 mKeyDirection;

	SpecialManager* specialManager;

	static unsigned int instances;

	Ogre::SceneNode* activeCameraNode;

	/** Atributos das atuais do jogador */
	float 
		hp,
		ap,
		hpPercent,
		apPercent;
protected:
	OgreBulletDynamics::DynamicsWorld* physicWorld;
	SoundManager* soundManager;
	Sound* soundWalk;

	Ogre::Real statsRegenartionCount;

	Ogre::Vector3 mGoalDirection;

	/** MOVEMENT */
	Ogre::Quaternion toGoal;

	Ogre::Real 
		yawToGoal,
		yawAtSpeed;

	btTransform localTrans;
	btVector3 translation;

	/** Atributos estáticos do jogador */
	float 
		maxHp,
		maxAp,
		maxAttack,
		maxDefense,
		maxVelocity;

private:
};

#endif // #ifndef __Robot_h_
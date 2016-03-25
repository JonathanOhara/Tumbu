#ifndef __CharacterEnemy_h_
#define __CharacterEnemy_h_

#include "Robot.h"

class CharacterEnemy : public Robot, public CollisionDetectionListener {
public:
	CharacterEnemy(OgreBulletDynamics::DynamicsWorld* _physicWorld, Ogre::String _robotName, SoundManager *_soundManager);
	virtual ~CharacterEnemy(void);

	bool frameRenderingQueued(const Ogre::FrameEvent &evt);
	void movePressed(MoveKey _moveKey);
	void moveReleased(MoveKey _moveKey);	

	void updateMovement(const Ogre::Real time);
	void updateAttack(const Ogre::Real time);
	void updateDefense(const Ogre::Real time);
	void updateRegeneration(const Ogre::Real time);
	
	OgreBulletDynamics::RigidBody *getOgreBulletRigidBody( const std::string& instanceName );
	void collision( CollisionDetectionListener *other );
	
	void unbuildParts();
	void buildAll();
	void buildNodes();
	void buildParts();
	void buildCharacter();

	void resetAllAnimations();
	void stopAllSkills();

	void addPart( int _part, Ogre::String meshName );
	void addPart( Part *_part );
	Part* removePart( int _part );

	void setNode(Ogre::SceneNode* node);
	void setActiveCameraNode(Ogre::SceneNode* cameraNode);

	float sofrerDano(float dano);
	float criarDano(float dano);

	void addHp( float _hp );
	void addAp( float _ap );

	void addHpInPercent( float hpPercent );
	void addApInPercent( float apPercent );

	void updateHpPercent();
	void updateApPercent();

	float getMaxHp();
	float getMaxAp();
	float getMaxAttack();
	float getMaxDefense();
	float getMaxVelocity();

	void emmitSound( Ogre::String nodeName, Ogre::String musicName, Ogre::String musicFile, bool loop, bool stream, Ogre::Real delay );

	void addEnemy( Robot* enemy);
protected:

private:
    
};
#endif // #ifndef __CharacterEnemy_h_
#ifndef __Character_h_
#define __Character_h_

#include "Robot.h"

class Character: public Robot, public OIS::KeyListener, public OIS::MouseListener, public OIS::JoyStickListener, public CollisionDetectionListener {
public:
	Character(OgreBulletDynamics::DynamicsWorld* _physicWorld, Ogre::String _robotName, SoundManager *_soundManager);
	virtual ~Character(void);

	bool frameRenderingQueued(const Ogre::FrameEvent &evt);

	void movePressed(MoveKey _moveKey);
	void moveReleased(MoveKey _moveKey);

	bool keyPressed( const OIS::KeyEvent &arg );
	bool keyReleased( const OIS::KeyEvent &arg );

	bool mouseMoved(const OIS::MouseEvent &arg);
	bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
	bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);

    bool povMoved( const OIS::JoyStickEvent &e, int pov );
    bool axisMoved( const OIS::JoyStickEvent &e, int axis );
    bool sliderMoved( const OIS::JoyStickEvent &e, int sliderID );
    bool buttonPressed( const OIS::JoyStickEvent &e, int button );
    bool buttonReleased( const OIS::JoyStickEvent &e, int button );

	void releaseAllKeys();

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
	Ogre::Real aux;
};

#endif // #ifndef __Character_h_

#include "Physics.h"

namespace Physics{
//------------------------------------------------------------------------------------- shapes
CollisionShape::~CollisionShape(void){
	delete mShape;
}
//-------------------------------------------------------------------------------------
BoxCollisionShape::BoxCollisionShape( const Ogre::Vector3 &halfExtents ){
	mShape = new btBoxShape( OgreBtConverter::to( halfExtents ) );
}
//-------------------------------------------------------------------------------------
CompoundCollisionShape::CompoundCollisionShape(void){
	mShape = new btCompoundShape();
}
//-------------------------------------------------------------------------------------
void CompoundCollisionShape::addChildShape( CollisionShape *shape, const Ogre::Vector3 &position, const Ogre::Quaternion &orientation ){
	btTransform localTransform( OgreBtConverter::to( orientation ), OgreBtConverter::to( position ) );
	static_cast<btCompoundShape*>( mShape )->addChildShape( localTransform, shape->getBulletShape() );
}
//-------------------------------------------------------------------------------------
TriangleMeshCollisionShape::~TriangleMeshCollisionShape(void){
	// Ogre::Bullet::createTrimeshCollider allocates the triangle mesh; the Bullet shape does not own it.
	btStridingMeshInterface *mesh = static_cast<btBvhTriangleMeshShape*>( mShape )->getMeshInterface();
	delete mShape;
	mShape = NULL;
	delete mesh;
}
//-------------------------------------------------------------------------------------
HeightmapCollisionShape::HeightmapCollisionShape( int width, int length, const Ogre::Vector3 &scale, float *heightData,
		Ogre::Real maxHeight, Ogre::Real minHeight, bool flipQuadEdges ){
	btHeightfieldTerrainShape *heightShape = new btHeightfieldTerrainShape(
		width, length, heightData, 1, minHeight, maxHeight, 1, PHY_FLOAT, flipQuadEdges );
	heightShape->setUseDiamondSubdivision( true );
	heightShape->setLocalScaling( OgreBtConverter::to( scale ) );
	mShape = heightShape;
}
//------------------------------------------------------------------------------------- converters
TriangleMeshCollisionShape* StaticMeshToShapeConverter::createTrimesh(void){
	return new TriangleMeshCollisionShape( Ogre::Bullet::createTrimeshCollider( mEntity ) );
}
//-------------------------------------------------------------------------------------
BoxCollisionShape* AnimatedMeshToShapeConverter::createBox(void){
	return new BoxCollisionShape( mEntity->getMesh()->getBounds().getSize() );
}
//-------------------------------------------------------------------------------------
CollisionShape* AnimatedMeshToShapeConverter::createConvex(void){
	return new CollisionShape( Ogre::Bullet::createConvexHullCollider( mEntity ) );
}
//------------------------------------------------------------------------------------- world
DynamicsWorld::DynamicsWorld( Ogre::SceneManager *sceneManager, const Ogre::AxisAlignedBox &, const Ogre::Vector3 &gravity ){
	mSceneManager		= sceneManager;
	mCollisionConfig	= new btDefaultCollisionConfiguration();
	mDispatcher			= new btCollisionDispatcher( mCollisionConfig );
	mBroadphase			= new btDbvtBroadphase();
	mSolver				= new btSequentialImpulseConstraintSolver();
	mWorld				= new btDiscreteDynamicsWorld( mDispatcher, mBroadphase, mSolver, mCollisionConfig );
	mDebugDrawer		= NULL;
	mWorld->setGravity( OgreBtConverter::to( gravity ) );
}
//-------------------------------------------------------------------------------------
DynamicsWorld::~DynamicsWorld(void){
	setDebugDrawNode( NULL );
	// Bodies are owned by the game; detach whatever is still registered so Bullet does not touch them.
	for( std::map<const btCollisionObject*, RigidBody*>::iterator it = mBodies.begin(); it != mBodies.end(); ++it ){
		mWorld->removeRigidBody( it->second->getBulletRigidBody() );
	}
	mBodies.clear();
	delete mWorld;
	delete mSolver;
	delete mBroadphase;
	delete mDispatcher;
	delete mCollisionConfig;
}
//-------------------------------------------------------------------------------------
void DynamicsWorld::stepSimulation( Ogre::Real elapsedTime, int maxSubSteps, Ogre::Real fixedTimestep ){
	mWorld->stepSimulation( elapsedTime, maxSubSteps, fixedTimestep );
	if( mDebugDrawer != NULL ){
		mDebugDrawer->update();
	}
}
//-------------------------------------------------------------------------------------
RigidBody* DynamicsWorld::findObject( const btCollisionObject *object ) const{
	std::map<const btCollisionObject*, RigidBody*>::const_iterator it = mBodies.find( object );
	return it == mBodies.end() ? NULL : it->second;
}
//-------------------------------------------------------------------------------------
void DynamicsWorld::setDebugDrawNode( Ogre::SceneNode *node ){
	if( mDebugDrawer != NULL ){
		mWorld->setDebugDrawer( NULL );
		delete mDebugDrawer;
		mDebugDrawer = NULL;
	}
	if( node != NULL ){
		mDebugDrawer = new Ogre::Bullet::DebugDrawer( node, mWorld );
	}
}
//-------------------------------------------------------------------------------------
void DynamicsWorld::_addRigidBody( RigidBody *body, short group, short mask ){
	mWorld->addRigidBody( body->getBulletRigidBody(), group, mask );
	mBodies[ body->getBulletRigidBody() ] = body;
}
//-------------------------------------------------------------------------------------
void DynamicsWorld::_removeRigidBody( RigidBody *body ){
	std::map<const btCollisionObject*, RigidBody*>::iterator it = mBodies.find( body->getBulletRigidBody() );
	if( it != mBodies.end() ){
		mWorld->removeRigidBody( body->getBulletRigidBody() );
		mBodies.erase( it );
	}
}
//------------------------------------------------------------------------------------- bodies
RigidBody::RigidBody( const Ogre::String &name, DynamicsWorld *world, short collisionGroup, short collisionMask ){
	mName			= name;
	mWorld			= world;
	mCollisionGroup	= collisionGroup;
	mCollisionMask	= collisionMask;
	mNode			= NULL;
	mShape			= NULL;
	mMotionState	= NULL;
	mBody			= NULL;
}
//-------------------------------------------------------------------------------------
RigidBody::~RigidBody(void){
	if( mBody != NULL ){
		mWorld->_removeRigidBody( this );
		delete mBody;
	}
	delete mMotionState;
}
//-------------------------------------------------------------------------------------
void RigidBody::createBody( Ogre::SceneNode *node, CollisionShape *shape, float restitution, float friction, float mass,
		const Ogre::Vector3 &position, const Ogre::Quaternion &orientation ){
	mNode	= node;
	mShape	= shape;

	node->setPosition( position );
	node->setOrientation( orientation );
	mMotionState = new Ogre::Bullet::RigidBodyState( node );

	btVector3 inertia( 0, 0, 0 );
	if( mass > 0 ){
		shape->getBulletShape()->calculateLocalInertia( mass, inertia );
	}

	mBody = new btRigidBody( mass, mMotionState, shape->getBulletShape(), inertia );
	mBody->setRestitution( restitution );
	mBody->setFriction( friction );
	mWorld->_addRigidBody( this, mCollisionGroup, mCollisionMask );
}
//-------------------------------------------------------------------------------------
void RigidBody::setShape( Ogre::SceneNode *node, CollisionShape *shape, float restitution, float friction, float mass,
		const Ogre::Vector3 &position, const Ogre::Quaternion &orientation ){
	createBody( node, shape, restitution, friction, mass, position, orientation );
}
//-------------------------------------------------------------------------------------
void RigidBody::setStaticShape( Ogre::SceneNode *node, CollisionShape *shape, float restitution, float friction,
		const Ogre::Vector3 &position, const Ogre::Quaternion &orientation ){
	createBody( node, shape, restitution, friction, 0, position, orientation );
}
//-------------------------------------------------------------------------------------
void RigidBody::setPosition( const Ogre::Vector3 &position ){
	btTransform transform = mBody->getWorldTransform();
	transform.setOrigin( OgreBtConverter::to( position ) );
	mBody->setWorldTransform( transform );
	mBody->setInterpolationWorldTransform( transform );
	mMotionState->setWorldTransform( transform );
}
//-------------------------------------------------------------------------------------
void RigidBody::applyImpulse( const Ogre::Vector3 &impulse, const Ogre::Vector3 &position ){
	mBody->activate( true );
	mBody->applyImpulse( OgreBtConverter::to( impulse ), OgreBtConverter::to( position ) );
}
//-------------------------------------------------------------------------------------
void RigidBody::setLinearVelocity( const Ogre::Vector3 &velocity ){
	mBody->activate( true );
	mBody->setLinearVelocity( OgreBtConverter::to( velocity ) );
}
//-------------------------------------------------------------------------------------
Ogre::Vector3 RigidBody::getWorldPosition(void) const{
	return BtOgreConverter::to( mBody->getWorldTransform().getOrigin() );
}
//-------------------------------------------------------------------------------------
Ogre::Quaternion RigidBody::getWorldOrientation(void) const{
	return BtOgreConverter::to( mBody->getWorldTransform().getRotation() );
}
//-------------------------------------------------------------------------------------
} // namespace Physics

#ifndef __Physics_h_
#define __Physics_h_

// Thin physics layer on top of Bullet 3 and Ogre 14's Bullet component.
//
// It replaces the dead OgreBullet addon the game was written against (2011) and keeps the same shape of API
// (RigidBody::setShape, CompoundCollisionShape::addChildShape, DynamicsWorld::findObject, ...) so the
// gameplay code only changed namespaces. Ownership rules match OgreBullet:
//   - a CollisionShape owns its Bullet shape (a compound shape does not own its children),
//   - a RigidBody owns its Bullet body and motion state, but not its CollisionShape,
//   - a DynamicsWorld does not own the RigidBodies added to it.

#include <Ogre.h>
#include <OgreBullet.h>

#include <map>

namespace Physics{

//------------------------------------------------------------------------------ conversions
struct OgreBtConverter{
	static btVector3 to( const Ogre::Vector3 &v ){ return Ogre::Bullet::convert( v ); }
	static btQuaternion to( const Ogre::Quaternion &q ){ return Ogre::Bullet::convert( q ); }
};

struct BtOgreConverter{
	static Ogre::Vector3 to( const btVector3 &v ){ return Ogre::Bullet::convert( v ); }
	static Ogre::Quaternion to( const btQuaternion &q ){ return Ogre::Bullet::convert( q ); }
};

//------------------------------------------------------------------------------ shapes
class CollisionShape{
public:
	explicit CollisionShape( btCollisionShape *shape = NULL ): mShape( shape ){}
	virtual ~CollisionShape(void);
	btCollisionShape* getBulletShape(void) const { return mShape; }
protected:
	btCollisionShape *mShape;
};

class BoxCollisionShape: public CollisionShape{
public:
	/// @param halfExtents half size of the box on each axis (like btBoxShape)
	explicit BoxCollisionShape( const Ogre::Vector3 &halfExtents );
};

class CompoundCollisionShape: public CollisionShape{
public:
	CompoundCollisionShape(void);
	/// The child shape is referenced, not owned.
	void addChildShape( CollisionShape *shape, const Ogre::Vector3 &position,
		const Ogre::Quaternion &orientation = Ogre::Quaternion::IDENTITY );
};

class TriangleMeshCollisionShape: public CollisionShape{
public:
	explicit TriangleMeshCollisionShape( btBvhTriangleMeshShape *shape ): CollisionShape( shape ){}
	virtual ~TriangleMeshCollisionShape(void);
};

class HeightmapCollisionShape: public CollisionShape{
public:
	/// @param heightData width * length floats, kept alive by the caller while the shape exists
	HeightmapCollisionShape( int width, int length, const Ogre::Vector3 &scale, float *heightData,
		Ogre::Real maxHeight, Ogre::Real minHeight, bool flipQuadEdges );
};

/// Builds collision shapes from an Entity's mesh.
class StaticMeshToShapeConverter{
public:
	explicit StaticMeshToShapeConverter( Ogre::Entity *entity ): mEntity( entity ){}
	TriangleMeshCollisionShape* createTrimesh(void);
protected:
	Ogre::Entity *mEntity;
};

class AnimatedMeshToShapeConverter: public StaticMeshToShapeConverter{
public:
	explicit AnimatedMeshToShapeConverter( Ogre::Entity *entity ): StaticMeshToShapeConverter( entity ){}
	/// Like OgreBullet: the mesh's full size is used as the box half extents (so the box is twice the mesh).
	/// Kept on purpose so hit detection behaves like the 2011 game.
	BoxCollisionShape* createBox(void);
	CollisionShape* createConvex(void);
};

//------------------------------------------------------------------------------ world and bodies
class RigidBody;

class DynamicsWorld{
public:
	/// @param bounds kept for API compatibility (the broadphase is unbounded)
	DynamicsWorld( Ogre::SceneManager *sceneManager, const Ogre::AxisAlignedBox &bounds, const Ogre::Vector3 &gravity );
	virtual ~DynamicsWorld(void);

	void stepSimulation( Ogre::Real elapsedTime, int maxSubSteps = 1, Ogre::Real fixedTimestep = 1.0f / 60.0f );

	btDiscreteDynamicsWorld* getBulletDynamicsWorld(void) const { return mWorld; }
	btCollisionWorld* getBulletCollisionWorld(void) const { return mWorld; }
	Ogre::SceneManager* getSceneManager(void) const { return mSceneManager; }

	/// The RigidBody wrapping a Bullet object, or NULL.
	RigidBody* findObject( const btCollisionObject *object ) const;

	/// Draws collision shapes as lines, attached to the given node (NULL disables it).
	void setDebugDrawNode( Ogre::SceneNode *node );

	// Used by RigidBody.
	void _addRigidBody( RigidBody *body, short group, short mask );
	void _removeRigidBody( RigidBody *body );

private:
	Ogre::SceneManager *mSceneManager;
	btDefaultCollisionConfiguration *mCollisionConfig;
	btCollisionDispatcher *mDispatcher;
	btBroadphaseInterface *mBroadphase;
	btSequentialImpulseConstraintSolver *mSolver;
	btDiscreteDynamicsWorld *mWorld;
	Ogre::Bullet::DebugDrawer *mDebugDrawer;
	std::map<const btCollisionObject*, RigidBody*> mBodies;
};

class RigidBody{
public:
	RigidBody( const Ogre::String &name, DynamicsWorld *world, short collisionGroup = 1, short collisionMask = -1 );
	virtual ~RigidBody(void);

	/// Dynamic body whose transform drives `node`. The node is moved to pos/orientation first.
	void setShape( Ogre::SceneNode *node, CollisionShape *shape, float restitution, float friction, float mass,
		const Ogre::Vector3 &position = Ogre::Vector3::ZERO, const Ogre::Quaternion &orientation = Ogre::Quaternion::IDENTITY );

	/// Static (mass 0) body at pos/orientation; the node is moved there too.
	void setStaticShape( Ogre::SceneNode *node, CollisionShape *shape, float restitution, float friction,
		const Ogre::Vector3 &position = Ogre::Vector3::ZERO, const Ogre::Quaternion &orientation = Ogre::Quaternion::IDENTITY );

	void setPosition( const Ogre::Vector3 &position );
	void setPosition( Ogre::Real x, Ogre::Real y, Ogre::Real z ){ setPosition( Ogre::Vector3( x, y, z ) ); }
	void applyImpulse( const Ogre::Vector3 &impulse, const Ogre::Vector3 &position );
	void setLinearVelocity( const Ogre::Vector3 &velocity );
	Ogre::Vector3 getWorldPosition(void) const;
	Ogre::Quaternion getWorldOrientation(void) const;
	void showDebugShape( bool ){}	// debug drawing is world-wide now (DynamicsWorld::setDebugDrawNode)

	const Ogre::String& getName(void) const { return mName; }
	void setName( const Ogre::String &name ){ mName = name; }
	btRigidBody* getBulletRigidBody(void) const { return mBody; }
	btCollisionObject* getBulletObject(void) const { return mBody; }
	Ogre::SceneNode* getSceneNode(void) const { return mNode; }
	CollisionShape* getShape(void) const { return mShape; }

private:
	void createBody( Ogre::SceneNode *node, CollisionShape *shape, float restitution, float friction, float mass,
		const Ogre::Vector3 &position, const Ogre::Quaternion &orientation );

	Ogre::String mName;
	DynamicsWorld *mWorld;
	short mCollisionGroup, mCollisionMask;
	Ogre::SceneNode *mNode;
	CollisionShape *mShape;
	Ogre::Bullet::RigidBodyState *mMotionState;
	btRigidBody *mBody;
};

} // namespace Physics

#endif // #ifndef __Physics_h_

#ifndef __Grass_h_
#define __Grass_h_

#include <Ogre.h>
#include <vector>

class btCollisionWorld;
class btCollisionObject;

/** The field's tall grass (the grass remake, 2026-10): toon blades of real geometry, generated when a match loads from
 *  the field map that is painted in Blender (media/tumbu/arena/field_map.png, B = grass height; arena_field.py). A blade
 *  grows only where the field itself is the top surface (a ray from above hits the field, not the ring, the coliseum or
 *  the walls) and not on the earth (the same rule as the field shader's earth edge). Clumps share height, lean and tone.
 *  Every vertex carries its blade's root, height and seed, so the vertex shader (grass.vert) bends each blade: wind gusts
 *  rolling across the field, and pushed aside by the robots' feet and by energy balls flying low over it. The blades are
 *  static ManualObjects in square chunks (culled separately). Tuning in media/configuration/grass.object. */
class Grass{
public:
	/// Builds the grass; `field` is the field's collision object in `world` (the others cover the ground).
	Grass( Ogre::SceneManager* sceneMgr, btCollisionWorld* world, const btCollisionObject* field );
	virtual ~Grass(void);

	/// The scene node that holds every chunk (DevTest -hide=grass).
	Ogre::SceneNode* getNode(void){ return node; }

private:
	struct Map{
		Ogre::Image image;
		bool loaded;
		Map() : loaded( false ){}
		/// Bilinear sample of one channel (0..1) at a position in the image (u, v in 0..1, wrapping when `wrap`).
		float sample( float u, float v, int channel, bool wrap ) const;
	};

	void build( btCollisionWorld* world, const btCollisionObject* field );

	Ogre::SceneManager* mSceneMgr;
	Ogre::SceneNode* node;
	std::vector<Ogre::ManualObject*> chunks;
};

#endif // #ifndef __Grass_h_

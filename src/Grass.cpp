#include "Grass.h"
#include "ConfigScript.h"

#include <btBulletDynamicsCommon.h>
#include <map>
#include <random>
//-------------------------------------------------------------------------------------
namespace{
	float configValue( ConfigNode* cfg, const Ogre::String &key, float fallback ){
		ConfigNode* child = cfg != NULL ? cfg->findChild( key ) : NULL;
		return child != NULL ? child->getValueF() : fallback;
	}

	// A value 0..1 per integer cell, the same for every blade in it (the clumps).
	float cellHash( float a ){
		float s = std::sin( a * 12.9898f ) * 43758.5453f;
		return s - std::floor( s );
	}

	// One float4 of a material's fragment program parameters (the field shader's tuning, so the grass follows its
	// earth edge), or `fallback` when it is not there.
	Ogre::Vector4 fragmentParam( const Ogre::String &material, const Ogre::String &name, const Ogre::Vector4 &fallback ){
		Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName( material );
		if( !mat ){
			return fallback;
		}
		mat->load();
		Ogre::Technique* technique = mat->getBestTechnique();
		if( technique == NULL || technique->getNumPasses() == 0 || !technique->getPass( 0 )->hasFragmentProgram() ){
			return fallback;
		}
		Ogre::GpuProgramParametersSharedPtr params = technique->getPass( 0 )->getFragmentProgramParameters();
		const Ogre::GpuConstantDefinition* def = params->_findNamedConstantDefinition( name );
		if( def == NULL || !def->isFloat() ){
			return fallback;
		}
		const float* f = params->getFloatPointer( def->physicalIndex );
		return Ogre::Vector4( f[0], f[1], f[2], f[3] );
	}

	struct Chunk{
		std::vector<float> vertices;    // position 3, normal 3, uv0 4, uv1 4
		std::vector<Ogre::uint32> indices;
		size_t count( void ) const { return vertices.size() / 14; }
	};
}
//-------------------------------------------------------------------------------------
float Grass::Map::sample( float u, float v, int channel, bool wrap ) const{
	if( !loaded ){
		return 0.5f;
	}
	const size_t w = image.getWidth(), h = image.getHeight();
	float x = u * w - 0.5f, y = v * h - 0.5f;
	float fx = x - std::floor( x ), fy = y - std::floor( y );
	long x0 = (long)std::floor( x ), y0 = (long)std::floor( y );
	float c[2][2];
	for( int j = 0; j < 2; j++ ){
		for( int i = 0; i < 2; i++ ){
			long px = x0 + i, py = y0 + j;
			if( wrap ){
				px = ( ( px % (long)w ) + (long)w ) % (long)w;
				py = ( ( py % (long)h ) + (long)h ) % (long)h;
			}else{
				px = std::max( 0L, std::min( (long)w - 1, px ) );
				py = std::max( 0L, std::min( (long)h - 1, py ) );
			}
			c[j][i] = image.getColourAt( (size_t)px, (size_t)py, 0 )[channel];
		}
	}
	return ( c[0][0] * ( 1 - fx ) + c[0][1] * fx ) * ( 1 - fy ) + ( c[1][0] * ( 1 - fx ) + c[1][1] * fx ) * fy;
}
//-------------------------------------------------------------------------------------
Grass::Grass( Ogre::SceneManager* sceneMgr, btCollisionWorld* world, const btCollisionObject* field ){
	mSceneMgr = sceneMgr;
	node = mSceneMgr->getRootSceneNode()->createChildSceneNode( "GrassNode" );
	build( world, field );
}
//-------------------------------------------------------------------------------------
Grass::~Grass(void){
	for( size_t i = 0; i < chunks.size(); i++ ){
		mSceneMgr->destroyManualObject( chunks[i] );
	}
	chunks.clear();
	mSceneMgr->destroySceneNode( node );
}
//-------------------------------------------------------------------------------------
void Grass::build( btCollisionWorld* world, const btCollisionObject* field ){
	Ogre::Timer timer;
	ConfigNode* cfg = ConfigScriptLoader::getSingleton().getConfigScript( "grass", "field" );
	if( cfg == NULL ){
		Ogre::LogManager::getSingleton().logError( "[Grass] no 'grass field' in grass.object: no tall grass" );
		return;
	}
	const float density		= configValue( cfg, "density", 40.0f );
	const float fullHeight	= configValue( cfg, "fullHeight", 0.9f );
	const float minHeight	= configValue( cfg, "minHeight", 0.06f );
	const float bladeWidth	= configValue( cfg, "width", 0.028f );
	const float widthJitter	= configValue( cfg, "widthJitter", 0.02f );
	const float widthScale	= configValue( cfg, "widthScale", 2.2f );
	const float bendMin		= configValue( cfg, "bend", 0.25f );
	const float bendJitter	= configValue( cfg, "bendJitter", 0.45f );
	const float clumpSize	= configValue( cfg, "clumpSize", 0.9f );
	const float patchSize	= configValue( cfg, "patchSize", 23.0f );
	const float earthMargin	= configValue( cfg, "earthMargin", 0.08f );
	const float earthJitter	= configValue( cfg, "earthJitter", 0.1f );
	const float chunkSize	= configValue( cfg, "chunkSize", 8.0f );
	const float extent		= configValue( cfg, "mapExtent", 196.0f );
	const int segments		= std::max( 1, (int)configValue( cfg, "segments", 3.0f ) );
	const Ogre::String material = cfg->findChild( "material" ) != NULL ? cfg->findChild( "material" )->getValue() : "Tumbu/FieldGrass";

	Map fieldMap, noise;
	try{
		fieldMap.image.load( "field_map.png", "Game" );
		fieldMap.loaded = true;
		noise.image.load( "meadow_noise.png", "Game" );
		noise.loaded = true;
	}catch( Ogre::Exception &e ){
		Ogre::LogManager::getSingleton().logError( "[Grass] " + e.getDescription() + ": no tall grass" );
		return;
	}
	// the field shader's earth edge: the same noise at the same scale, so no blade grows on the earth
	const Ogre::Vector4 layerScale = fragmentParam( "fieldMaterial", "layerScale", Ogre::Vector4( 5, 4, 6, 7.3f ) );
	const Ogre::Vector4 fieldParams = fragmentParam( "fieldMaterial", "fieldParams", Ogre::Vector4( 0.5f, 0.6f, 0.28f, 0.14f ) );

	// the part of the map where grass grows (B > 0)
	const size_t mapW = fieldMap.image.getWidth(), mapH = fieldMap.image.getHeight();
	size_t minX = mapW, minY = mapH, maxX = 0, maxY = 0;
	for( size_t y = 0; y < mapH; y++ ){
		for( size_t x = 0; x < mapW; x++ ){
			if( fieldMap.image.getColourAt( x, y, 0 ).b > 0.01f ){
				minX = std::min( minX, x );
				maxX = std::max( maxX, x );
				minY = std::min( minY, y );
				maxY = std::max( maxY, y );
			}
		}
	}
	if( minX > maxX ){
		Ogre::LogManager::getSingleton().logMessage( "[Grass] the field map has no tall grass (B = 0 everywhere)" );
		return;
	}
	// the map covers `extent` units from above: column -> x, row -> z (Ogre's v runs down the image, blender2ogre 1 - v)
	const float x0 = (float)minX / mapW * extent - extent / 2, x1 = (float)( maxX + 1 ) / mapW * extent - extent / 2;
	const float z0 = (float)minY / mapH * extent - extent / 2, z1 = (float)( maxY + 1 ) / mapH * extent - extent / 2;
	const int chunksX = std::max( 1, (int)std::ceil( ( x1 - x0 ) / chunkSize ) );

	std::mt19937 rng( (unsigned)configValue( cfg, "seed", 1234.0f ) );
	std::uniform_real_distribution<float> U( 0.0f, 1.0f );
	const long candidates = (long)( ( x1 - x0 ) * ( z1 - z0 ) * density );
	long placed = 0, onEarth = 0, covered = 0;
	std::map<int, Chunk> built;
	for( long i = 0; i < candidates; i++ ){
		const float x = x0 + U( rng ) * ( x1 - x0 ), z = z0 + U( rng ) * ( z1 - z0 );
		const float u = ( x + extent / 2 ) / extent, v = ( z + extent / 2 ) / extent;
		const float tall = fieldMap.sample( u, v, 2, false );
		if( tall < 0.02f ){
			continue;
		}
		// not on the earth or the snow (the field shader's rule, a little inside its edge)
		const float breakup = noise.sample( x / layerScale.w, z / layerScale.w, 0, true ) - 0.5f;
		const float earth = fieldMap.sample( u, v, 0, false ) + breakup * fieldParams.y;
		const float snow = fieldMap.sample( u, v, 1, false ) + breakup * 0.4f;
		if( earth > fieldParams.x - earthMargin + U( rng ) * earthJitter || snow > 0.5f - earthMargin ){
			onEarth++;
			continue;
		}
		// only where the field is the top surface (not under the ring, the walls or the seats)
		btVector3 from( x, 80.0f, z ), to( x, -40.0f, z );
		btCollisionWorld::ClosestRayResultCallback hit( from, to );
		world->rayTest( from, to, hit );
		if( !hit.hasHit() || hit.m_collisionObject != field ){
			covered++;
			continue;
		}
		Ogre::Vector3 normal( hit.m_hitNormalWorld.x(), hit.m_hitNormalWorld.y(), hit.m_hitNormalWorld.z() );
		normal.normalise();
		if( normal.y < 0.0f ){
			normal = -normal;
		}
		const Ogre::Vector3 root( x, hit.m_hitPointWorld.y() - 0.02f, z );

		// clumps: a cell shares height, lean and tone; a broad patch field varies the height across the field
		const float cx = std::floor( x / clumpSize ), cz = std::floor( z / clumpSize );
		const float clump = cellHash( cx * 17.0f + cz * 131.0f );
		const float patch = noise.sample( x / patchSize + 0.4f, z / patchSize + 0.1f, 1, true );
		const float height = tall * fullHeight * ( 0.65f + 0.7f * clump ) * ( 0.75f + 0.5f * patch ) * ( 0.8f + 0.4f * U( rng ) );
		if( height < minHeight ){
			continue;
		}
		const float seed = U( rng );
		const float lean = cellHash( clump * 91.0f ) * Ogre::Math::TWO_PI;
		const Ogre::Vector3 leanDir( std::cos( lean ), 0, std::sin( lean ) );
		const float facing = U( rng ) * Ogre::Math::TWO_PI;
		const Ogre::Vector3 side( std::cos( facing ), 0, std::sin( facing ) );
		const float tone = clump * 0.7f + U( rng ) * 0.3f;
		const float width = ( bladeWidth + widthJitter * U( rng ) ) * ( 0.7f + 0.6f * std::sqrt( height ) ) * widthScale;
		const float bend = bendMin + bendJitter * U( rng );
		const Ogre::Vector3 bladeDir = ( leanDir * 0.7f + Ogre::Vector3( std::cos( facing + 1.57f ), 0, std::sin( facing + 1.57f ) ) * 0.3f ).normalisedCopy();
		// the normal leans on the ground's, so the field shades like one surface instead of thousands of strips
		const Ogre::Vector3 faceNormal = side.crossProduct( Ogre::Vector3::UNIT_Y ).normalisedCopy();
		const Ogre::Vector3 n = ( normal * 0.6f + faceNormal * 0.4f ).normalisedCopy();

		const int key = (int)std::floor( ( z - z0 ) / chunkSize ) * chunksX + (int)std::floor( ( x - x0 ) / chunkSize );
		Chunk &chunk = built[key];
		const Ogre::uint32 base = (Ogre::uint32)chunk.count();
		// a tapered, curved blade: `segments` quads narrowing to a point; t = 0 at the root, 1 at the tip
		for( int k = 0; k <= segments; k++ ){
			const float t = (float)k / segments;
			const Ogre::Vector3 p = root + Ogre::Vector3::UNIT_Y * ( height * t ) + bladeDir * ( bend * height * t * t );
			const float w = width * ( 1.0f - t * t * 0.85f );
			for( int e = 0; e < ( k == segments ? 1 : 2 ); e++ ){
				const float across = ( k == segments ) ? 0.0f : ( e == 0 ? -1.0f : 1.0f );
				const Ogre::Vector3 q = p + side * ( w * 0.5f * across );
				const float data[14] = { q.x, q.y, q.z, n.x, n.y, n.z, t, across, tone, seed, root.x, root.y, root.z, height };
				chunk.vertices.insert( chunk.vertices.end(), data, data + 14 );
			}
		}
		for( int k = 0; k < segments - 1; k++ ){
			const Ogre::uint32 a = base + k * 2;
			const Ogre::uint32 quad[6] = { a, a + 1, a + 3, a, a + 3, a + 2 };
			chunk.indices.insert( chunk.indices.end(), quad, quad + 6 );
		}
		const Ogre::uint32 a = base + ( segments - 1 ) * 2;
		const Ogre::uint32 tip[3] = { a, a + 1, a + 2 };
		chunk.indices.insert( chunk.indices.end(), tip, tip + 3 );
		placed++;
	}

	size_t vertices = 0;
	for( std::map<int, Chunk>::iterator c = built.begin(); c != built.end(); c++ ){
		const Chunk &chunk = c->second;
		Ogre::ManualObject* mo = mSceneMgr->createManualObject( "Grass" + Ogre::StringConverter::toString( c->first ) );
		mo->setCastShadows( false );
		mo->estimateVertexCount( chunk.count() );
		mo->estimateIndexCount( chunk.indices.size() );
		mo->begin( material, Ogre::RenderOperation::OT_TRIANGLE_LIST, "Game" );
		for( size_t v = 0; v < chunk.count(); v++ ){
			const float* d = &chunk.vertices[v * 14];
			mo->position( d[0], d[1], d[2] );
			mo->normal( d[3], d[4], d[5] );
			mo->textureCoord( d[6], d[7], d[8], d[9] );
			mo->textureCoord( d[10], d[11], d[12], d[13] );
		}
		for( size_t k = 0; k < chunk.indices.size(); k++ ){
			mo->index( chunk.indices[k] );
		}
		mo->end();
		node->attachObject( mo );
		chunks.push_back( mo );
		vertices += chunk.count();
	}
	Ogre::LogManager::getSingleton().logMessage( "[Grass] " + Ogre::StringConverter::toString( (int)placed ) + " blades, "
		+ Ogre::StringConverter::toString( (int)vertices ) + " vertices in " + Ogre::StringConverter::toString( (int)chunks.size() )
		+ " chunks (" + Ogre::StringConverter::toString( (int)candidates ) + " tried: " + Ogre::StringConverter::toString( (int)onEarth )
		+ " on earth or snow, " + Ogre::StringConverter::toString( (int)covered ) + " covered) in "
		+ Ogre::StringConverter::toString( (int)timer.getMilliseconds() ) + " ms" );
}

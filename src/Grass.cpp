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

	// One float4 of a material's vertex program parameters, or `fallback`.
	Ogre::Vector4 vertexParam( const Ogre::String &material, const Ogre::String &name, const Ogre::Vector4 &fallback ){
		Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName( material );
		if( !mat ){
			return fallback;
		}
		mat->load();
		Ogre::Technique* technique = mat->getBestTechnique();
		if( technique == NULL || technique->getNumPasses() == 0 || !technique->getPass( 0 )->hasVertexProgram() ){
			return fallback;
		}
		Ogre::GpuProgramParametersSharedPtr params = technique->getPass( 0 )->getVertexProgramParameters();
		const Ogre::GpuConstantDefinition* def = params->_findNamedConstantDefinition( name );
		if( def == NULL || !def->isFloat() ){
			return fallback;
		}
		const float* f = params->getFloatPointer( def->physicalIndex );
		return Ogre::Vector4( f[0], f[1], f[2], f[3] );
	}

	// Sets one float4 of a material's vertex program parameters (the flowers take the grass's wind and push).
	void setVertexParam( const Ogre::String &material, const Ogre::String &name, const Ogre::Vector4 &value ){
		Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName( material );
		if( !mat ){
			return;
		}
		mat->load();
		Ogre::Technique* technique = mat->getBestTechnique();
		if( technique == NULL || technique->getNumPasses() == 0 || !technique->getPass( 0 )->hasVertexProgram() ){
			return;
		}
		technique->getPass( 0 )->getVertexProgramParameters()->setNamedConstant( name, value );
	}

	struct Chunk{
		std::vector<float> vertices;    // position 3, normal 3, uv0 4, uv1 4
		std::vector<Ogre::uint32> indices;
		// per plant set (the flowers, the reeds): position 3, normal 3, uv0 4, uv1 4, colour 4
		std::vector<float> plantVertices[2];
		std::vector<Ogre::uint32> plantIndices[2];
		size_t count( void ) const { return vertices.size() / 14; }
		size_t plantCount( int set ) const { return plantVertices[set].size() / 18; }
	};

	// The plant sets: their block in grass.object ("<type> field"), the prefix of their chunks' names, their node.
	const int PLANT_SETS = 2;
	const char* PLANT_TYPES[PLANT_SETS] = { "flowers", "reeds" };
	const char* PLANT_OBJECTS[PLANT_SETS] = { "Flowers", "Reeds" };

	// A flower model (media/tumbu/arena/flower_<kind>.mesh): its root at the origin, growing up y.
	struct FlowerModel{
		Ogre::String name;
		std::vector<Ogre::Vector3> positions, normals;
		std::vector<Ogre::ColourValue> colours;
		std::vector<Ogre::uint32> indices;
		float height;
		FlowerModel() : height( 0 ){}
	};

	// blender2ogre writes Blender's linear colours; the game's colours are sRGB values (what Blender shows)
	float linearToSrgb( float c ){
		return c <= 0.0031308f ? c * 12.92f : 1.055f * std::pow( c, 1.0f / 2.4f ) - 0.055f;
	}

	// Reads a flower mesh's triangles, normals and vertex colours, then removes the mesh (it is never drawn itself).
	bool readFlower( const Ogre::String &name, FlowerModel &model ){
		Ogre::MeshPtr mesh;
		try{
			mesh = Ogre::MeshManager::getSingleton().load( name + ".mesh", "Game", Ogre::HBU_GPU_ONLY, Ogre::HBU_GPU_ONLY, true, true );
		}catch( Ogre::Exception &e ){
			Ogre::LogManager::getSingleton().logError( "[Grass] flower " + name + ": " + e.getDescription() );
			return false;
		}
		model.name = name;
		std::map<Ogre::VertexData*, Ogre::uint32> bases;
		for( unsigned short s = 0; s < mesh->getNumSubMeshes(); s++ ){
			Ogre::SubMesh* sub = mesh->getSubMesh( s );
			Ogre::VertexData* vd = sub->useSharedVertices ? mesh->sharedVertexData : sub->vertexData;
			if( vd == NULL || sub->indexData == NULL || !sub->indexData->indexBuffer ){
				continue;
			}
			const Ogre::VertexElement* pos = vd->vertexDeclaration->findElementBySemantic( Ogre::VES_POSITION );
			if( pos == NULL ){
				continue;
			}
			if( bases.find( vd ) == bases.end() ){
				bases[vd] = (Ogre::uint32)model.positions.size();
				const Ogre::VertexElement* elements[3] = { pos, vd->vertexDeclaration->findElementBySemantic( Ogre::VES_NORMAL ),
					vd->vertexDeclaration->findElementBySemantic( Ogre::VES_DIFFUSE ) };
				std::vector<Ogre::Vector3> values[2];
				std::vector<Ogre::ColourValue> colours( vd->vertexCount, Ogre::ColourValue::White );
				values[0].assign( vd->vertexCount, Ogre::Vector3::ZERO );
				values[1].assign( vd->vertexCount, Ogre::Vector3::UNIT_Y );
				for( int e = 0; e < 3; e++ ){
					const Ogre::VertexElement* el = elements[e];
					if( el == NULL ){
						continue;
					}
					Ogre::HardwareVertexBufferSharedPtr buffer = vd->vertexBufferBinding->getBuffer( el->getSource() );
					Ogre::HardwareBufferLockGuard lock( buffer, Ogre::HardwareBuffer::HBL_READ_ONLY );
					for( size_t v = 0; v < vd->vertexCount; v++ ){
						unsigned char* vertex = (unsigned char*)lock.pData + ( vd->vertexStart + v ) * buffer->getVertexSize();
						if( e < 2 ){
							float* f;
							el->baseVertexPointerToElement( vertex, &f );
							values[e][v] = Ogre::Vector3( f[0], f[1], f[2] );
						}else if( el->getType() == Ogre::VET_UBYTE4_NORM ){
							unsigned char* c;
							el->baseVertexPointerToElement( vertex, &c );
							colours[v] = Ogre::ColourValue( linearToSrgb( c[0] / 255.0f ), linearToSrgb( c[1] / 255.0f ),
								linearToSrgb( c[2] / 255.0f ), c[3] / 255.0f );
						}
					}
				}
				for( size_t v = 0; v < vd->vertexCount; v++ ){
					model.positions.push_back( values[0][v] );
					model.normals.push_back( values[1][v].normalisedCopy() );
					model.colours.push_back( colours[v] );
					model.height = std::max( model.height, values[0][v].y );
				}
			}
			const Ogre::uint32 base = bases[vd];
			Ogre::HardwareIndexBufferSharedPtr ib = sub->indexData->indexBuffer;
			Ogre::HardwareBufferLockGuard lock( ib, Ogre::HardwareBuffer::HBL_READ_ONLY );
			for( size_t i = 0; i < sub->indexData->indexCount; i++ ){
				const size_t k = sub->indexData->indexStart + i;
				const Ogre::uint32 index = ib->getType() == Ogre::HardwareIndexBuffer::IT_32BIT
					? ( (Ogre::uint32*)lock.pData )[k] : ( (Ogre::uint16*)lock.pData )[k];
				model.indices.push_back( base + index );
			}
		}
		Ogre::MeshManager::getSingleton().remove( mesh );
		return model.height > 0.0f && !model.indices.empty();
	}
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
	flowerNode = node->createChildSceneNode( "FlowerNode" );
	reedNode = node->createChildSceneNode( "ReedNode" );
	build( world, field );
}
//-------------------------------------------------------------------------------------
Grass::~Grass(void){
	for( size_t i = 0; i < chunks.size(); i++ ){
		mSceneMgr->destroyManualObject( chunks[i] );
	}
	chunks.clear();
	mSceneMgr->destroySceneNode( flowerNode );
	mSceneMgr->destroySceneNode( reedNode );
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

	std::uniform_real_distribution<float> U( 0.0f, 1.0f );
	// Where a plant can grow: 0 = here (root and the ground's normal set, `tall` = the field map's B), 1 = no tall grass
	// here, 2 = on the earth or the snow, 3 = covered (by the ring, the walls or the seats).
	auto ground = [&]( float x, float z, std::mt19937 &r, Ogre::Vector3 &root, Ogre::Vector3 &normal, float &tall ) -> int{
		const float u = ( x + extent / 2 ) / extent, v = ( z + extent / 2 ) / extent;
		tall = fieldMap.sample( u, v, 2, false );
		if( tall < 0.02f ){
			return 1;
		}
		// not on the earth or the snow (the field shader's rule, a little inside its edge)
		const float breakup = noise.sample( x / layerScale.w, z / layerScale.w, 0, true ) - 0.5f;
		const float earth = fieldMap.sample( u, v, 0, false ) + breakup * fieldParams.y;
		const float snow = fieldMap.sample( u, v, 1, false ) + breakup * 0.4f;
		if( earth > fieldParams.x - earthMargin + U( r ) * earthJitter || snow > 0.5f - earthMargin ){
			return 2;
		}
		// only where the field is the top surface (not under the ring, the walls or the seats)
		btVector3 from( x, 80.0f, z ), to( x, -40.0f, z );
		btCollisionWorld::ClosestRayResultCallback hit( from, to );
		world->rayTest( from, to, hit );
		if( !hit.hasHit() || hit.m_collisionObject != field ){
			return 3;
		}
		normal = Ogre::Vector3( hit.m_hitNormalWorld.x(), hit.m_hitNormalWorld.y(), hit.m_hitNormalWorld.z() );
		normal.normalise();
		if( normal.y < 0.0f ){
			normal = -normal;
		}
		root = Ogre::Vector3( x, hit.m_hitPointWorld.y() - 0.02f, z );
		return 0;
	};
	// the chunk a position falls in
	auto chunkKey = [&]( float x, float z ) -> int{
		return (int)std::floor( ( z - z0 ) / chunkSize ) * chunksX + (int)std::floor( ( x - x0 ) / chunkSize );
	};

	std::mt19937 rng( (unsigned)configValue( cfg, "seed", 1234.0f ) );
	const long candidates = (long)( ( x1 - x0 ) * ( z1 - z0 ) * density );
	long placed = 0, onEarth = 0, covered = 0;
	std::map<int, Chunk> built;
	for( long i = 0; i < candidates; i++ ){
		const float x = x0 + U( rng ) * ( x1 - x0 ), z = z0 + U( rng ) * ( z1 - z0 );
		Ogre::Vector3 root, normal;
		float tall;
		const int where = ground( x, z, rng, root, normal, tall );
		if( where == 1 ){
			continue;
		}else if( where == 2 ){
			onEarth++;
			continue;
		}else if( where == 3 ){
			covered++;
			continue;
		}

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

		Chunk &chunk = built[chunkKey( x, z )];
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

	// The plants (PLANT_SETS: the flowers, the reeds): clumps of one kind each, spread around the ring between ringFrom
	// and ringTo from the centre, on ground where the tall grass grows. Each plant is its model turned, scaled and leaning
	// a little, already in world space, carrying what flower.vert needs to bend it like a blade.
	int plants[PLANT_SETS] = { 0, 0 }, plantClumps[PLANT_SETS] = { 0, 0 }, plantKinds[PLANT_SETS] = { 0, 0 };
	Ogre::String plantMaterial[PLANT_SETS];
	for( int set = 0; set < PLANT_SETS; set++ ){
		ConfigNode* pcfg = ConfigScriptLoader::getSingleton().getConfigScript( PLANT_TYPES[set], "field" );
		std::vector<FlowerModel> models;
		if( pcfg != NULL && pcfg->findChild( "meshes" ) != NULL ){
			const std::vector<Ogre::String> &names = pcfg->findChild( "meshes" )->getValues();
			for( size_t m = 0; m < names.size(); m++ ){
				FlowerModel model;
				if( readFlower( names[m], model ) ){
					models.push_back( model );
				}
			}
		}
		plantKinds[set] = (int)models.size();
		plantMaterial[set] = pcfg != NULL && pcfg->findChild( "material" ) != NULL ? pcfg->findChild( "material" )->getValue() : "Tumbu/FieldFlower";
		if( models.empty() ){
			continue;
		}
		// the plants sway and part with the grass
		setVertexParam( plantMaterial[set], "windParams", vertexParam( material, "windParams", Ogre::Vector4( 0.8f, 0.6f, 0.35f, 9 ) ) );
		setVertexParam( plantMaterial[set], "pushParams", vertexParam( material, "pushParams", Ogre::Vector4( 1.2f, 1.6f, 1.6f, 1.2f ) ) );
		const int clumps		= (int)configValue( pcfg, "clumps", 10.0f );
		const float ringFrom	= configValue( pcfg, "ringFrom", 14.5f );
		const float ringTo		= configValue( pcfg, "ringTo", 19.0f );
		const float clumpRadius	= configValue( pcfg, "clumpRadius", 0.6f );
		const int perClumpMin	= (int)configValue( pcfg, "perClumpMin", 3.0f );
		const int perClumpMax	= std::max( perClumpMin, (int)configValue( pcfg, "perClumpMax", 6.0f ) );
		const float scaleMin	= configValue( pcfg, "scale", 0.85f );
		const float scaleJitter	= configValue( pcfg, "scaleJitter", 0.35f );
		const float leanMax		= configValue( pcfg, "lean", 8.0f );
		const float wind		= configValue( pcfg, "wind", 1.0f );
		// a clump that found no ground (all its tries on the earth, the snow or under the walls) tries another spot
		const int retries		= (int)configValue( pcfg, "retries", 0.0f );
		std::mt19937 frng( (unsigned)configValue( pcfg, "seed", 77.0f ) );
		// every kind in turn, from a shuffled start, so a few clumps already show them all
		const int firstKind = (int)( frng() % models.size() );
		for( int c = 0; c < clumps; c++ ){
			const FlowerModel &model = models[( firstKind + c ) % models.size()];
			int grown = 0;
			float cx = 0, cz = 0;
			for( int attempt = 0; attempt <= retries && grown == 0; attempt++ ){
				const float angle = ( c + 0.2f + 0.6f * U( frng ) ) / clumps * Ogre::Math::TWO_PI;
				const float radius = ringFrom + U( frng ) * ( ringTo - ringFrom );
				cx = std::cos( angle ) * radius;
				cz = std::sin( angle ) * radius;
				const int count = perClumpMin + (int)( frng() % ( perClumpMax - perClumpMin + 1 ) );
				for( int f = 0, tries = 0; f < count && tries < count * 6; tries++ ){
					const float a = U( frng ) * Ogre::Math::TWO_PI, d = std::sqrt( U( frng ) ) * clumpRadius;
					const float x = cx + std::cos( a ) * d, z = cz + std::sin( a ) * d;
					Ogre::Vector3 root, normal;
					float tall;
					if( ground( x, z, frng, root, normal, tall ) != 0 ){
						continue;
					}
					f++;
					const float scale = scaleMin + scaleJitter * U( frng );
					const float yaw = U( frng ) * Ogre::Math::TWO_PI, leanDir = U( frng ) * Ogre::Math::TWO_PI;
					const Ogre::Quaternion turn = Ogre::Quaternion( Ogre::Degree( leanMax * U( frng ) ), Ogre::Vector3( std::cos( leanDir ), 0, std::sin( leanDir ) ) )
						* Ogre::Quaternion( Ogre::Radian( yaw ), Ogre::Vector3::UNIT_Y );
					const float tone = U( frng ), seed = U( frng );
					const float height = model.height * scale;
					Chunk &chunk = built[chunkKey( x, z )];
					const Ogre::uint32 base = (Ogre::uint32)chunk.plantCount( set );
					for( size_t v = 0; v < model.positions.size(); v++ ){
						const Ogre::Vector3 p = root + turn * ( model.positions[v] * scale );
						const Ogre::Vector3 n = turn * model.normals[v];
						const Ogre::ColourValue &col = model.colours[v];
						const float t = Ogre::Math::Clamp( model.positions[v].y / model.height, 0.0f, 1.0f );
						const float data[18] = { p.x, p.y, p.z, n.x, n.y, n.z, t, wind, tone, seed, root.x, root.y, root.z, height,
							col.r, col.g, col.b, col.a };
						chunk.plantVertices[set].insert( chunk.plantVertices[set].end(), data, data + 18 );
					}
					for( size_t k = 0; k < model.indices.size(); k++ ){
						chunk.plantIndices[set].push_back( base + model.indices[k] );
					}
					grown++;
					plants[set]++;
				}
			}
			if( grown > 0 ){
				plantClumps[set]++;
				Ogre::LogManager::getSingleton().logMessage( "[Grass] " + Ogre::String( PLANT_TYPES[set] ) + " clump: "
					+ Ogre::StringConverter::toString( grown ) + " " + model.name + " at " + Ogre::StringConverter::toString( cx ) + " "
					+ Ogre::StringConverter::toString( cz ) );
			}
		}
	}

	size_t vertices = 0, grassChunks = 0;
	for( std::map<int, Chunk>::iterator c = built.begin(); c != built.end(); c++ ){
		const Chunk &chunk = c->second;
		for( int set = 0; set < PLANT_SETS; set++ ){
			if( chunk.plantCount( set ) == 0 ){
				continue;
			}
			Ogre::ManualObject* fo = mSceneMgr->createManualObject( PLANT_OBJECTS[set] + Ogre::StringConverter::toString( c->first ) );
			fo->setCastShadows( false );
			fo->estimateVertexCount( chunk.plantCount( set ) );
			fo->estimateIndexCount( chunk.plantIndices[set].size() );
			fo->begin( plantMaterial[set], Ogre::RenderOperation::OT_TRIANGLE_LIST, "Game" );
			for( size_t v = 0; v < chunk.plantCount( set ); v++ ){
				const float* d = &chunk.plantVertices[set][v * 18];
				fo->position( d[0], d[1], d[2] );
				fo->normal( d[3], d[4], d[5] );
				fo->colour( d[14], d[15], d[16], d[17] );
				fo->textureCoord( d[6], d[7], d[8], d[9] );
				fo->textureCoord( d[10], d[11], d[12], d[13] );
			}
			for( size_t k = 0; k < chunk.plantIndices[set].size(); k++ ){
				fo->index( chunk.plantIndices[set][k] );
			}
			fo->end();
			( set == 0 ? flowerNode : reedNode )->attachObject( fo );
			chunks.push_back( fo );
		}
		if( chunk.count() == 0 ){
			continue;
		}
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
		grassChunks++;
	}
	Ogre::LogManager::getSingleton().logMessage( "[Grass] " + Ogre::StringConverter::toString( (int)placed ) + " blades, "
		+ Ogre::StringConverter::toString( (int)vertices ) + " vertices in " + Ogre::StringConverter::toString( (int)grassChunks )
		+ " chunks (" + Ogre::StringConverter::toString( (int)candidates ) + " tried: " + Ogre::StringConverter::toString( (int)onEarth )
		+ " on earth or snow, " + Ogre::StringConverter::toString( (int)covered ) + " covered) in "
		+ Ogre::StringConverter::toString( (int)timer.getMilliseconds() ) + " ms; " + Ogre::StringConverter::toString( plants[0] )
		+ " flowers in " + Ogre::StringConverter::toString( plantClumps[0] ) + " clumps of " + Ogre::StringConverter::toString( plantKinds[0] )
		+ " kinds; " + Ogre::StringConverter::toString( plants[1] ) + " reeds in " + Ogre::StringConverter::toString( plantClumps[1] )
		+ " clumps of " + Ogre::StringConverter::toString( plantKinds[1] ) + " kinds" );
}

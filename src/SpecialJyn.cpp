#include "SpecialJyn.h"
#include "TUMBU.h"
#include "Robot.h"
#include "GUI.h"
#include "SoundManager.h"
#include "EffectsManager.h"
#include "Effect.h"
//-------------------------------------------------------------------------------------
SpecialJyn::SpecialJyn( Ogre::SceneManager* _sceneMgr, Ogre::SceneNode* _particleSystemNode, Robot* _speller, Physics::DynamicsWorld* _world, int _count, float _damage ){
	world = _world;
	sceneMgr = _sceneMgr;
	particleSystemNode = _particleSystemNode;
	robotSpeller = _speller;
	count = _count;
	damage = _damage;
	
	times = 0;
	timePSO = 0;
	melhorFitness = 9999;
	everBestFitness = 9999;
	everBestPosition = Ogre::Vector3::ZERO;

	particulaMaiorFitness = 0;
	melhorParticula = 0;
	tamanhoMaiorParticula = 0;

	explosionParticleSystem = NULL;	
	specialShape = NULL;
	specialRigidNode = NULL;
	throwEffect = NULL;
	specialLight = NULL;

	Ogre::String lightName = robotSpeller->robotName +  "_jyn_light_ " + Ogre::StringConverter::toString(count);
	Ogre::String particleSystemName = robotSpeller->robotName +  "_jyn_particle_ " +  Ogre::StringConverter::toString(count);

	specialLight = sceneMgr->createLight( lightName );
	specialLight->setType(Ogre::Light::LT_POINT);
	specialLight->setDiffuseColour(PARTICLE_LIGHT_DIFFUSE_COLOR);
	specialLight->setSpecularColour(PARTICLE_LIGHT_SPECULAR_COLOR);
	specialLight->setAttenuation(1, 0, 0, 500);
	// Ogre 14 lights are positioned by their scene node.
	sceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject( specialLight );
	specialLight->getParentSceneNode()->setPosition(0, 10000, 0);

	particleSystem = sceneMgr->createParticleSystem();
	explosionParticleSystem = sceneMgr->createParticleSystem(particleSystemName, "TumbuSpecialExplosion");
	explosionParticleSystem->setEmitting(false);
	explosionParticleSystem->getEmitter(0)->setEnabled(false);

	particleSystemNode->attachObject( particleSystem );

	createRandomParticles();
	// The Genki Dama's light (and later its trail): it follows the ball and grows as the swarm merges into it.
	startBallEffect( "jyn_ball", targetVector, getKiColour( "jyn" ) );

	setSpecialStatus( SpecialInterface::NONE );

}
//-------------------------------------------------------------------------------------
SpecialJyn::~SpecialJyn(void){
	clear();

	if( specialLight != NULL ){
		sceneMgr->destroySceneNode( specialLight->getParentSceneNode() );
		sceneMgr->destroyLight(specialLight);
	}

	explosionParticleSystem->clear();

	if( explosionParticleSystem->isAttached() ){
		explosionParticleSystem->getParentSceneNode()->detachObject( explosionParticleSystem );
	}

	sceneMgr->destroyParticleSystem( particleSystem );
	sceneMgr->destroyParticleSystem( explosionParticleSystem );

	setSpecialStatus( SpecialInterface::FINISHED );
}
//-------------------------------------------------------------------------------------
void SpecialJyn::update(const Ogre::Real time){

	// From the cast until the ball is thrown, the whole swarm (and its PSO memory) follows the speller.
	if ( getSpecialStatus() == SpecialInterface::NONE || getSpecialStatus() == SpecialInterface::CONCENTRATING
		|| getSpecialStatus() == SpecialInterface::CONCENTRATED ){
		moverTodasParticulas( getChargeAnchor() );
	}

	if ( getSpecialStatus() == SpecialInterface::CONCENTRATING ){
		timePSO += time;
		if( timePSO > 0.05f ) {
			timePSO -= 0.05f;

			if(times < MAX_ITERATIONS){
				times++;
			
				//Avalia o Desempenho de todas partículas
				avaliarDesempenhoTodos();

				executaComplementoPSO();

				executaPSO(0.05f);

				//Se fitness Medio das particulas chego a 0.001 parar de fazer
				if(fitnessMedio < 0.2f){
					times = MAX_ITERATIONS;
					setSpecialStatus( SpecialInterface::CONCENTRATED );
				}
			}else{
				setSpecialStatus( SpecialInterface::CONCENTRATED );
			}
		}
	}else if( getSpecialStatus() == SpecialInterface::ATTACKING ){
		timeToResest -= time;
			
		if( specialShape != NULL ){
			particleList[melhorParticula]->particle->mPosition = specialRigidBodyList.front()->getWorldPosition();
		}

		if(timeToResest <= 0){
			setSpecialStatus( SpecialInterface::FINISHED );
		}
	}else if( getSpecialStatus() == SpecialInterface::HITTED ){
		clear();
		setSpecialStatus( SpecialInterface::FADEOUT );
	}else if( getSpecialStatus() == SpecialInterface::FADEOUT ){
		timeToResest -= time;

		if(timeToResest <= 0){
			setSpecialStatus( SpecialInterface::FINISHED );
		}
	}

	if( !particleList.empty() ){
		// Charging: the light grows with the ball (each merged swarm ball adds to it); thrown: full strength.
		Ogre::Real grown = Ogre::Math::saturate( tamanhoMaiorParticula / ( ( NUMBER_OF_PARTICLES - 1 ) * ( PARTICLE_WIDTH + PARTICLE_HEIGHT ) / 20 ) );
		Ogre::Real intensity = getSpecialStatus() == SpecialInterface::ATTACKING ? 1.0f : 0.3f + 0.7f * grown;
		updateBallEffect( particleList[melhorParticula]->particle->mPosition, intensity );
		updateStreams();
		if( throwEffect != NULL ){
			throwEffect->setPosition( particleList[melhorParticula]->particle->mPosition );
		}
	}
}
//-------------------------------------------------------------------------------------
void SpecialJyn::collision( CollisionDetectionListener *other ){
	if( getSpecialStatus() == SpecialInterface::ATTACKING ){
		switch( other->objectTag ){
		case TumbuEnums::TERRAIN:
			hitScenario( particleList[melhorParticula]->particle->mPosition );
			break;
		case TumbuEnums::SCENE_OBJECT:
			hitScenario( particleList[melhorParticula]->particle->mPosition );
			break;
		case TumbuEnums::ROBOT:
			Robot *enemy;

			btRigidBody* otherRigidBody = other->getOgreBulletRigidBody( other->rigidBodyName )->getBulletRigidBody();

			std::list<Robot*>::iterator j;
			j = robotSpeller->enemyList.begin();
			while ( j != robotSpeller->enemyList.end() ){
				enemy = (*j);

				if( enemy->charRigidBody->getBulletRigidBody() == otherRigidBody ){
					enemy->emmitSound( "specialJynSound", "explosion", "explosion.ogg", false, true, 0.0f);
					hit( enemy->robotNode );
					float dano = enemy->sofrerDano( robotSpeller->criarDano(damage) );
					GUI::getInstance()->addSkillHit( enemy->robotNode->getPosition(), Ogre::StringConverter::toString( dano ) );
					break;
				}
				j++;
			}
			break;
		}
	}
}
//-------------------------------------------------------------------------------------
Physics::RigidBody* SpecialJyn::getOgreBulletRigidBody( const std::string& instanceName ){
	Physics::RigidBody 
		*returnObject = NULL,
		*specialRigid;

	std::list<Physics::RigidBody*>::iterator i = specialRigidBodyList.begin();
	while ( i != specialRigidBodyList.end() ){
		specialRigid = (*i);
		if( specialRigid->getName() == instanceName ){
			returnObject = specialRigid;
			break;
		}
		i++;
	}

	return returnObject;
}
//-------------------------------------------------------------------------------------
void SpecialJyn::concentrate(){
	setSpecialStatus( SpecialInterface::CONCENTRATING );
}
//-------------------------------------------------------------------------------------
void SpecialJyn::attack(Ogre::Quaternion orientation){
	setSpecialStatus( SpecialInterface::ATTACKING );

	releaseStreams();
	if( EffectsManager::getInstance() != NULL && !particleList.empty() ){
		throwEffect = EffectsManager::getInstance()->spawn( "jyn_throw", particleList[melhorParticula]->particle->mPosition, getKiColour( "jyn" ), true );
	}

	Physics::RigidBody* specialRigidBody;

	Ogre::String nodeName = robotSpeller->robotName + "_jyn_node_" + Ogre::StringConverter::toString(count);
	Ogre::String rightBodyName = robotSpeller->robotName + "_jyn_rigidbody_" + Ogre::StringConverter::toString(count);

	specialRigidNode = particleSystemNode->getParentSceneNode()->getParentSceneNode()->createChildSceneNode( nodeName );

	specialShape = new Physics::BoxCollisionShape( Ogre::Vector3( tamanhoMaiorParticula / 2, tamanhoMaiorParticula / 2, tamanhoMaiorParticula / 2 ) );
	specialRigidBody = new Physics::RigidBody( rightBodyName, world );

	specialRigidBody->setShape( specialRigidNode, 
		specialShape,
		0.1f,         // dynamic body restitution
		1.0f,         // dynamic body friction
		30,          // dynamic bodymass
		Ogre::Vector3( particleSystemNode->getParentSceneNode()->getPosition().x, particleSystemNode->getParentSceneNode()->getPosition().y + 2.5f, particleSystemNode->getParentSceneNode()->getPosition().z ), // starting position of the box
		orientation
	);// orientation of the box

	specialRigidBody->getBulletObject()->activate(true);

	Ogre::Vector3 translation = orientation * Ogre::Vector3(0,-100, 1000);

	specialRigidBody->applyImpulse( 
		translation, Ogre::Vector3(0, 0, 0) );

	specialRigidBodyList.push_back( specialRigidBody );

	TUMBU::getInstance()->addCollisionDetectionListener( this, specialRigidBody->getName() );

	timeToResest = 4;
}
//-------------------------------------------------------------------------------------
void SpecialJyn::hitScenario( Ogre::Vector3 position ){
	std::list<Physics::RigidBody*>::iterator i = specialRigidBodyList.begin();
	while ( i != specialRigidBodyList.end() ){
		(*i)->getShape()->getBulletShape()->setLocalScaling( btVector3( 7, 7, 7 ) );
		i++;
	}

	TUMBU::getInstance()->getActiveSceneNode()->attachObject( explosionParticleSystem );

	explosionParticleSystem->setEmitting(true);
	explosionParticleSystem->getEmitter(0)->setEnabled(true);
	explosionParticleSystem->getEmitter(0)->setPosition( position );

	Sound *explosion = SoundManager::getInstance()->createSound("explosion", "explosion.ogg", position, false, false );

	explosion->setDelayToPlay( 0.0f );

	setSpecialStatus( SpecialInterface::HITTED );
}
//-------------------------------------------------------------------------------------
void SpecialJyn::hit(Ogre::SceneNode* hittedNode){
	std::list<Physics::RigidBody*>::iterator i = specialRigidBodyList.begin();
	while ( i != specialRigidBodyList.end() ){
		(*i)->getShape()->getBulletShape()->setLocalScaling( btVector3( 7, 7, 7 ) );
		i++;
	}

	hittedNode->attachObject(explosionParticleSystem);

	explosionParticleSystem->setEmitting(true);
	explosionParticleSystem->getEmitter(0)->setEnabled(true);

	setSpecialStatus( SpecialInterface::HITTED );
}
//-------------------------------------------------------------------------------------
void SpecialJyn::toDelete(){
	setSpecialStatus( SpecialInterface::TO_DELETE );
}
//-------------------------------------------------------------------------------------
void SpecialJyn::clear(){
	releaseBallEffect();
	releaseStreams();
	if( throwEffect != NULL ){
		throwEffect->release();
		throwEffect = NULL;
	}
	times = 0;
    timePSO = 0;
    melhorFitness = 9999;
    everBestFitness = 9999;
    everBestPosition = Ogre::Vector3::ZERO;

    particulaMaiorFitness = 0;
    melhorParticula = 0;
    tamanhoMaiorParticula = 0;

	for(unsigned int i = 0; i < particleList.size(); i++){
		delete particleList[i];
    }
    
	particleList.clear();
    particleSystem->clear();

	std::list<Physics::RigidBody*>::iterator i = specialRigidBodyList.begin();
	while ( i != specialRigidBodyList.end() ){
		TUMBU::getInstance()->removeCollisionDetectionListener( (*i)->getName() );
		delete *i;
		i = specialRigidBodyList.erase(i);
	}
	specialRigidBodyList.clear();

    if( specialShape != NULL ){
		delete specialShape;
		specialShape = NULL;
	}

	if( specialRigidNode != NULL ){
		specialRigidNode->removeAndDestroyAllChildren();
		sceneMgr->destroySceneNode(specialRigidNode);
		specialRigidNode = NULL;
	}
        
    if( specialLight != NULL){
		sceneMgr->destroySceneNode( specialLight->getParentSceneNode() );
		sceneMgr->destroyLight(specialLight);
		specialLight = NULL;
    }

	timeToResest = 4;
}
//-------------------------------------------------------------------------------------
void SpecialJyn::createRandomParticles(){
	Ogre::Real fitnessAtual;

	times = 0;
	timePSO = 0;

	tamanhoMaiorParticula = 0;
	targetVector = getChargeAnchor();

	particleSystem->_update(1);
	particleSystem->setDefaultDimensions( PARTICLE_WIDTH, PARTICLE_HEIGHT );
	particleSystem->setMaterialName(JYN_ORB_MATERIAL);
	particleSystem->setSpeedFactor(0);

	Ogre::ColourValue kiColour = getKiColour( "jyn" );
	EnergyParticle* particula;

	//Cria Particulas (objeto EnergyParticle) com posicoes e velocidades randomicas
	for( int i = 0; i < NUMBER_OF_PARTICLES; i++){
		particula = new EnergyParticle( particleSystem->createParticle() );
		
		particula->active = true;

		Ogre::Vector3 position(	Ogre::Math::RangeRandom( targetVector.x - 10, targetVector.x + 10), 
			Ogre::Math::RangeRandom(targetVector.y -2, targetVector.y + 2), 
			Ogre::Math::RangeRandom(targetVector.z - 10, targetVector.z + 10));

		Ogre::Vector3 velocity(	Ogre::Math::RangeRandom(-4.0f, 4.0f), 
			Ogre::Math::RangeRandom(-5.0f, 5.0f), 
			Ogre::Math::RangeRandom(-4.0f, 4.0f));
			
		fitnessAtual = Ogre::Real(avaliarDesempenho(position));

		setOrbSize( particula->particle, PARTICLE_WIDTH );
		particula->particle->mTimeToLive = PARTICLE_LIVE_TIME;
		particula->particle->mColour = orbColour( kiColour );
		particula->particle->mDirection = Ogre::Vector3::ZERO;
		particula->particle->mRotationSpeed = 0;
		particula->particle->mPosition = Ogre::Vector3(position);
		
		particula->velocity = velocity;
		particula->fitness = fitnessAtual;
		particula->bestFitness = fitnessAtual;
		particula->bestPosition = position;

		if( EffectsManager::getInstance() != NULL ){
			particula->stream = EffectsManager::getInstance()->spawn( "jyn_mote", position, kiColour, true );
		}

		particleList.push_back(particula);
	}
	everBestFitness = particleList[0]->fitness;
	everBestPosition = particleList[0]->particle->mPosition;
	fitnessMedio = 1000;
}
//-------------------------------------------------------------------------------------
void SpecialJyn::executaPSO(Ogre::Real time){
	int g = 0;
	double inertia = 0;
	Ogre::Vector3 globalBest;

	//Para cada partícula
	for(int i = 0; i < NUMBER_OF_PARTICLES; i++){
		g = i;
		//Verifica se a partícula atual está com melhor fitness do que seu histórico local
		if( particleList[i]->fitness < particleList[i]->bestFitness ){
			particleList[i]->bestFitness = particleList[i]->fitness;
			particleList[i]->bestPosition = particleList[i]->particle->mPosition;
		}

		//Verifica seus vizinhos para saber qual é o vizinho com melhor fitness
		for(int j = 0; j < NUMBER_OF_PARTICLES; j++){
			if( particleList[j]->fitness < particleList[g]->fitness ){
				g = j;
				globalBest = particleList[j]->particle->mPosition;
			}
		}

		//Verifica se o fitness do melhor é o melhor de todos (Parecido com o salvacionismo)
		if( particleList[g]->fitness < everBestFitness ){
			everBestFitness = particleList[g]->fitness;
			everBestPosition = particleList[g]->particle->mPosition;
		}

		//Calcula a inercia baseada em quantas iterações faltam
		inertia = 1 - ( (double)times / MAX_ITERATIONS );

		/*
		* Calcula a velocidade pela seguinte formula
		* velocidade = (velocidade * inercia) + (( constanteDeAceleracao1 * numeroRandomicoDe0A1)) * ( melhorPosicaoLocal - posicaoAtual)) + (( constanteDeAceleracao2 * numeroRandomicoDe0A1)) * ( melhorPosicaoDeSempre - posicaoAtual))
		*/
		double randomA = time * Ogre::Math::RangeRandom(0,1),
		       randomB = time * Ogre::Math::RangeRandom(0,1);

		double	newX = (particleList[i]->velocity.x * inertia) + (randomA * ( particleList[i]->bestPosition.x - particleList[i]->particle->mPosition.x )) + (randomB * ( everBestPosition.x - particleList[i]->particle->mPosition.x)),
				newY = (particleList[i]->velocity.y * inertia) + (randomA * ( particleList[i]->bestPosition.y - particleList[i]->particle->mPosition.y )) + (randomB * ( everBestPosition.y - particleList[i]->particle->mPosition.y)),
				newZ = (particleList[i]->velocity.z * inertia) + (randomA * ( particleList[i]->bestPosition.z - particleList[i]->particle->mPosition.z )) + (randomB * ( everBestPosition.z - particleList[i]->particle->mPosition.z));
					

		Ogre::Vector3 newVelocidade;
		newVelocidade.x = Ogre::Real(newX);
		newVelocidade.y = Ogre::Real(newY);
		newVelocidade.z = Ogre::Real(newZ);

		/** Calcula a posicao */
		Ogre::Vector3 newPosition(
			particleList[i]->particle->mPosition.x + newVelocidade.x,
			particleList[i]->particle->mPosition.y + newVelocidade.y,
			particleList[i]->particle->mPosition.z + newVelocidade.z);

		particleList[i]->velocity = newVelocidade;
		particleList[i]->particle->mPosition = newPosition;
	}
}
//-------------------------------------------------------------------------------------
void SpecialJyn::executaComplementoPSO(){
	int g = 0;

 	// Varre todas as particulas para saber qual tem melhor fitness
	for(int j = 0; j < NUMBER_OF_PARTICLES; j++){
		if(particleList[j]->active){
			setOrbSize( particleList[j]->particle, PARTICLE_WIDTH );
		}
		if( particleList[j]->fitness < particleList[g]->fitness ){
			g = j;
		}
	}


	for(int j = 0; j < NUMBER_OF_PARTICLES; j++){
		if( j != g ){
			
			if( particleList[j]->fitness < ( 0.3f + PARTICLE_WIDTH/2 + tamanhoMaiorParticula/2 ) ){
				if(particleList[j]->active){
					tamanhoMaiorParticula += (PARTICLE_WIDTH + PARTICLE_HEIGHT) / 20;
				}
				particleList[j]->particle->setDimensions(0,0);
				particleList[j]->active = false;
			}
		}
	}


	//Altera Cor e Tamanho da melhor Particula
	setOrbSize( particleList[g]->particle, PARTICLE_WIDTH + tamanhoMaiorParticula );
	particleList[g]->active = true;

	melhorParticula = g;
}
//-------------------------------------------------------------------------------------
void SpecialJyn::avaliarDesempenhoTodos(){
	fitnessMedio = 0;
	particulaMaiorFitness = 0;
	melhorFitness = 999999;

	for(int i = 0; i < NUMBER_OF_PARTICLES; i++){
		particleList[i]->fitness = avaliarDesempenho( particleList[i]->particle->mPosition );
		fitnessMedio += particleList[i]->fitness;

		if( particleList[i]->fitness < melhorFitness ){
			melhorFitness = particleList[i]->fitness;
			particulaMaiorFitness = i;
		}
	}
	fitnessMedio /= NUMBER_OF_PARTICLES;
}
//-------------------------------------------------------------------------------------
double SpecialJyn::avaliarDesempenho(const Ogre::Vector3 pos){
	double euclideanDistance;

	euclideanDistance = Ogre::Math::Sqrt( Ogre::Math::Pow( targetVector.x - pos.x, 2) + 
										  Ogre::Math::Pow( targetVector.y - pos.y , 2) + 
									      Ogre::Math::Pow( targetVector.z - pos.z, 2) 
										);

	return euclideanDistance;
}
//-------------------------------------------------------------------------------------
void SpecialJyn::moverTodasParticulas(Ogre::Vector3 moveTarget){
	// Translate the swarm, the personal bests, the global best and the target by how far the anchor moved, so
	// the PSO keeps converging (and the ball keeps its place) while the speller walks.
	Ogre::Vector3 delta = moveTarget - targetVector;
	if( delta != Ogre::Vector3::ZERO ){
		for(unsigned int i = 0; i < particleList.size(); i++){
			particleList[i]->particle->mPosition += delta;
			particleList[i]->bestPosition += delta;
		}
		everBestPosition += delta;
		targetVector = moveTarget;
	}

	if( specialLight != NULL && !particleList.empty() ){
		specialLight->getParentSceneNode()->setPosition( particleList[melhorParticula]->particle->mPosition );
	}
}
//-------------------------------------------------------------------------------------
Ogre::Vector3 SpecialJyn::getChargeAnchor(void){
	// The ball gathers above the speller's head (its raised hand in pre_special_jyn).
	return robotSpeller->robotNode->_getDerivedPosition() + Ogre::Vector3( 0, 2.5f, 0 );
}
//-------------------------------------------------------------------------------------
void SpecialJyn::updateStreams(void){
	for( unsigned int i = 0; i < particleList.size(); i++ ){
		EnergyParticle* ball = particleList[i];
		if( ball->stream == NULL ){
			continue;
		}
		if( ball->active ){
			ball->stream->setPosition( ball->particle->mPosition );
		}else{
			// Merged into the Genki Dama: its stream fades out where it ended.
			ball->stream->release();
			ball->stream = NULL;
		}
	}
}
//-------------------------------------------------------------------------------------
void SpecialJyn::releaseStreams(void){
	for( unsigned int i = 0; i < particleList.size(); i++ ){
		if( particleList[i]->stream != NULL ){
			particleList[i]->stream->release();
			particleList[i]->stream = NULL;
		}
	}
}
//-------------------------------------------------------------------------------------

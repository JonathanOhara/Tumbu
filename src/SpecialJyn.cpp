#include "SpecialJyn.h"
#include "TUMBU.h"
#include "Robot.h"
#include "Part.h"
#include "GUI.h"
#include "SoundManager.h"
#include "EffectsManager.h"
#include "Effect.h"
#include "ConfigScript.h"
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
	tamanhoVisivel = 0;
	convergiu = false;
	// skills.object, skill jyn: "gather homing" (default) or "gather pso" (the 2011 Particle Swarm Optimization).
	ConfigNode* gather = ConfigScriptLoader::getSingleton().getConfigScript( "skill", "jyn" )->findChild( "gather" );
	usePSO = gather != NULL && gather->getValue() == "pso";

	specialShape = NULL;
	specialRigidNode = NULL;
	throwEffect = NULL;
	chargeEffect = NULL;
	particleSystem = sceneMgr->createParticleSystem();

	particleSystemNode->attachObject( particleSystem );

	createRandomParticles();

	setSpecialStatus( SpecialInterface::NONE );

}
//-------------------------------------------------------------------------------------
SpecialJyn::~SpecialJyn(void){
	clear();

	sceneMgr->destroyParticleSystem( particleSystem );

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
		if( !usePSO ){
			executaHoming( time );
		}else{
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
					convergiu = true;
				}
			}else{
				convergiu = true;
			}
		}
		}
		// Ready to throw once the swarm has converged and the ball has finished growing.
		if( convergiu && tamanhoVisivel >= tamanhoMaiorParticula - 0.001f ){
			setSpecialStatus( SpecialInterface::CONCENTRATED );
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
		// The impact effects (EffectsManager) play on their own: the special is over at once, so the speller lowers its
		// arm right after the explosion (the 2011 fade-out waited 4 s for the old explosion particles).
		clear();
		setSpecialStatus( SpecialInterface::FINISHED );
	}

	if( !particleList.empty() ){
		bool charging = getSpecialStatus() <= SpecialInterface::CONCENTRATED;
		if( charging ){
			// The ball sits at the gathering point above the hand (where the light, the lightning and the motes are).
			// Homing: always (particle 0 is the ball). PSO: once the first swarm ball has merged.
			if( !usePSO ){
				particleList[melhorParticula]->particle->mPosition = targetVector;
			}else if( tamanhoMaiorParticula > 0 ){
				EnergyParticle* ball = particleList[melhorParticula];
				ball->particle->mPosition += ( targetVector - ball->particle->mPosition ) * std::min( 1.0f, 8.0f * time );
				ball->velocity = Ogre::Vector3::ZERO;
				ball->bestPosition = targetVector;
			}
			// Each swarm ball that arrives makes the ball grow a little: the shown size eases towards the merged size
			// (about a second to catch up), instead of jumping.
			Ogre::Real missing = tamanhoMaiorParticula - tamanhoVisivel;
			if( missing > 0 ){
				tamanhoVisivel += std::min( missing, std::max( missing * 4.0f, 0.06f ) * time );
			}
			if( !usePSO ){
				// Homing: the ball grows from nothing to its full size as the swarm balls arrive.
				Ogre::Real full = ( NUMBER_OF_PARTICLES - 1 ) * ( PARTICLE_WIDTH + PARTICLE_HEIGHT ) / 20;
				setOrbSize( particleList[melhorParticula]->particle, ( PARTICLE_WIDTH + full ) * tamanhoVisivel / full );
			}else if( getSpecialStatus() != SpecialInterface::NONE ){
				setOrbSize( particleList[melhorParticula]->particle, PARTICLE_WIDTH + tamanhoVisivel );
			}
		}
		// Charging: the light and the lightning grow with the ball (0 until the first swarm ball arrives); thrown: full.
		Ogre::Real grown = Ogre::Math::saturate( tamanhoVisivel / ( ( NUMBER_OF_PARTICLES - 1 ) * ( PARTICLE_WIDTH + PARTICLE_HEIGHT ) / 20 ) );
		Ogre::Real intensity = charging ? 0.3f + 0.7f * grown : 1.0f;
		// The light follows the ball; the gathering (chargeEffect) stays at the raised hand, where the ball settles.
		Ogre::Vector3 ballPosition = particleList[melhorParticula]->particle->mPosition;
		updateBallEffect( ballPosition, intensity );
		// The layers that wrap the ball (lightning, where the motes arrive) measure in its radius.
		Ogre::Real ballRadius = ( PARTICLE_WIDTH + tamanhoVisivel ) * 0.5f;
		if( ballEffect != NULL ){
			ballEffect->setScale( ballRadius );
		}
		if( chargeEffect != NULL ){
			chargeEffect->setPosition( charging ? targetVector : particleList[melhorParticula]->particle->mPosition );
			chargeEffect->setIntensity( grown );
			chargeEffect->setScale( ballRadius );
		}
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

	// The swarm balls show from the cast; the gathering at the hand starts now: they fly in, the Genki Dama's light
	// follows the ball, and the motes, dust and lightning gather at the raised hand.
	Ogre::ColourValue ki = getKiColour( "jyn" );
	for( unsigned int i = 0; i < particleList.size(); i++ ){
		EnergyParticle* ball = particleList[i];
		if( !usePSO ){
			if( i == 0 ){
				continue;	// the Genki Dama itself
			}
			// One after another over about 1.2 s, each flight 0.7..1 s, from where it is now.
			ball->startOffset = ball->particle->mPosition - targetVector;
			ball->flightAge = 0;
			ball->flightDelay = ( i - 1 ) * 1.2f / ( NUMBER_OF_PARTICLES - 1 ) + Ogre::Math::RangeRandom( 0, 0.15f );
			ball->flightTime = Ogre::Math::RangeRandom( 0.7f, 1.0f );
		}
		if( EffectsManager::getInstance() != NULL && ball->stream == NULL ){
			ball->stream = EffectsManager::getInstance()->spawn( "jyn_mote", ball->particle->mPosition, ki, true );
		}
	}
	if( !particleList.empty() ){
		startBallEffect( "jyn_ball", particleList[melhorParticula]->particle->mPosition, ki );
	}
	if( EffectsManager::getInstance() != NULL && chargeEffect == NULL ){
		chargeEffect = EffectsManager::getInstance()->spawn( "jyn_charge", targetVector, ki, true );
	}
}
//-------------------------------------------------------------------------------------
void SpecialJyn::attack(Ogre::Quaternion orientation){
	setSpecialStatus( SpecialInterface::ATTACKING );

	// The gathering winds down around the thrown ball: the motes still on their way chase it (update moves the effect).
	if( chargeEffect != NULL ){
		chargeEffect->stopLayers();
	}
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
		particleList.empty() ? targetVector : particleList[melhorParticula]->particle->mPosition, // starting position: the ball, above the raised hand
		orientation
	);// orientation of the box

	specialRigidBody->getBulletObject()->activate(true);

	// The 2011 throw: forward and a little down. The ball now starts above the raised hand, higher than the old fixed
	// point, so a standing enemy in front is aimed at directly (its chest), at the same speed.
	Ogre::Vector3 translation = orientation * Ogre::Vector3(0,-100, 1000);
	Ogre::Vector3 start = particleList.empty() ? targetVector : particleList[melhorParticula]->particle->mPosition;
	Ogre::Vector3 facing = orientation * Ogre::Vector3::UNIT_Z;
	facing.y = 0;
	facing.normalise();
	Robot* aimed = NULL;
	Ogre::Real nearest = 0;
	for( std::list<Robot*>::iterator e = robotSpeller->enemyList.begin(); e != robotSpeller->enemyList.end(); e++ ){
		Ogre::Vector3 toEnemy = (*e)->robotNode->_getDerivedPosition() - start;
		toEnemy.y = 0;
		Ogre::Real distance = toEnemy.normalise();
		// Roughly in front (within 60 degrees of where the speller faces).
		if( facing.dotProduct( toEnemy ) > 0.5f && ( aimed == NULL || distance < nearest ) ){
			aimed = *e;
			nearest = distance;
		}
	}
	if( aimed != NULL ){
		Ogre::Vector3 chest = aimed->robotNode->_getDerivedPosition() + Ogre::Vector3( 0, 1.1f, 0 );
		translation = ( chest - start ).normalisedCopy() * translation.length();
	}

	// Throw speed (units per second, "throwSpeed" of skill jyn in skills.object): the 2011 impulse gave about 33.5.
	ConfigNode* speedNode = ConfigScriptLoader::getSingleton().getConfigScript( "skill", "jyn" )->findChild( "throwSpeed" );
	Ogre::Real throwSpeed = speedNode != NULL ? speedNode->getValueF() : 33.5f;
	translation = translation.normalisedCopy() * throwSpeed * 30;	// impulse = speed x mass (30, below)

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

	// The Genki Dama explodes (effects.object jyn_impact): flash, hit-stop, explosion ball, shockwave, debris, smoke.
	if( EffectsManager::getInstance() != NULL ){
		EffectsManager::getInstance()->spawn( "jyn_impact", position, getKiColour( "jyn" ) );
	}

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

	if( EffectsManager::getInstance() != NULL && !particleList.empty() ){
		EffectsManager::getInstance()->spawn( "jyn_impact", particleList[melhorParticula]->particle->mPosition, getKiColour( "jyn" ) );
	}

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
	releaseChargeEffect();
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
    tamanhoVisivel = 0;
    convergiu = false;

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
        
	timeToResest = 4;
}
//-------------------------------------------------------------------------------------
void SpecialJyn::createRandomParticles(){
	Ogre::Real fitnessAtual;

	times = 0;
	timePSO = 0;

	tamanhoMaiorParticula = 0;
	tamanhoVisivel = 0;
	convergiu = false;
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
		if( !usePSO ){
			// Homing: the ball (0) at the gathering point; the swarm on a ring 3..6 units around, at varied heights.
			Ogre::Radian angle( Ogre::Math::RangeRandom( 0, Ogre::Math::TWO_PI ) );
			Ogre::Real distance = Ogre::Math::RangeRandom( 3, 6 );
			position = i == 0 ? targetVector : targetVector + Ogre::Vector3( Ogre::Math::Cos( angle ) * distance,
				Ogre::Math::RangeRandom( -1.0f, 2.0f ), Ogre::Math::Sin( angle ) * distance );
		}

		Ogre::Vector3 velocity(	Ogre::Math::RangeRandom(-4.0f, 4.0f), 
			Ogre::Math::RangeRandom(-5.0f, 5.0f), 
			Ogre::Math::RangeRandom(-4.0f, 4.0f));
			
		fitnessAtual = Ogre::Real(avaliarDesempenho(position));

		// The swarm shows from the cast; with homing the ball (0) starts with no size and grows as the swarm arrives.
		setOrbSize( particula->particle, !usePSO && i == 0 ? 0 : PARTICLE_WIDTH );
		particula->particle->mTimeToLive = PARTICLE_LIVE_TIME;
		particula->particle->mColour = orbColour( kiColour );
		particula->particle->mDirection = Ogre::Vector3::ZERO;
		particula->particle->mRotationSpeed = 0;
		particula->particle->mPosition = Ogre::Vector3(position);
		
		particula->velocity = velocity;
		particula->fitness = fitnessAtual;
		particula->bestFitness = fitnessAtual;
		particula->bestPosition = position;


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


	//Altera Cor e Tamanho da melhor Particula (o tamanho cresce aos poucos em update: tamanhoVisivel)
	setOrbSize( particleList[g]->particle, PARTICLE_WIDTH + tamanhoVisivel );
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
}
//-------------------------------------------------------------------------------------
Ogre::Vector3 SpecialJyn::getChargeAnchor(void){
	// The ball gathers above the speller's raised hand (pre_special_jyn raises it): the higher of the two hands, a
	// little above the middle finger. Above the head when the hands cannot be found.
	Ogre::Vector3 best = robotSpeller->robotNode->_getDerivedPosition() + Ogre::Vector3( 0, 2.5f, 0 );
	Ogre::Vector3 hand;
	bool found = false;
	if( getBonePosition( robotSpeller->rightArm, "finger_3_1_R", hand ) ){
		best = hand;
		found = true;
	}
	if( getBonePosition( robotSpeller->leftArm, "finger_3_1_L", hand ) && ( !found || hand.y > best.y ) ){
		best = hand;
		found = true;
	}
	// The ball's lower edge stays 0.25 above the knuckles, however big it has grown.
	Ogre::Real radius = ( PARTICLE_WIDTH + tamanhoVisivel ) * 0.5f;
	return found ? best + Ogre::Vector3( 0, 0.25f + radius, 0 ) : best;
}
//-------------------------------------------------------------------------------------
bool SpecialJyn::getBonePosition( Part* part, const char* boneName, Ogre::Vector3 &position ){
	if( part == NULL || part->entity == NULL || !part->entity->hasSkeleton() || part->entity->getParentSceneNode() == NULL ){
		return false;
	}
	Ogre::SkeletonInstance* skeleton = part->entity->getSkeleton();
	if( !skeleton->hasBone( boneName ) ){
		return false;
	}
	position = part->entity->getParentSceneNode()->_getFullTransform() * skeleton->getBone( boneName )->_getDerivedPosition();
	return true;
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
void SpecialJyn::releaseChargeEffect(void){
	// Gone at once: after the impact (or a timeout) no mote may still be flying to where the ball was.
	if( chargeEffect != NULL ){
		chargeEffect->kill();
		chargeEffect = NULL;
	}
}
//-------------------------------------------------------------------------------------
void SpecialJyn::executaHoming( Ogre::Real time ){
	// Each swarm ball waits its turn, then flies into the gathering point: slow at first, fast at the end, turning a
	// little around it and arcing up. Positions are relative to the gathering point, so they follow the robot. On
	// arrival it merges: the ball grows by one step (tamanhoVisivel then eases towards it in update).
	melhorParticula = 0;
	bool flying = false;
	for( unsigned int i = 1; i < particleList.size(); i++ ){
		EnergyParticle* ball = particleList[i];
		if( !ball->active ){
			continue;
		}
		flying = true;
		ball->flightAge += time;
		Ogre::Real t = ( ball->flightAge - ball->flightDelay ) / ball->flightTime;
		if( t >= 1 ){
			ball->active = false;
			ball->particle->setDimensions( 0, 0 );
			ball->particle->mPosition = targetVector;
			tamanhoMaiorParticula += ( PARTICLE_WIDTH + PARTICLE_HEIGHT ) / 20;
			continue;
		}
		t = std::max<Ogre::Real>( t, 0 );
		Ogre::Real e = t * t;
		Ogre::Quaternion turn( Ogre::Radian( 0.9f * e ), Ogre::Vector3::UNIT_Y );
		Ogre::Vector3 offset = turn * ( ball->startOffset * ( 1 - e ) );
		offset.y += Ogre::Math::Sin( Ogre::Math::PI * t ) * 0.6f;
		ball->particle->mPosition = targetVector + offset;
	}
	if( !flying ){
		convergiu = true;
	}
}
//-------------------------------------------------------------------------------------

#include "Effect.h"
#include "EffectLayers.h"
#include "ConfigScript.h"
//-------------------------------------------------------------------------------------
Effect::Effect( EffectsManager* _manager, ConfigNode* definition, const Ogre::Vector3 &_position, const Ogre::ColourValue &_kiColour, bool _held ){
	manager = _manager;
	position = _position;
	kiColour = _kiColour;
	held = _held;
	age = 0;
	intensity = 1;
	scale = 1;

	std::vector<ConfigNode*> &children = definition->getChildren();
	for( size_t i = 0; i < children.size(); i++ ){
		EffectLayer* layer = createEffectLayer( this, children[i] );
		if( layer != NULL ){
			layers.push_back( layer );
		}
	}
}
//-------------------------------------------------------------------------------------
Effect::~Effect(void){
	for( size_t i = 0; i < layers.size(); i++ ){
		delete layers[i];
	}
	layers.clear();
}
//-------------------------------------------------------------------------------------
bool Effect::update( Ogre::Real time ){
	age += time;
	bool running = false;
	for( size_t i = 0; i < layers.size(); i++ ){
		if( layers[i] == NULL ){
			continue;
		}
		if( layers[i]->update( age, time ) ){
			running = true;
		}else{
			delete layers[i];
			layers[i] = NULL;
		}
	}
	// A held effect stays until its owner releases it, even when nothing is showing yet.
	return running || held;
}
//-------------------------------------------------------------------------------------
void Effect::stopLayers(void){
	for( size_t i = 0; i < layers.size(); i++ ){
		if( layers[i] != NULL ){
			layers[i]->stop();
		}
	}
}
//-------------------------------------------------------------------------------------
void Effect::release(void){
	held = false;
	for( size_t i = 0; i < layers.size(); i++ ){
		if( layers[i] != NULL ){
			layers[i]->stop();
		}
	}
}
//-------------------------------------------------------------------------------------
Ogre::ColourValue Effect::readColour( ConfigNode* node, const Ogre::String &key ){
	ConfigNode* child = node->findChild( key );
	if( child == NULL || child->getValues().empty() ){
		return Ogre::ColourValue::White;
	}
	std::vector<Ogre::String> &values = child->getValues();
	if( values[0] == "ki" ){
		Ogre::Real factor = values.size() > 1 ? Ogre::StringConverter::parseReal( values[1] ) : 1.0f;
		Ogre::ColourValue colour = kiColour * factor;
		colour.a = 1;
		return colour;
	}
	if( values.size() < 3 ){
		Ogre::LogManager::getSingleton().logWarning( "Effect: '" + key + "' needs r g b or ki" );
		return Ogre::ColourValue::White;
	}
	return Ogre::ColourValue( Ogre::StringConverter::parseReal( values[0] ), Ogre::StringConverter::parseReal( values[1] ),
		Ogre::StringConverter::parseReal( values[2] ), 1 );
}
//-------------------------------------------------------------------------------------
Ogre::Real Effect::readReal( ConfigNode* node, const Ogre::String &key, Ogre::Real defaultValue ){
	ConfigNode* child = node->findChild( key );
	if( child == NULL || child->getValues().empty() ){
		return defaultValue;
	}
	return child->getValueF();
}
//-------------------------------------------------------------------------------------
Ogre::Vector3 Effect::readVector( ConfigNode* node, const Ogre::String &key, const Ogre::Vector3 &defaultValue ){
	ConfigNode* child = node->findChild( key );
	if( child == NULL || child->getValues().size() < 3 ){
		return defaultValue;
	}
	return Ogre::Vector3( child->getValueF( 0 ), child->getValueF( 1 ), child->getValueF( 2 ) );
}
//-------------------------------------------------------------------------------------

#ifndef __SkillHitManager_h_
#define __SkillHitManager_h_

#include <Ogre.h>
#include <OIS.h>
#include <CEGUI.h> 
#include <RendererModules/Ogre/CEGUIOgreRenderer.h>

class GUI;

#include "SkillHit.h"

class SkillHitManager {
public:
	SkillHitManager( GUI *_gui);
	SkillHitManager( );
	virtual ~SkillHitManager(void);
	void reset();

	void update(const Ogre::Real time);

	void addSkillHit( Ogre::Vector3 position, Ogre::String message );
protected:
private:
	GUI *gui;

	std::list<SkillHit*> skillHitList;

	CEGUI::Window* label;

	/** Para os iterators */
	SkillHit* _skillHit;
	std::list<SkillHit*>::iterator itSkillHitList;
	std::list<SkillHit*>::iterator itSkillHitListEnd;
};

#endif // #ifndef __Confirm_h_
#ifndef __SkillHit_h_
#define __SkillHit_h_

#include <Ogre.h>
#include <OIS.h>
#include <CEGUI.h> 
#include <RendererModules/Ogre/CEGUIOgreRenderer.h>
#include "Dialog.h"

class SkillHit {
public:
	SkillHit( Ogre::Vector3 _position, Ogre::String _message );
	virtual ~SkillHit(void);

	enum SkillHitStatus { INITIALIZED, CREATED, LIVE, TO_DELETE };

	SkillHitStatus getSkillHitStatus();
	void setSkillHitStatus( SkillHitStatus _skillHitStatus );

	void moveUp(const Ogre::Real time);
protected:
private:
	void initializeLabel();

	CEGUI::Window* label;
	Ogre::String message;

	SkillHitStatus skillHitStatus;

	Ogre::Real liveTime;

	double
		top,
		left,
		height,
		width;
};

#endif // #ifndef __Confirm_h_
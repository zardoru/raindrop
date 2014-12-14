#ifndef NOTESKIN_H_
#define NOTESKIN_H_

class Image;
class LuaManager;

// Structure not yet final. 
class Noteskin
{
	float  NoteHeight;
	Image* GearImage[VSRG::MAX_CHANNELS];
	Image* KeyImage[VSRG::MAX_CHANNELS];
	AABB NoteCrop[VSRG::MAX_CHANNELS];
	Transformation NoteTransformation[VSRG::NK_TOTAL][VSRG::MAX_CHANNELS];
	LuaManager *NoteskinLua;
public:
	Noteskin(GString Name);
	void Update(float Delta);
};

#endif
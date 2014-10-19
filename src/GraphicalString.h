#ifndef GSTRING_H_
#define GSTRING_H_

class GraphicalString : public GraphObject2D
{
	String mText;
	Font* mFont;
public:
	GraphicalString();
	void SetText(String _Text);
	String GetText() const;
	void SetFont(Font* _Font);
	Font* GetFont() const;

	void Render();
};

#endif

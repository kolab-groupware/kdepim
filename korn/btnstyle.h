/*
* btnstyle.h -- Declaration of class KornBtnStyle.
* Generated by newclass on Sat Apr 21 04:45:02 EST 2001.
*/
#ifndef SSK_BTNSTYLE_H
#define SSK_BTNSTYLE_H

class KornButton;
class KMailDrop;

/**
* @short KornBtnStyle
* @author Sirtaj Singh Kang (taj@kde.org)
* @version $Id$
*/
class KornBtnStyle
{
public:
	/**
	* KornBtnStyle Constructor
	*/
	KornBtnStyle( KornButton *btn, KMailDrop *box )
	{ _btn = btn; _box = box;}

	virtual void enable() = 0;
	virtual void disable() = 0;
	virtual void update( int msgcount, bool newMessages ) = 0;

	/**
	* KornBtnStyle Destructor
	*/
	virtual ~KornBtnStyle(){}
	
protected:
	KornButton *_btn;
	KMailDrop *_box;

private:
	KornBtnStyle& operator=( const KornBtnStyle& );
	KornBtnStyle( const KornBtnStyle& );
};

class KornBtnIconStyle : public KornBtnStyle
{
public:
	KornBtnIconStyle( KornButton *btn, KMailDrop *box )
		: KornBtnStyle( btn, box ) {}

	virtual ~KornBtnIconStyle(){}
	virtual void enable(){}
	virtual void disable(){}
	virtual void update( int msgcount, bool newMessages );
};

class KornBtnPlainTextStyle : public KornBtnStyle
{
public:
	KornBtnPlainTextStyle( KornButton *btn, KMailDrop *box )
		: KornBtnStyle( btn, box ){}
	virtual ~KornBtnPlainTextStyle(){}

	virtual void enable();
	virtual void disable();
	virtual void update( int msgcount, bool newMessages );
};

class KornBtnColourTextStyle : public KornBtnStyle
{
public:
	KornBtnColourTextStyle( KornButton *btn, KMailDrop *box )
		: KornBtnStyle( btn, box ){}
	virtual ~KornBtnColourTextStyle(){}

	virtual void enable();
	virtual void disable();
	virtual void update( int msgcount, bool newMessages );
};

#endif // SSK_BTNSTYLE_H

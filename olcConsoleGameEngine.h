/*
OneLoneCoder.com - Command Line Game Engine
"Who needs a frame buffer?" - @Javidx9

Disclaimer
~~~~~~~~~~
I don't care what you use this for. It's intended to be educational, and perhaps
to the oddly minded - a little bit of fun. Please hack this, change it and use it
in any way you see fit. BUT, you acknowledge that I am not responsible for anything
bad that happens as a result of your actions. However, if good stuff happens, I
would appreciate a shout out, or at least give the blog some publicity for me.
Cheers!

Background
~~~~~~~~~~
If you've seen any of my videos - I like to do things using the windows console. It's quick
and easy, and allows you to focus on just the code that matters - ideal when you're 
experimenting. Thing is, I have to keep doing the same initialisation and display code
each time, so this class wraps that up.

Author
~~~~~~
Twitter: @javidx9
Blog: www.onelonecoder.com

Video:
~~~~~~
https://youtu.be/cWc0hgYwZyc

Last Updated: 21/08/2017

Usage:
~~~~~~
This class is abstract, so you must inherit from it. Override the OnUserCreate() function
with all the stuff you need for your application (for thready reasons it's best to do
this in this function and not your class constructor). Override the OnUserUpdate(float fElapsedTime)
function with the good stuff, it gives you the elapsed time since the last call so you
can modify your stuff dynamically. Both functions should return true, unless you need
the application to close.

	int main()
	{
		// Use olcConsoleGameEngine derived app
		OneLoneCoder_Example game;

		// Create a console with resolution 160x100 characters
		// Each character occupies 8x8 pixels
		game.ConstructConsole(160, 100, 8, 8);

		// Start the engine!
		game.Start();

		return 0;
	}

Input is also handled for you - interrogate the m_keys[] array with the virtual
keycode you want to know about. bPressed is set for the frame the key is pressed down
in, bHeld is set if the key is held down, bReleased is set for the frame the key
is released in.

The draw routines treat characters like pixels. By default they are set to white solid
blocks - but you can draw any unicode character, using any of the colours listed below.

There may be bugs! 

See my other videos for examples!

*/

//-----------------------------------------------------------------------------
#pragma once
//-----------------------------------------------------------------------------
#include <string>
#include <atomic>
#include <condition_variable>
#include <mutex>
//-----------------------------------------------------------------------------
#include <filesystem>

#define S2WS(x)		std::filesystem::path(x).wstring()
#define WS2S(x)		std::filesystem::path(x).string()
//-----------------------------------------------------------------------------
#include <windows.h>
//-----------------------------------------------------------------------------

enum COLOUR
{
	FG_BLACK		= 0x0000,
	FG_DARK_BLUE    = 0x0001,	
	FG_DARK_GREEN   = 0x0002,
	FG_DARK_CYAN    = 0x0003,
	FG_DARK_RED     = 0x0004,
	FG_DARK_MAGENTA = 0x0005,
	FG_DARK_YELLOW  = 0x0006,
	FG_GREY         = 0x0007,
	FG_BLUE			= 0x0009,
	FG_GREEN		= 0x000A,
	FG_CYAN			= 0x000B,
	FG_RED			= 0x000C,
	FG_MAGENTA		= 0x000D,
	FG_YELLOW		= 0x000E,
	FG_WHITE		= 0x000F,
	BG_BLACK		= 0x0000,
	BG_DARK_BLUE	= 0x0010,
	BG_DARK_GREEN	= 0x0020,
	BG_DARK_CYAN	= 0x0030,
	BG_DARK_RED		= 0x0040,
	BG_DARK_MAGENTA = 0x0050,
	BG_DARK_YELLOW	= 0x0060,
	BG_GREY			= 0x0070,
	BG_BLUE			= 0x0090,
	BG_GREEN		= 0x00A0,
	BG_CYAN			= 0x00B0,
	BG_RED			= 0x00C0,
	BG_MAGENTA		= 0x00D0,
	BG_YELLOW		= 0x00E0,
	BG_WHITE		= 0x00F0,
};
//-----------------------------------------------------------------------------

enum PIXEL_TYPE
{
	PIXEL_SOLID = 0x2588,
	PIXEL_THREEQUARTERS = 0x2593,
	PIXEL_HALF = 0x2592,
	PIXEL_QUARTER = 0x2591,
};
//-----------------------------------------------------------------------------

class olcSprite
{
private:
	wchar_t *m_Glyphs  = nullptr;
	short   *m_Colours = nullptr;

	void Create(int w, int h);

public:
	olcSprite()	                  {}
	olcSprite(int w, int h)       { Create(w, h); }
	olcSprite(std::wstring sFile) { if(!Load(sFile)) Create(8, 8); }

	int nWidth = 0;
	int nHeight = 0;

	void    SetGlyph(int x, int y, wchar_t c);
	void    SetColour(int x, int y, short c);

	wchar_t GetGlyph(int x, int y);
	short   GetColour(int x, int y);

	bool    Save(std::wstring sFile);
	bool    Load(std::wstring sFile);
};
//-----------------------------------------------------------------------------

class olcConsoleGameEngine
{
private:
	HANDLE                     m_hOriginalConsole;
	CONSOLE_SCREEN_BUFFER_INFO m_OriginalConsoleInfo;
	HANDLE                     m_hConsole;
	SMALL_RECT                 m_rectWindow;
	short*                     m_keyOldState;
	short*                     m_keyNewState;

	void GameThread();

	void handleKeyboardInput();
	void handleMouseInput();

protected:
	int                        m_nScreenWidth;
	int                        m_nScreenHeight;
	CHAR_INFO*                 m_bufScreen;
	std::atomic<bool>          m_bAtomActive;
	std::condition_variable    m_cvGameFinished;
	std::mutex                 m_muxGame;
	std::wstring               m_sAppName;

	struct sKeyState
	{
		bool bPressed;
		bool bReleased;
		bool bHeld;
	} m_keys[256];


	int Error(wchar_t *msg);

public:
	olcConsoleGameEngine();
	virtual ~olcConsoleGameEngine();

	inline int ScreenWidth()  { return m_nScreenWidth;  }
	inline int ScreenHeight() {	return m_nScreenHeight;	}

	int ConstructConsole(int width, int height, int fontw = 12, int fonth = 12);

	void Draw(int x, int y, wchar_t c = 0x2588, short col = 0x000F);
	void DrawString(int x, int y, std::wstring c, short col = 0x000F);
	void DrawStringAlpha(int x, int y, std::wstring c, short col = 0x000F);
	void DrawLine(int x1, int y1, int x2, int y2, wchar_t c = 0x2588, short col = 0x000F);
	void DrawSprite(int x, int y, olcSprite *sprite);
	void DrawPartialSprite(int x, int y, olcSprite *sprite, int ox, int oy, int w, int h);

	void Fill(int x1, int y1, int x2, int y2, wchar_t c = 0x2588, short col = 0x000F);
	void Clip(int &x, int &y);
	void Start();

	// User MUST OVERRIDE THESE!!
	virtual bool OnUserCreate() = 0;
	virtual bool OnUserUpdate(float fElapsedTime) = 0;
};
//-----------------------------------------------------------------------------

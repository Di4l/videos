//---------------------------------------------------------------------------//
//  Adapting the "snake" project by javidx9 to this game engine by the same
//  author I have come to realize that adding a delay on the game loop becomes
//  a problem on detecting key presses.
//
//  option 1: Added a parameter on the engine that allow for a delay between
//  calls to the main loop to be inserted. That "delay" time is spent checking
//  for user input (keys pressed)
//---------------------------------------------------------------------------//


//-----------------------------------------------------------------------------
#include "olcConsoleGameEngine.h"

#include <cwchar>
#include <fstream>
#include <chrono>
#include <vector>
#include <list>
#include <thread>
//-----------------------------------------------------------------------------
using namespace std;
//-----------------------------------------------------------------------------

void olcSprite::Create(int w, int h)
{
	nWidth    = w;
	nHeight   = h;
	m_Glyphs  = new wchar_t[w * h];
	m_Colours = new short[w * h];
	for(int i = 0; i < w * h; ++i)
	{
		m_Glyphs[i]  = L' ';
		m_Colours[i] = FG_BLACK;
	}
}
//-----------------------------------------------------------------------------

void olcSprite::SetGlyph(int x, int y, wchar_t c)
{
	if (x <0 or x > nWidth or y < 0 or y > nHeight)
		return;
	else
		m_Glyphs[y * nWidth + x] = c;
}
//-----------------------------------------------------------------------------

void olcSprite::SetColour(int x, int y, short c)
{
	if (x <0 or x > nWidth or y < 0 or y > nHeight)
		return;
	else
		m_Colours[y * nWidth + x] = c;
}
//-----------------------------------------------------------------------------

wchar_t olcSprite::GetGlyph(int x, int y)
{
	if (x <0 or x > nWidth or y < 0 or y > nHeight)
		return L' ';
	else
		return m_Glyphs[y * nWidth + x];
}
//-----------------------------------------------------------------------------

short olcSprite::GetColour(int x, int y)
{
	if (x <0 or x > nWidth or y < 0 or y > nHeight)
		return FG_BLACK;
	else
		return m_Colours[y * nWidth + x];
}
//-----------------------------------------------------------------------------

bool olcSprite::Save(wstring sFile)
{
	string aux(sFile.begin(), sFile.end());
	ofstream f;
	f.open(aux.c_str(), ios_base::binary);

	f.write((char*)&nWidth, sizeof(int));
	f.write((char*)&nHeight, sizeof(int));
	f.write((char*)m_Colours, sizeof(short) * nWidth * nHeight);
	f.write((char*)m_Glyphs, sizeof(wchar_t) * nWidth * nHeight);

	f.close();
	return true;
}
//-----------------------------------------------------------------------------

bool olcSprite::Load(wstring sFile)
{
	delete[] m_Glyphs;
	delete[] m_Colours;
	nWidth = 0;
	nHeight = 0;

	string aux(sFile.begin(), sFile.end());
	ifstream f;
	f.open(aux.c_str(), ios_base::binary | ios_base::trunc);

	f.read((char*)&nWidth, sizeof(int));
	f.read((char*)&nHeight, sizeof(int));
	Create(nWidth, nHeight);

	f.read((char*)m_Colours, sizeof(short) * nWidth * nHeight);
	f.read((char*)m_Glyphs, sizeof(wchar_t) * nWidth * nHeight);

	f.close();
	return true;
}
//-----------------------------------------------------------------------------












//-----------------------------------------------------------------------------

olcConsoleGameEngine::olcConsoleGameEngine()
{
	m_nScreenWidth  = 80;
	m_nScreenHeight = 30;

	m_hOriginalConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	m_hConsole         = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
										0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	m_keyNewState      = new short[256];
	m_keyOldState      = new short[256];
	m_bufScreen        = nullptr;
	m_sAppName         = L"Default";
	m_LoopDelay        = std::chrono::milliseconds::zero();

	memset(m_keyNewState, 0, 256 * sizeof(short));
	memset(m_keyOldState, 0, 256 * sizeof(short));
	memset(m_keys, 0, 256 * sizeof(sKeyState));
}
//-----------------------------------------------------------------------------

olcConsoleGameEngine::~olcConsoleGameEngine()
{
	SetConsoleActiveScreenBuffer(m_hOriginalConsole);
	if(m_hConsole)
		CloseHandle(m_hConsole);
	if(m_bufScreen)
		delete[] m_bufScreen;
	delete[] m_keyNewState;
	delete[] m_keyOldState;
}
//-----------------------------------------------------------------------------

void olcConsoleGameEngine::GameThread()
{
	// Create user resources as part of this thread
	if (!OnUserCreate())
		return;

	wchar_t s[128];
	auto    tp1 = chrono::system_clock::now();
	auto    tp2 = tp1;
	float   fEtime = 0.0;
	chrono::duration<float> dEtime;

	// Run as fast as possible
	while(m_bAtomActive)
	{
		// Handle Timing
		tp2 = chrono::system_clock::now();

		// Handle Input
		memset(m_keys, 0, 256 * sizeof(sKeyState));
		do
		{
			for(int i = 0; i < 256; ++i)
			{
				m_keyNewState[i] = GetAsyncKeyState(i);
//				m_keys[i].bPressed  = false;
//				m_keys[i].bReleased = false;
				if(m_keyNewState[i] != m_keyOldState[i])
				{
					if(m_keyNewState[i] & 0x8000)
					{
						m_keys[i].bPressed = true;//!m_keys[i].bHeld;
						m_keys[i].bHeld    = true;
					}
					else
					{
						m_keys[i].bReleased = true;
						m_keys[i].bHeld     = false;
					}
				}
				m_keyOldState[i] = m_keyNewState[i];
			}
		} while((std::chrono::system_clock::now() - tp2) < m_LoopDelay);

		// Calculate the time elapsed since last frame
		tp2    = chrono::system_clock::now();
		dEtime = tp2 - tp1;
		tp1    = tp2;
		fEtime = dEtime.count();

		// Handle Frame Update
		if(!OnUserUpdate(fEtime, m_LoopDelay))
			m_bAtomActive = false;

		// Update Title & Present Screen Buffer
		swprintf_s(s, 128, L"OneLoneCoder.com - Console Game Engine - %s - FPS: %3.2f ", m_sAppName.c_str(), 1.0f / fEtime);
		SetConsoleTitleW(s);
		WriteConsoleOutputW(m_hConsole, m_bufScreen, { (short)m_nScreenWidth, (short)m_nScreenHeight }, { 0,0 }, &m_rectWindow);
	}

	m_cvGameFinished.notify_one();
}
//-----------------------------------------------------------------------------

int olcConsoleGameEngine::Error(wchar_t *msg)
{
	wchar_t buf[256];
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 256, NULL);
	wprintf(L"[ERROR] %ls: %ls\n", msg, buf);

	wstring txt(msg);
	txt += wstring(L":\n") + wstring(buf);
	MessageBoxW(NULL, txt.c_str(), L"OLC Console Game Engine Error", MB_OK | MB_ICONERROR);
	return -1;
}
//-----------------------------------------------------------------------------

int olcConsoleGameEngine::ConstructConsole(int width, int height, int fontw, int fonth)
{
	if(INVALID_HANDLE_VALUE == m_hConsole)
		return Error(L"CreateConsoleScreenBuffer");

	m_nScreenWidth  = width;
	m_nScreenHeight = height;

	//-- This is what I have found so far:
	//   - Windows establishes lower and upper limits to the size of the screen buffer.
	//     The low limit depend on the current font size of the console and the current size of
	//     the console screen.
	//     Trying to set the buffer size below the actual console window size results
	//     in an error.
	//   - The maximum size for the buffer depends on the font size. The maximum buffer size
	//     can be obtained by a call to GetConsoleScreenBufferInfo().
	//
	//     This is how we proceed to manage to get the console buffer and window size:
	//
	//   1. Set Console Window to the lowest possible size hence reducing the
	//      low limit to its bare minimum
	//   2. Set the ScreenBuffer size to the desired console size
	//   3. Set the buffer to the console.
	//   4. Set, now that the buffer has been assigned to the console, the font
	//      size to the desired value. Mind though, the "Consolas" font has a
	//      fixed pitch for the size, the width being half the height.
	//   5. Get the ScreenBuffer info to check the maximum size allowed for the
	//      console window. Adjust desired buffer size to allowed if necessary
	//   6. Set the Window Size to the screen buffer size

	//-- 1. Change console visual size to a minimum so ScreenBuffer can shrink
	//   below the actual visual size
	m_rectWindow = { 0, 0, 1, 1 };
	SetConsoleWindowInfo(m_hConsole, TRUE, &m_rectWindow);

	//-- 2. Set now the size of the screen buffer (there are minimum system sizes that
	//   can be obtained by calling GetSystemMetrics() with params SM_CXMIN y SM_CXMAX
	COORD coord;//({ m_nScreenWidth, m_nScreenHeight });
	coord.X = m_nScreenWidth;
	coord.Y = m_nScreenHeight;
	if(!SetConsoleScreenBufferSize(m_hConsole, coord))
		Error(L"SetConsoleScreenBufferSize");

	//-- 3. Set the buffer to the console
	if(!SetConsoleActiveScreenBuffer(m_hConsole))
		return Error(L"SetConsoleActiveScreenBuffer");
	m_bufScreen = new CHAR_INFO[m_nScreenWidth*m_nScreenHeight];
	memset(m_bufScreen, 0, sizeof(CHAR_INFO) * m_nScreenWidth * m_nScreenHeight);

	//-- 4. Set the font size now that the buffer has been assigned to the
	//      actual console
	CONSOLE_FONT_INFOEX cfi;
	cfi.cbSize       = sizeof(cfi);
	if(!GetCurrentConsoleFontEx(m_hConsole, false, &cfi))
		return Error(L"GetCurrentConsoleFontEx");
	cfi.nFont        = 0;
	cfi.dwFontSize.X = fonth / 2;//fontw;
	cfi.dwFontSize.Y = fonth;
	cfi.FontFamily   = FF_DONTCARE;
	cfi.FontWeight   = FW_NORMAL;
	wcscpy_s(cfi.FaceName, L"Consolas");
	if(!SetCurrentConsoleFontEx(m_hConsole, false, &cfi))
		return Error(L"SetCurrentConsoleFontEx");

	//-- 5. Get Screen Buffer Info and check the maximum allowed window size
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if(!GetConsoleScreenBufferInfo(m_hConsole, &csbi))
		return Error(L"GetConsoleScreenBufferInfo");
	//-- Re-scale the buffer and window if we've exceeded the high-end limit
	if(m_nScreenHeight > csbi.dwMaximumWindowSize.Y
			or m_nScreenWidth > csbi.dwMaximumWindowSize.X)
	{
		m_nScreenHeight = m_nScreenHeight > csbi.dwMaximumWindowSize.Y
						? csbi.dwMaximumWindowSize.Y : m_nScreenHeight;
		m_nScreenWidth = m_nScreenWidth > csbi.dwMaximumWindowSize.X
						? csbi.dwMaximumWindowSize.X : m_nScreenWidth;
		coord.X = m_nScreenWidth;
		coord.Y = m_nScreenHeight;
		if(!SetConsoleScreenBufferSize(m_hConsole, coord))
			Error(L"SetConsoleScreenBufferSize");
	}

	//-- 6. Set Console Window Size
	m_rectWindow = { 0, 0, m_nScreenWidth - 1, m_nScreenHeight - 1 };
	if(!SetConsoleWindowInfo(m_hConsole, TRUE, &m_rectWindow))
		return Error(L"SetConsoleWindowInfo");

	return 1;
}
//-----------------------------------------------------------------------------

void olcConsoleGameEngine::Draw(int x, int y, wchar_t c, short col)
{
	if (x >= 0 and x < m_nScreenWidth and y >= 0 and y < m_nScreenHeight)
	{
		m_bufScreen[y * m_nScreenWidth + x].Char.UnicodeChar = c;
		m_bufScreen[y * m_nScreenWidth + x].Attributes = col;
	}
}
//-----------------------------------------------------------------------------

void olcConsoleGameEngine::Fill(int x1, int y1, int x2, int y2, wchar_t c, short col)
{
	Clip(x1, y1);
	Clip(x2, y2);
	for(int x = x1; x < x2; ++x)
		for(int y = y1; y < y2; ++y)
			Draw(x, y, c, col);
}
//-----------------------------------------------------------------------------

void olcConsoleGameEngine::DrawString(int x, int y, wstring c, short col)
{
	for(size_t i = 0; i < c.size(); ++i)
	{
		m_bufScreen[y * m_nScreenWidth + x + i].Char.UnicodeChar = c[i];
		m_bufScreen[y * m_nScreenWidth + x + i].Attributes       = col;
	}
}
//-----------------------------------------------------------------------------

void olcConsoleGameEngine::DrawStringAlpha(int x, int y, wstring c, short col)
{
	for(size_t i = 0; i < c.size(); ++i)
	{
		if(c[i] != L' ')
		{
			m_bufScreen[y * m_nScreenWidth + x + i].Char.UnicodeChar = c[i];
			m_bufScreen[y * m_nScreenWidth + x + i].Attributes = col;
		}
	}
}
//-----------------------------------------------------------------------------

void olcConsoleGameEngine::Clip(int &x, int &y)
{
	if(x < 0) x = 0;
	else if(x >= m_nScreenWidth) x = m_nScreenWidth;
	if(y < 0) y = 0;
	else if(y >= m_nScreenHeight) y = m_nScreenHeight;
}
//-----------------------------------------------------------------------------

void olcConsoleGameEngine::DrawLine(int x1, int y1, int x2, int y2, wchar_t c, short col)
{
	int x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;
	dx  = x2 - x1;
	dy  = y2 - y1;
	dx1 = abs(dx);
	dy1 = abs(dy);
	px  = 2 * dy1 - dx1;
	py  = 2 * dx1 - dy1;
	if(dy1 <= dx1)
	{
		if(dx >= 0)
		{
			x  = x1;
			y  = y1;
			xe = x2;
		}
		else
		{
			x  = x2;
			y  = y2;
			xe = x1;
		}
		Draw(x, y, c, col);

		for(i = 0; x < xe; ++i)
		{
			x = x + 1;
			if(px < 0)
				px = px + 2 * dy1;
			else
			{
				if((dx < 0 and dy < 0) or (dx > 0 and dy > 0))
					y = y + 1;
				else
					y = y - 1;
				px = px + 2 * (dy1 - dx1);
			}
			Draw(x, y, c, col);
		}
	}
	else
	{
		if(dy >= 0)
		{
			x = x1;
			y = y1;
			ye = y2;
		}
		else
		{
			x = x2;
			y = y2;
			ye = y1;
		}
		Draw(x, y, c, col);

		for(i = 0; y < ye; ++i)
		{
			y = y + 1;
			if(py <= 0)
				py = py + 2 * dx1;
			else
			{
				if((dx < 0 and dy < 0) or (dx > 0 and dy > 0))
					x = x + 1;
				else
					x = x - 1;
				py = py + 2 * (dx1 - dy1);
			}
			Draw(x, y, c, col);
		}
	}
}
//-----------------------------------------------------------------------------

void olcConsoleGameEngine::DrawSprite(int x, int y, olcSprite *sprite)
{
	if (sprite == nullptr)
		return;

	for(int i = 0; i < sprite->nWidth; ++i)
	{
		for(int j = 0; j < sprite->nHeight; ++j)
		{
			if(sprite->GetGlyph(i, j) != L' ')
				Draw(x + i, y + j, sprite->GetGlyph(i, j), sprite->GetColour(i, j));
		}
	}
}
//-----------------------------------------------------------------------------

void olcConsoleGameEngine::DrawPartialSprite(int x, int y, olcSprite *sprite, int ox, int oy, int w, int h)
{
	if(sprite == nullptr)
		return;

	for(int i = 0; i < w; ++i)
	{
		for(int j = 0; j < h; ++j)
		{
			if(sprite->GetGlyph(i+ox, j+oy) != L' ')
				Draw(x + i, y + j, sprite->GetGlyph(i+ox, j+oy), sprite->GetColour(i+ox, j+oy));
		}
	}
}
//-----------------------------------------------------------------------------

void olcConsoleGameEngine::Start()
{
	m_bAtomActive = true;
	// Start the thread
	thread t = thread(&olcConsoleGameEngine::GameThread, this);
	// Wait for thread to be exited
	auto lck = unique_lock<mutex>(m_muxGame);
	m_cvGameFinished.wait(lck);
	// Tidy up
	t.join();
}
//-----------------------------------------------------------------------------

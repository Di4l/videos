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
	if (x < 0 || x > nWidth || y < 0 || y > nHeight)
		return;
	else
		m_Glyphs[y * nWidth + x] = c;
}
//-----------------------------------------------------------------------------

void olcSprite::SetColour(int x, int y, short c)
{
	if (x <0 || x > nWidth || y < 0 || y > nHeight)
		return;
	else
		m_Colours[y * nWidth + x] = c;
}
//-----------------------------------------------------------------------------

wchar_t olcSprite::GetGlyph(int x, int y)
{
	if (x <0 || x > nWidth || y < 0 || y > nHeight)
		return L' ';
	else
		return m_Glyphs[y * nWidth + x];
}
//-----------------------------------------------------------------------------

short olcSprite::GetColour(int x, int y)
{
	if (x <0 || x > nWidth || y < 0 || y > nHeight)
		return FG_BLACK;
	else
		return m_Colours[y * nWidth + x];
}
//-----------------------------------------------------------------------------

bool olcSprite::Save(std::wstring sFile)
{
	std::string aux = WS2S(sFile);
	std::ofstream f;
	f.open(aux.c_str(), std::ios_base::binary);

	f.write((char*)&nWidth, sizeof(int));
	f.write((char*)&nHeight, sizeof(int));
	f.write((char*)m_Colours, sizeof(short) * nWidth * nHeight);
	f.write((char*)m_Glyphs, sizeof(wchar_t) * nWidth * nHeight);

	f.close();
	return true;
}
//-----------------------------------------------------------------------------

bool olcSprite::Load(std::wstring sFile)
{
	delete[] m_Glyphs;
	delete[] m_Colours;
	nWidth = 0;
	nHeight = 0;

	std::string aux = WS2S(sFile);
	std::ifstream f;
	f.open(aux.c_str(), std::ios_base::binary | std::ios_base::trunc);

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
	: m_nScreenWidth(80)
	, m_nScreenHeight(30)
	, m_bufScreen(nullptr)
	, m_sAppName(L"Default")
{
	m_hOriginalConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	m_hConsole         = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
										0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	m_keyNewState      = new short[256];
	m_keyOldState      = new short[256];

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
	std::chrono::system_clock::time_point tp1;
	auto    tp2    = std::chrono::system_clock::now();
	float   fEtime = 0.0;

	// Run as fast as possible
	while(m_bAtomActive)
	{
		// Handle Timing
		tp1    = tp2;
		tp2    = std::chrono::system_clock::now();
		fEtime = std::chrono::duration<float>(tp2 - tp1).count();

		// Handle keyboard input
		handleKeyboardInput();

		// Handle Mouse Input - Check for window events
		handleMouseInput();

		// Handle Frame Update
		if(!OnUserUpdate(fEtime))
			m_bAtomActive = false;

		// Update Title & Present Screen Buffer
		swprintf_s(s, 128, L"OneLoneCoder.com - Console Game Engine - %s - FPS: %3.2f ", m_sAppName.c_str(), 1.0f / fEtime);
		SetConsoleTitleW(s);
		WriteConsoleOutputW(m_hConsole, m_bufScreen, { (short)m_nScreenWidth, (short)m_nScreenHeight }, { 0,0 }, &m_rectWindow);
	}

	m_cvGameFinished.notify_one();
}
//-----------------------------------------------------------------------------

void olcConsoleGameEngine::handleKeyboardInput()
{
	// Handle Keyboard Input
	for (int i = 0; i < 256; i++)
	{
		m_keyNewState[i] = GetAsyncKeyState(i);

		m_keys[i].bPressed = false;
		m_keys[i].bReleased = false;

		if (m_keyNewState[i] != m_keyOldState[i])
		{
			if (m_keyNewState[i] & 0x8000)
			{
				m_keys[i].bPressed = !m_keys[i].bHeld;
				m_keys[i].bHeld = true;
			}
			else
			{
				m_keys[i].bReleased = true;
				m_keys[i].bHeld = false;
			}
		}

		m_keyOldState[i] = m_keyNewState[i];
	}
}
//-----------------------------------------------------------------------------

void olcConsoleGameEngine::handleMouseInput()
{
	//// Handle Mouse Input - Check for window events
	//INPUT_RECORD inBuf[32];
	//DWORD events = 0;
	//GetNumberOfConsoleInputEvents(m_hConsoleIn, &events);
	//if (events > 0)
	//	ReadConsoleInput(m_hConsoleIn, inBuf, events, &events);

	//// Handle events - we only care about mouse clicks and movement
	//// for now
	//for (DWORD i = 0; i < events; i++)
	//{
	//	switch (inBuf[i].EventType)
	//	{
	//	case FOCUS_EVENT:
	//	{
	//		m_bConsoleInFocus = inBuf[i].Event.FocusEvent.bSetFocus;
	//	}
	//	break;

	//	case MOUSE_EVENT:
	//	{
	//		switch (inBuf[i].Event.MouseEvent.dwEventFlags)
	//		{
	//		case MOUSE_MOVED:
	//		{
	//			m_mousePosX = inBuf[i].Event.MouseEvent.dwMousePosition.X;
	//			m_mousePosY = inBuf[i].Event.MouseEvent.dwMousePosition.Y;
	//		}
	//		break;

	//		case 0:
	//		{
	//			for (int m = 0; m < 5; m++)
	//				m_mouseNewState[m] = (inBuf[i].Event.MouseEvent.dwButtonState & (1 << m)) > 0;

	//		}
	//		break;

	//		default:
	//			break;
	//		}
	//	}
	//	break;

	//	default:
	//		break;
	//		// We don't care just at the moment
	//	}
	//}

	//for (int m = 0; m < 5; m++)
	//{
	//	m_mouse[m].bPressed = false;
	//	m_mouse[m].bReleased = false;

	//	if (m_mouseNewState[m] != m_mouseOldState[m])
	//	{
	//		if (m_mouseNewState[m])
	//		{
	//			m_mouse[m].bPressed = true;
	//			m_mouse[m].bHeld = true;
	//		}
	//		else
	//		{
	//			m_mouse[m].bReleased = true;
	//			m_mouse[m].bHeld = false;
	//		}
	//	}

	//	m_mouseOldState[m] = m_mouseNewState[m];
	//}
}
//-----------------------------------------------------------------------------

int olcConsoleGameEngine::Error(wchar_t *msg)
{
	wchar_t buf[256];
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 256, NULL);
	wprintf(L"[ERROR] %ls: %ls\n", msg, buf);

	std::wstring txt(msg);
	txt += std::wstring(L":\n") + std::wstring(buf);
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
	if(m_nScreenHeight > csbi.dwMaximumWindowSize.Y ||
	   m_nScreenWidth  > csbi.dwMaximumWindowSize.X)
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
	m_rectWindow = { 0, 0, SHORT(m_nScreenWidth - 1), SHORT(m_nScreenHeight - 1) };
	if(!SetConsoleWindowInfo(m_hConsole, TRUE, &m_rectWindow))
		return Error(L"SetConsoleWindowInfo");

	return 1;
}
//-----------------------------------------------------------------------------

void olcConsoleGameEngine::Draw(int x, int y, wchar_t c, short col)
{
	if (x >= 0 && x < m_nScreenWidth && y >= 0 && y < m_nScreenHeight)
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

void olcConsoleGameEngine::DrawString(int x, int y, std::wstring c, short col)
{
	for(size_t i = 0; i < c.size(); ++i)
	{
		m_bufScreen[y * m_nScreenWidth + x + i].Char.UnicodeChar = c[i];
		m_bufScreen[y * m_nScreenWidth + x + i].Attributes       = col;
	}
}
//-----------------------------------------------------------------------------

void olcConsoleGameEngine::DrawStringAlpha(int x, int y, std::wstring c, short col)
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
				if((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
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
				if((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
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
	std::thread t = std::thread(&olcConsoleGameEngine::GameThread, this);
	// Wait for thread to be exited
	auto lck = std::unique_lock<std::mutex>(m_muxGame);
	m_cvGameFinished.wait(lck);
	// Tidy up
	t.join();
}
//-----------------------------------------------------------------------------

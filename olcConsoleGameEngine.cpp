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
	if (x <0 || x > nWidth || y < 0 || y > nHeight)
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
	m_nScreenWidth = 80;
	m_nScreenHeight = 30;

	m_hOriginalConsole = 0;
	m_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	m_keyNewState = new short[256];
	m_keyOldState = new short[256];
	memset(m_keyNewState, 0, 256 * sizeof(short));
	memset(m_keyOldState, 0, 256 * sizeof(short));
	memset(m_keys, 0, 256 * sizeof(sKeyState));
	m_bufScreen = nullptr;
	m_sAppName = L"Default";
}
//-----------------------------------------------------------------------------

olcConsoleGameEngine::~olcConsoleGameEngine()
{
	SetConsoleActiveScreenBuffer(m_hOriginalConsole);
	delete[] m_bufScreen;
}
//-----------------------------------------------------------------------------

void olcConsoleGameEngine::GameThread()
{
	// Create user resources as part of this thread
	if (!OnUserCreate())
		return;

	auto tp1 = chrono::system_clock::now();
	auto tp2 = chrono::system_clock::now();

	// Run as fast as possible
	while (m_bAtomActive)
	{
		// Handle Timing
		tp2 = chrono::system_clock::now();
		chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count();

		// Handle Input
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


		// Handle Frame Update
		if (!OnUserUpdate(fElapsedTime))
			m_bAtomActive = false;

		// Update Title & Present Screen Buffer
		wchar_t s[128];
		swprintf(s, 128, L"OneLoneCoder.com - Console Game Engine - %s - FPS: %3.2f ", m_sAppName.c_str(), 1.0f / fElapsedTime);
		SetConsoleTitleW(s);
		WriteConsoleOutput(m_hConsole, m_bufScreen, { (short)m_nScreenWidth, (short)m_nScreenHeight }, { 0,0 }, &m_rectWindow);
	}

	m_cvGameFinished.notify_one();
}
//-----------------------------------------------------------------------------

int olcConsoleGameEngine::Error(wchar_t *msg)
{
	wchar_t buf[256];
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 256, NULL);
	SetConsoleActiveScreenBuffer(m_hOriginalConsole);
	wprintf(L"ERROR: %s\n\t%s\n", msg, buf);
	return -1;
}
//-----------------------------------------------------------------------------

int olcConsoleGameEngine::ConstructConsole(int width, int height, int fontw, int fonth)
{
	m_nScreenWidth  = width;
	m_nScreenHeight = height;

	CONSOLE_FONT_INFOEX cfi;
	cfi.cbSize = sizeof(cfi);
	cfi.nFont = 0;
	cfi.dwFontSize.X = fontw;
	cfi.dwFontSize.Y = fonth;
	cfi.FontFamily = FF_DONTCARE;
	cfi.FontWeight = FW_NORMAL;
	wcscpy_s(cfi.FaceName, L"Consolas");

	if(!SetCurrentConsoleFontEx(m_hConsole, false, &cfi))
		return Error(L"SetCurrentConsoleFontEx");

	COORD coordLargest = GetLargestConsoleWindowSize(m_hConsole);
	if (m_nScreenHeight > coordLargest.Y)
		return Error(L"Game Height Too Big");
	if (m_nScreenWidth > coordLargest.X)
		return Error(L"Game Width Too Big");

	COORD buffer = { (short)m_nScreenWidth, (short)m_nScreenHeight };
	if (!SetConsoleScreenBufferSize(m_hConsole, buffer))
		Error(L"SetConsoleScreenBufferSize");

	m_rectWindow = { 0, 0, (short)m_nScreenWidth - 1, (short)m_nScreenHeight - 1 };
	if (!SetConsoleWindowInfo(m_hConsole, TRUE, &m_rectWindow))
		Error(L"SetConsoleWindowInfo");

	m_bufScreen = new CHAR_INFO[m_nScreenWidth*m_nScreenHeight];

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

void olcConsoleGameEngine::DrawString(int x, int y, wstring c, short col)
{
	for(size_t i = 0; i < c.size(); ++i)
	{
		m_bufScreen[y * m_nScreenWidth + x + i].Char.UnicodeChar = c[i];
		m_bufScreen[y * m_nScreenWidth + x + i].Attributes = col;
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
	dx = x2 - x1;
	dy = y2 - y1;
	dx1 = abs(dx);
	dy1 = abs(dy);
	px = 2 * dy1 - dx1;
	py = 2 * dx1 - dy1;
	if (dy1 <= dx1)
	{
		if (dx >= 0)
		{
			x = x1;
			y = y1;
			xe = x2;
		}
		else
		{
			x = x2;
			y = y2;
			xe = x1;
		}
		Draw(x, y, c, col);
		for(i = 0; x < xe; ++i)
		{
			x = x + 1;
			if (px<0)
				px = px + 2 * dy1;
			else
			{
				if ((dx<0 && dy<0) || (dx>0 && dy>0))
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
		if (dy >= 0)
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
			if (py <= 0)
				py = py + 2 * dx1;
			else
			{
				if ((dx<0 && dy<0) || (dx>0 && dy>0))
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

	for (int i = 0; i < sprite->nWidth; ++i)
	{
		for (int j = 0; j < sprite->nHeight; ++j)
		{
			if (sprite->GetGlyph(i, j) != L' ')
				Draw(x + i, y + j, sprite->GetGlyph(i, j), sprite->GetColour(i, j));
		}
	}
}
//-----------------------------------------------------------------------------

void olcConsoleGameEngine::DrawPartialSprite(int x, int y, olcSprite *sprite, int ox, int oy, int w, int h)
{
	if (sprite == nullptr)
		return;

	for (int i = 0; i < w; ++i)
	{
		for (int j = 0; j < h; ++j)
		{
			if (sprite->GetGlyph(i+ox, j+oy) != L' ')
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

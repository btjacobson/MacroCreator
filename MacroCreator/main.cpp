#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stack>
#include <future>
#include <thread>
#include <io.h>
#include <list>
#include <fcntl.h>
#include <map>
#include <thread>
#include <chrono>

const int MacroMaxLength = 500;
const int LogLength = 1;
const int LogRows = 8;

#define PI 3.14159265359

bool draw = false;
bool drawCircle = false;
bool macroInsertDone = true;
bool executeMacroDone = true;
bool finished = false;
bool macroLoop = true;
bool recordingTime = false;

std::list<std::wstring> LogMessages[LogLength];

void PrintMenu()
{
	std::wcout << L"┌───────────────────────────┐" << std::endl;
	std::wcout << L"│       Macro program       │" << std::endl;
	std::wcout << L"├───────────────────────────┤" << std::endl;
	std::wcout << L"│ LMB - Save point          │" << std::endl;
	std::wcout << L"│ 1   - Playback macro      │" << std::endl;
	std::wcout << L"│ 2   - Save to file        │" << std::endl;
	std::wcout << L"│ 3   - Line drawing        │" << std::endl;
	std::wcout << L"│ 4   - Circle drawing      │" << std::endl;
	std::wcout << L"│ 5   - Load from file      │" << std::endl;
	std::wcout << L"│ 6   - Clear current macro │" << std::endl;
	std::wcout << L"│ 0   - Stop loop           │" << std::endl;
	std::wcout << L"└───────────────────────────┘" << std::endl;
}

struct Timer
{
	double ms = 0.0;
	int seconds = 0;
	int minutes = 0;
	int hours = 0;

	void StartTime()
	{
		ms = 0.0;
		seconds = 0;
		minutes = 0;
		hours = 0;
		bool start = false;

		while (!start)
		{
			if (GetAsyncKeyState(VK_RBUTTON))
			{
				start = true;
			}
		}

		while (true)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

			ms += 2;

			if (ms == 1000)
			{
				seconds++;

				if (seconds == 60)
				{
					minutes++;
					seconds = 0;

					if (minutes == 60)
					{
						hours++;
						minutes = 0;
					}
				}
				ms = 0;
			}
		}
		recordingTime = false;
	}

	double ResetTime()
	{
		double tempMS = (hours * 3600000) + (minutes * 60000) + (seconds * 1000) + ms;

		ms = 0.0;
		seconds = 0;
		minutes = 0;
		hours = 0;

		return tempMS;
	}
};

struct Macro
{
	int SleepTime;
	int SleepTimeBeforeClick;
	int MacroIndex;
	POINT CursorPosition[MacroMaxLength] = { POINT() };
	double CursorTimes[MacroMaxLength] = { double() };
	Timer timer;
};

int Abs(int x)
{
	if (x < 0)
	{
		return -x;
	}
	return x;
}

int Sign(int x)
{
	if (x < 0)
	{
		return -1;
	}
	return 1;
}

void PrintMessage(std::wstring message)
{
	std::wstring fullList = L"";

	if (LogMessages->size() < LogRows)
	{
		LogMessages->push_back(message);
	}
	else
	{
		LogMessages->pop_front();
		LogMessages->push_back(message);
	}

	for (auto tempMessage = LogMessages->begin(); tempMessage != LogMessages->end(); tempMessage++)
	{
		fullList += *tempMessage;
		fullList += '\n';
	}
	system("CLS");
	PrintMenu();
	std::wcout << fullList;
}

void Draw(POINT start, POINT end)
{
	int x1 = start.x;
	int x2 = end.x;
	int y1 = start.y;
	int y2 = end.y;

	int dx = x2 - x1;
	int dy = y2 - y1;

	int x = 0;
	int y = 0;

	if (Abs(dy) > Abs(dx))
	{
		for (y = y1; y != y2; y += Sign(dy))
		{
			x = x1 + (y - y1) * dx / dy;
			SetCursorPos(x, y);
			Sleep(1);
		}
	}
	else
	{
		for (x = x1; x != x2; x += Sign(dx))
		{
			y = y1 + (x - x1) * dy / dx;
			SetCursorPos(x, y);
			Sleep(1);
		}
	}

}

void DrawCircle(long x, long y, float r, Macro* macro)
{
	float theta = 0;
	float h = 0;
	float k = 0;
	float x2 = 0;
	float y2 = 0;
	float angleStep = 0.001;

	for (theta; theta < 2 * PI; theta += angleStep)
	{
		x2 = x + r * cos(theta);
		y2 = y + r * sin(theta);
		theta += angleStep;
		r -= 0.01;

		SetCursorPos(x2, y2);
		Sleep(macro->SleepTime / 1000);
	}
}

void MouseLeftClick(POINT start, POINT end, Macro* macro)
{
	int randRad = (std::rand() % (250 - 5 + 1)) + 5;

	long testX = start.x + randRad * cos(0);
	long testY = start.y + randRad * sin(0);

	if (drawCircle)
	{
		SetCursorPos(testX, testY);
	}

	INPUT input = { 0 };
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	SendInput(1, &input, sizeof(INPUT));

	if (draw)
	{
		Draw(start, end);
	}
	if (drawCircle)
	{
		DrawCircle(start.x, start.y, randRad, macro);
	}

	ZeroMemory(&input, sizeof(INPUT));
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	SendInput(1, &input, sizeof(INPUT));
}

void InsertNewMacroPos(Macro* macro)
{
	if (macro->MacroIndex + 1 < MacroMaxLength)
	{
		macro->MacroIndex++;
		GetCursorPos(&macro->CursorPosition[macro->MacroIndex]);
		macro->CursorTimes[macro->MacroIndex] = macro->timer.ResetTime();

		std::wstring point = L"";
		point += std::to_wstring(macro->CursorPosition[macro->MacroIndex].x);
		point += L",";
		point += std::to_wstring(macro->CursorPosition[macro->MacroIndex].y);

		PrintMessage(L"Point inserted " + point);
		PrintMessage(L"Took " + std::to_wstring(macro->CursorTimes[macro->MacroIndex]) + L" ms");
	}
}

void InsertNewMacroPos(Macro* macro, long x, long y, double t)
{
	if (macro->MacroIndex + 1 < MacroMaxLength)
	{
		macro->MacroIndex++;
		macro->CursorPosition[macro->MacroIndex].x = x;
		macro->CursorPosition[macro->MacroIndex].y = y;
		macro->CursorTimes[macro->MacroIndex] = t;
		PrintMessage(L"New Macro position inserted");
	}
}

void ExecuteMacro(Macro* macro)
{
	for (int i = 0; i <= macro->MacroIndex; ++i)
	{
		if (GetAsyncKeyState('0'))
		{
			macroLoop = false;
			PrintMessage(L"Exiting loop");
			macro->timer.ResetTime();
			break;
		}

		SetCursorPos(macro->CursorPosition[i].x, macro->CursorPosition[i].y);
		Sleep(macro->CursorTimes[i]);
		if (i != macro->MacroIndex)
		{
			MouseLeftClick(macro->CursorPosition[i], macro->CursorPosition[i + 1], macro);
			PrintMessage(L"Waited: " + std::to_wstring(macro->CursorTimes[i]));
		}
		else
		{
			MouseLeftClick(macro->CursorPosition[i], macro->CursorPosition[i], macro);
			PrintMessage(L"Waited: " + std::to_wstring(macro->CursorTimes[i]));
		}
	}
}

Macro CreateNewMacro(int sleepBeforeClick, int sleep)
{
	Macro macro;
	macro.SleepTime = sleep;
	macro.SleepTimeBeforeClick = sleepBeforeClick;
	macro.MacroIndex = -1;
	return macro;
}

void SaveMacro(Macro* macro)
{
	int count = 0;
	std::ofstream outFile;
	outFile.open("macro.txt");

	PrintMessage(L"Saving Macro..");
	for (int i = 0; i < macro->MacroIndex + 1; ++i)
	{
		if (i + 1 != macro->MacroIndex + 1)
		{
			outFile << macro->CursorPosition[i].x << " " << macro->CursorPosition[i].y << " " << macro->CursorTimes[i] << "\n";
		}
		else
		{
			outFile << macro->CursorPosition[i].x << " " << macro->CursorPosition[i].y << " " << macro->CursorTimes[i];
		}
		count = i;
	}
	PrintMessage(L"Saved " + std::to_wstring(count + 1) + L" points");

	outFile.close();
}

void LoadMacro(Macro* macro)
{
	long x;
	long y;
	double t;
	int count = 0;
	std::ifstream inFile;
	inFile.open("macro.txt");
	
	PrintMessage(L"Loading Macro..");
	while (!inFile.eof())
	{
		inFile >> x >> y >> t;
		InsertNewMacroPos(macro, x, y, t);
		count++;
	}
	PrintMessage(L"Loaded " + std::to_wstring(count) + L" points");

	inFile.close();
}

void HandleInput(Macro& m)
{
	// Set new macro position
	if (GetAsyncKeyState(VK_RBUTTON))
	{
		macroInsertDone = false;
		recordingTime = true;
	}
	else if (!GetAsyncKeyState(VK_RBUTTON))
	{
		if (!macroInsertDone)
		{
			InsertNewMacroPos(&m);
		}
		macroInsertDone = true;
	}

	// Execute the macro
	if (GetAsyncKeyState('1'))
	{
		executeMacroDone = false;
		macroLoop = true;
	}
	else if (!GetAsyncKeyState('1'))
	{
		if (!executeMacroDone)
		{
			while (macroLoop)
			{
				ExecuteMacro(&m);
			}
			//ExecuteMacro(&m);
		}
		executeMacroDone = true;
	}

	if (GetAsyncKeyState('2'))
	{
		SaveMacro(&m);
		Sleep(1000);
	}

	if (GetAsyncKeyState('3'))
	{
		draw = !draw;
		PrintMessage(L"Line toggled");
		if (draw)
		{
			PrintMessage(L"On");
		}
		else
		{
			PrintMessage(L"Off");
		}
		Sleep(1000);
	}

	if (GetAsyncKeyState('4'))
	{
		drawCircle = !drawCircle;
		PrintMessage(L"Circle toggled");
		if (drawCircle)
		{
			PrintMessage(L"On");
		}
		else
		{
			PrintMessage(L"Off");
		}
		Sleep(1000);
	}

	if (GetAsyncKeyState('5'))
	{
		LoadMacro(&m);
		Sleep(1000);
	}

	if (GetAsyncKeyState('6'))
	{
		PrintMessage(L"Clearing points...");
		for (auto point : m.CursorPosition)
		{
			point.x = 0;
			point.y = 0;
			m.MacroIndex = -1;
		}
		PrintMessage(L"Finished clearing points");
		Sleep(1000);
	}
}

int main()
{
	_setmode(_fileno(stdout), _O_U16TEXT);
	
	PrintMenu();
	
	srand(time(0));
	
	Macro m = CreateNewMacro(50, 1700);
	
	std::thread timeThread(&Timer::StartTime, &m.timer);
	while (!finished)
	{
		HandleInput(m);
		Sleep(5);
	}

	system("PAUSE");
	return 0;
}
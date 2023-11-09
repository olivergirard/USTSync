#pragma once

#include <iostream>
#include <vector>

#include "shtypes.h"
#include "UTAURead.h"

using namespace std;

struct Note {
	double length = 0;
	double tempo = 0;
	std::wstring lyric;
};

class UTAURead
{
public:
    static std::vector<Note> GetNotes();
    static void AnalyzeNotes(LPWSTR fileName);
	static bool WasFileRead();
};
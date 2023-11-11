#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <cerrno>
#include <cstring>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>

#include "UTAURead.h"
#include "shtypes.h"

static std::vector<Note> notes;
static bool fileRead = false;

std::vector<Note> UTAURead::GetNotes() {
	return notes;
}

bool UTAURead::WasFileRead() {

	return fileRead;
}

void UTAURead::AnalyzeNotes(LPWSTR fileName)
{
	const std::locale locale(".932");
	std::locale::global(std::locale(".932"));

	std::wifstream in;

	in.imbue(locale);
	in.open(fileName);

	std::ifstream file(fileName);

	std::vector<std::wstring> ustData;
	std::wstring data;
	
	/* TODO: Remove this debug console. */
	/*AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);*/

	while (getline(in, data)) {
		ustData.push_back(data);
	}

	float currentTempo = 0;
	struct Note currentNote;

	for (std::wstring line : ustData) {

		line = line.c_str();
		
		if (line.find(L"Length=") != std::string::npos) {
			currentNote.length = std::stod(line.substr(7));
		}
		else if (line.find(L"Lyric=") != std::wstring::npos) {
			currentNote.lyric = line.substr(6);
		}
		else if (line.find(L"Tempo=") != std::string::npos) {
			currentNote.tempo = std::stod(line.substr(6));
		}
		else if (line.find(L"[") != std::string::npos) {

			/* Assuming a note of length 0 means an empty struct and thus no notes have been read yet. */
			
			if (currentNote.length != 0) {
				/* Should not work for lines [#VERSION] and [#SETTING]. */
				notes.push_back(currentNote);
			}
		}
	}

	fileRead = true;
}
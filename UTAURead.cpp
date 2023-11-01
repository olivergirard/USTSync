#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <cerrno>
#include <cstring>

#include "UTAURead.h"
#include "shtypes.h"

struct Note {
	double length = 0;
	double tempo = 0;
	std::string lyric = "";
};

std::vector<Note> notes;

void UTAURead::AnalyzeNotes(LPWSTR fileName)
{
	std::ifstream file(fileName);

	std::vector<std::string> ustData;
	std::string line;

	while (getline(file, line)) {
		ustData.push_back(line);
	}

	float currentTempo = 0;
	struct Note currentNote;

	for (std::string line : ustData) {
		
		if (line.find("Length=") != std::string::npos) {
			currentNote.length = std::stod(line.substr(line.find("Length=") + 8));
		}
		else if (line.find("Lyric=") != std::string::npos) {
			currentNote.lyric = line.substr(line.find("Lyric=") + 7);
		}
		else if (line.find("Tempo=") != std::string::npos) {
			currentNote.tempo = std::stod(line.substr(line.find("Tempo=") + 7));
		}
		else if (line.find("[") != std::string::npos) {

			/* Assuming a note of length 0 means an empty struct and thus no notes have been read yet. */
			
			if (currentNote.length != 0) {
				/* Should not work for lines [#VERSION] and [#SETTING]. */
				notes.push_back(currentNote);
			}
		}
	}
}
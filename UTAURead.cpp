#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <cerrno>
#include <cstring>

struct Note {
	float length = 0;
	float tempo = 0;
	std::string lyric = "";
};

std::vector<Note> notes;

void AnalyzeNotes(std::vector<std::string> ustData)
{
	float currentTempo = 0;
	struct Note currentNote;

	for (std::string line : ustData) {
		
		if (line.find("Length=")) {
			currentNote.length = std::stof(line.substr(line.find("Length=") + 1));
		}
		else if (line.find("Lyric=")) {
			currentNote.lyric = line.substr(line.find("Lyric=") + 1);
		}
		else if (line.find("Tempo=")) {
			currentNote.tempo = std::stof(line.substr(line.find("Tempo=") + 1));
		}
		else if (line.find("[")) {

			/* Assuming a note of length 0 means an empty struct and thus no notes have been read yet. */
			
			if (currentNote.length != 0) {
				/* Should not work for lines [#VERSION] and [#SETTING]. */
				notes.push_back(currentNote);
			}
		}
	}
}

int main() {

	
	//TODO prompt the user for a file...
	std::string fileName = "C:/Users/azure/source/repos/MidiMoment/usts/ust.ust";
	std::ifstream file(fileName);

	std::vector<std::string> ustData;
	std::string line;

	while (getline(file, line)) {
		ustData.push_back(line);
	}

	AnalyzeNotes(ustData);
}
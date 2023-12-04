#include <fstream>
#include <windows.h>
#include <commdlg.h>
#include <iostream>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windowsx.h>
#include <WinUser.h>
#include <tuple>
#include <filesystem>
#include <time.h>
#include <math.h>

#include "vector.h"
#include "resource.h"
#include "utf8-utils.h"
#include "UTAURead.h"

/* For graphics. */
#include "SDL/SDL.h"
#include "SDL/SDL_ttf.h"

/* For music playing. */
#include <Mmsystem.h>
#include <mciapi.h>
#pragma comment(lib, "Winmm.lib")

#define MAX_LOADSTRING 100
#define SDL_MAIN_HANDLED

/* My defines. */
#define minCorners(a,b) (((a)<(b))?(a):(b))
#define maxCorners(a,b) (((a)>(b))?(a):(b))
#define baseFontSize 60

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

/* My global variables. */
int windowWidth = 870;
int windowHeight = 550;
int windowX = 0;
int windowY = 0;

int scrollX = 0;
int scrollWidth = 0;

bool initialRead = false;
bool isRangeSelected = false;
bool addFont = false;
char* userInput;

RECT selectedRange = { -1, -1, -1, -1 };
RECT previousRange = { -1, -1, -1, -1 };

COLORREF colorToChange = NULL;
std::string mp3File;
clock_t startingTime;
std::filesystem::path fileLocation;
std::string fontPath = "fonts/Gen Jyuu Gothic Monospace Bold.ttf";

std::vector<std::tuple<RECT, std::wstring, double, COLORREF, double, double>> noteRectangles;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ErrorBox(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    FontBox(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_USTSYNC, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_USTSYNC));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

/* My functions. */

bool AllNotesEffected() {

	/* TODO */
	bool allNotes = true;

	for (std::tuple<RECT, std::wstring, double, COLORREF, double, double> tuple : noteRectangles) {
		if (std::get<3>(tuple) == RGB(255, 255, 255)) {
			allNotes = false;
			break;
		}
	}

	return allNotes;
}

bool RangeChecker(RECT rectangle) {

	if ((rectangle.left == -1) && (rectangle.right == -1) && (rectangle.top == -1) && (rectangle.bottom == -1)) {
		return false;
	}

	return true;
}

void DrawNotes(HWND hWnd, HDC hdc) {

	std::vector<Note> notes = UTAURead::GetNotes();

	RECT rectangle = { 0,0,0,0 };

	rectangle.left = 10;
	rectangle.right = 20;
	rectangle.top = 220;
	rectangle.bottom = 250;

	SetDCPenColor(hdc, RGB(0, 0, 0));

	int temp = 0;

	if (initialRead == false) {
		for (Note note : notes) {

			temp += (note.length / 7) + 1;
			rectangle.right += note.length / 7;

			Rectangle(hdc, rectangle.left, rectangle.top, rectangle.right, rectangle.bottom);
			DrawText(hdc, LPCWSTR(note.lyric.c_str()), -1, &rectangle, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

			std::tuple<RECT, std::wstring, double, COLORREF, double, double> data = std::make_tuple(rectangle, note.lyric, note.length, RGB(255, 255, 255), note.tempo, baseFontSize);
			noteRectangles.push_back(data);

			rectangle.left = rectangle.right + 1;
		}

		rectangle.right = rectangle.left + 20;
		HBRUSH hbrush = CreateSolidBrush(RGB(255, 255, 255));
		FillRect(hdc, &rectangle, hbrush);
		DeleteBrush(hbrush);
		temp += 20;

		scrollWidth = temp;
		initialRead = true;

		fontPath = filesystem::absolute(fontPath).generic_string();
	}
	else {

		for (std::tuple<RECT, std::wstring, double, COLORREF, double, double> tuple : noteRectangles) {

			RECT noteRectangle = std::get<0>(tuple);

			HBRUSH hbrush = CreateSolidBrush(std::get<3>(tuple));
			FillRect(hdc, &noteRectangle, hbrush);
			DeleteBrush(hbrush);

			SetDCPenColor(hdc, RGB(0, 0, 0));
			SelectObject(hdc, GetStockObject(NULL_BRUSH));
			Rectangle(hdc, noteRectangle.left, noteRectangle.top, noteRectangle.right, noteRectangle.bottom);

			SetBkMode(hdc, TRANSPARENT);
			DrawText(hdc, LPCWSTR(std::get<1>(tuple).c_str()), -1, &noteRectangle, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

		}
	}
}

bool Overlap(RECT noteRectangle, RECT selectedRange) {

	int left = selectedRange.left;
	int right = selectedRange.right;
	int top = selectedRange.top;
	int bottom = selectedRange.bottom;

	if (selectedRange.top > selectedRange.bottom && selectedRange.left > selectedRange.right) {
		/* If dragging from the lower-right corner... */
		left = selectedRange.right;
		right = selectedRange.left;
		bottom = selectedRange.top;
		top = selectedRange.bottom;

	}
	else if (selectedRange.right < selectedRange.left) {
		/* If dragging from the upper-right corner... */
		left = selectedRange.right;
		right = selectedRange.left;
	}
	else if (selectedRange.bottom < selectedRange.top) {
		/* If dragging from the lower-left corner... */
		bottom = selectedRange.top;
		top = selectedRange.bottom;
	}

	/* Otherwise, dragging from the upper-left corner. */
	if ((minCorners(noteRectangle.right, right) > maxCorners(noteRectangle.left, left)) && (minCorners(noteRectangle.bottom, bottom) > maxCorners(noteRectangle.top, top)) == true) {
		return true;
	}
	else {
		return false;
	}
}

void SelectRange(HWND hWnd, HDC hdc) {

	for (std::tuple<RECT, std::wstring, double, COLORREF, double, double> tuple : noteRectangles) {

		RECT noteRectangle = std::get<0>(tuple);

		if (Overlap(noteRectangle, selectedRange) == true) {

			HBRUSH hbrush = CreateSolidBrush(RGB(100, 234, 255));
			FillRect(hdc, &noteRectangle, hbrush);
			DeleteBrush(hbrush);

			SetDCPenColor(hdc, RGB(0, 0, 0));
			SelectObject(hdc, GetStockObject(NULL_BRUSH));
			Rectangle(hdc, noteRectangle.left, noteRectangle.top, noteRectangle.right, noteRectangle.bottom);

			SetBkMode(hdc, TRANSPARENT);
			DrawText(hdc, LPCWSTR(std::get<1>(tuple).c_str()), -1, &noteRectangle, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
		}
	}
}

/* Appear - RGB(0, 255, 0).
	Group - RGB(255, 0, 0).
*/

void AddEffect(HWND hWnd, COLORREF colorToChange) {

	/* Adding an effect to the range of notes. */
	int index = 0;

	for (std::tuple<RECT, std::wstring, double, COLORREF, double, double> tuple : noteRectangles) {

		RECT noteRectangle = std::get<0>(tuple);

		if (Overlap(noteRectangle, selectedRange) == true) {

			std::tuple<RECT, std::wstring, double, COLORREF, double, double> updatedTuple = { std::get<0>(tuple) , std::get<1>(tuple), std::get<2>(tuple), colorToChange, std::get<4>(tuple), std::get<5>(tuple)};
			noteRectangles[index] = updatedTuple;
		}

		index++;
	}
}

void AddGrouping(HWND hWnd) {

	bool firstNote = false;
	bool timeToPush = false;

	int left = 0;
	int top = 0;
	int right = 0;
	int bottom = 0;

	double newLength = 0;
	double newTempo = 0;
	std::wstring newLyric;

	std::vector<std::tuple<RECT, std::wstring, double, COLORREF, double, double>> newNoteRectangles;
	std::tuple<RECT, std::wstring, double, COLORREF, double, double> newNote = {};

	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

	for (std::tuple<RECT, std::wstring, double, COLORREF, double, double> tuple : noteRectangles) {

		RECT noteRectangle = std::get<0>(tuple);

		if (Overlap(noteRectangle, selectedRange) == true) {

			timeToPush = true;

			if (firstNote == false) {
				newNoteRectangles.push_back(newNote);
				newTempo = std::get<4>(tuple);
				left = noteRectangle.left;
				top = noteRectangle.top;
				firstNote = true;
				
			}

			newLength += std::get<2>(tuple);
			right = noteRectangle.right;
			bottom = noteRectangle.bottom;
			newLyric += std::get<1>(tuple);
		}
		else {

			if (timeToPush == true) {
				RECT newRectangle = { left, top, right, bottom };
				newNote = std::make_tuple(newRectangle, newLyric, newLength, RGB(255, 255, 255), newTempo, (double)baseFontSize);
				newNoteRectangles.push_back(newNote);
				timeToPush = false;
			}
			
			newNoteRectangles.push_back(tuple);
		}
	}

	noteRectangles.clear();
	noteRectangles = newNoteRectangles;

	for (std::tuple<RECT, std::wstring, double, COLORREF, double, double> tuple : noteRectangles) {

		cout << std::get<2>(tuple) << endl;
	}

}

void AddFontSize(HWND hWnd, int newFontSize) {

	/* Adding an effect to the range of notes. */
	int index = 0;

	for (std::tuple<RECT, std::wstring, double, COLORREF, double, double> tuple : noteRectangles) {

		RECT noteRectangle = std::get<0>(tuple);

		if (Overlap(noteRectangle, selectedRange) == true) {

			std::tuple<RECT, std::wstring, double, COLORREF, double, double> updatedTuple = { std::get<0>(tuple) , std::get<1>(tuple), std::get<2>(tuple), std::get<3>(tuple), std::get<4>(tuple), newFontSize};
			noteRectangles[index] = updatedTuple;
		}

		index++;
	}
}

std::tuple<RECT, std::wstring, double, COLORREF, double, double> GetNoteToDisplay() {

	double baseBPM = 125;

	clock_t currentTime = clock();
	double duration = double(currentTime - startingTime) / double(CLOCKS_PER_SEC);
	double noteSum = 0;
	double tempoFactor;

	for (std::tuple<RECT, std::wstring, double, COLORREF, double, double> note : noteRectangles) {

		if (std::get<2>(note) != 0) {
			tempoFactor = baseBPM / std::get<4>(note);
			noteSum += std::get<2>(note) * tempoFactor;

			if (noteSum >= duration * 1000) {

				std::wcout << std::get<1>(note) << endl;
				return note;

			}
		}
		
	}

	RECT emptyRectangle = { 0 };
	std::tuple<RECT, std::wstring, double, COLORREF, double, double> emptyNote = std::make_tuple(emptyRectangle, L" ", 0, RGB(0, 0, 0), 0, baseFontSize);

	return emptyNote;
}

SDL_Surface* UpdateSurface() {

	std::tuple<RECT, std::wstring, double, COLORREF, double, double> note = GetNoteToDisplay();

	TTF_Font* font = TTF_OpenFont(fontPath.c_str(), std::get<5>(note));

	const auto fontDirectionSuccess = TTF_SetFontDirection(font, TTF_DIRECTION_LTR);
	const auto fontScriptNameSuccess = TTF_SetFontScriptName(font, "Hani");

	SDL_Color textColor = { 0x00, 0x00, 0x00, 0xFF };
	SDL_Color textBackgroundColor = { 0xFF, 0xFF, 0xFF, 0xFF };

	std::wstring lyric = std::get<1>(note);

	if (lyric == L"R") {
		lyric = L" ";
	}

	const wchar_t* wstr = lyric.c_str();

	int wstr_len = (int)wcslen(wstr);
	int num_chars = WideCharToMultiByte(CP_UTF8, 0, wstr, wstr_len, NULL, 0, NULL, NULL);
	CHAR* strTo = (CHAR*)malloc((num_chars + 1) * sizeof(CHAR));

	if (strTo)
	{
		WideCharToMultiByte(CP_UTF8, 0, wstr, wstr_len, strTo, num_chars, NULL, NULL);
		strTo[num_chars] = '\0';
	}

	SDL_Surface* textSurface = TTF_RenderUTF8_Blended(font, strTo, textColor);
	if (!textSurface) {
		printf("Unable to render text surface!\n"
			"SDL2_ttf Error: %s\n", TTF_GetError());
	}

	return textSurface;
}

void RenderVideo() {

	SDL_Init(SDL_INIT_VIDEO);
	TTF_Init();

	SDL_Window* window = SDL_CreateWindow("USTSync: Video", windowX, windowY, windowWidth, windowHeight, SDL_WINDOW_SHOWN);

	if (!window) {
		printf("Window could not be created!\n"
			"SDL_Error: %s\n", SDL_GetError());
	}
	else {
		SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

		if (!renderer) {
			printf("Renderer could not be created!\n"
				"SDL_Error: %s\n", SDL_GetError());
		}
		else {
			SDL_Rect squareRect;
			squareRect.w = min(windowWidth, windowHeight) / 2;
			squareRect.h = min(windowWidth, windowHeight) / 2;
			squareRect.x = windowWidth / 2 - squareRect.w / 2;
			squareRect.y = windowHeight / 2 - squareRect.h / 2;

			SDL_Surface* textSurface = UpdateSurface();

			if (!textSurface) {
				printf("Unable to render text surface!\n"
					"SDL2_ttf Error: %s\n", TTF_GetError());
			}
			else {

				SDL_Texture* text = NULL;
				SDL_Rect textRect;
				
				text = SDL_CreateTextureFromSurface(renderer, textSurface);
				if (!text) {
					printf("Unable to create texture from rendered text!\n"
						"SDL2 Error: %s\n", SDL_GetError());
					return;
				}

				textRect.w = textSurface->w;
				textRect.h = textSurface->h;

				SDL_FreeSurface(textSurface);

				textRect.x = (windowWidth - textRect.w) / 2;
				textRect.y = (windowHeight - textRect.h) / 2;

				bool quit = false;
				while (true)
				{
					SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
					SDL_RenderClear(renderer);

					textSurface = UpdateSurface();
					text = SDL_CreateTextureFromSurface(renderer, textSurface);

					textRect.w = textSurface->w;
					textRect.h = textSurface->h;

					SDL_FreeSurface(textSurface);

					textRect.x = (windowWidth - textRect.w) / 2;
					textRect.y = (windowHeight - textRect.h) / 2;

					SDL_RenderCopy(renderer, text, NULL, &textRect);
					SDL_RenderPresent(renderer);
				}

				SDL_DestroyRenderer(renderer);
			}
		}

		SDL_DestroyWindow(window);
	}

	TTF_Quit();
	SDL_Quit();
}

/* End of my functions. */

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_USTSYNC));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_USTSYNC);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{

	hInst = hInstance; // Store instance handle in our global variable
	fileLocation = std::filesystem::current_path();

	RECT desktop;
	const HWND hDesktop = GetDesktopWindow();
	GetWindowRect(hDesktop, &desktop);
	windowX = desktop.right / 4;
	windowY = desktop.bottom / 4;

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | SB_HORZ,
		windowX, windowY, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_ERASEBKGND:
	{
		return 1;
	}
	case WM_HSCROLL: {

		switch (LOWORD(wParam))
		{
		case SB_PAGELEFT:
			scrollX -= 10;
			break;
		case SB_PAGERIGHT:
			scrollX += 10;
			break;
		case SB_LINELEFT:
			scrollX -= 5;
			break;
		case SB_LINERIGHT:
			scrollX += 5;
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			scrollX = HIWORD(wParam);
			break;
		}

		InvalidateRect(hWnd, NULL, false);
		UpdateWindow(hWnd);
		break;
	}
	case WM_LBUTTONDOWN:
	{
		if (UTAURead::WasFileRead() == true) {

			selectedRange = { -1, -1, -1, -1 };
			selectedRange.left = GET_X_LPARAM(lParam) + scrollX;
			selectedRange.top = GET_Y_LPARAM(lParam);

			isRangeSelected = true;
		}

		break;
	}
	case WM_LBUTTONUP:
	{
		isRangeSelected = false;

		InvalidateRect(hWnd, NULL, false);
		UpdateWindow(hWnd);
		break;
	}
	case WM_MOUSEMOVE:
	{
		if (isRangeSelected == true) {
			selectedRange.right = GET_X_LPARAM(lParam) + scrollX;
			selectedRange.bottom = GET_Y_LPARAM(lParam);

			if (previousRange.right > selectedRange.right && previousRange.bottom > selectedRange.bottom) {
				InvalidateRect(hWnd, &previousRange, false);
				UpdateWindow(hWnd);
			}
		}

		break;
	}
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case ID_NOPERSIST_CENTERED: {

			colorToChange = RGB(0, 255, 0);
			AddEffect(hWnd, colorToChange);
			break;
		}
		case ID_EFFECTS_GROUP: {
			AddGrouping(hWnd);
			break;
		}
		case ID_EFFECTS_FONTSIZE: {

			DialogBox(hInst, MAKEINTRESOURCE(IDD_FONTSIZE), hWnd, FontBox);
			while (addFont == false) {
				/* Wait for the user to close the dialog box. */
			}
			AddFontSize(hWnd, stoi(userInput));
			addFont = false;
			break;
		}
		case IDM_UST:
		{
			WCHAR buffer[MAX_PATH];
			OPENFILENAME ofn = {};
			ofn.lStructSize = sizeof(ofn);
			ofn.lpstrFilter = TEXT("*.UST\0");
			ofn.lpstrFile = buffer, ofn.nMaxFile = MAX_PATH, * buffer = '\0';
			ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			if (GetOpenFileName(&ofn)) {
				UTAURead::AnalyzeNotes(ofn.lpstrFile);
			}

			std::filesystem::current_path(fileLocation);

			break;
		}
		case IDM_MP3:
		{
			WCHAR buffer[MAX_PATH];
			OPENFILENAME ofn = {};
			ofn.lStructSize = sizeof(ofn);
			ofn.lpstrFilter = TEXT("*.WAV\0");
			ofn.lpstrFile = buffer, ofn.nMaxFile = MAX_PATH, * buffer = '\0';
			ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			if (GetOpenFileName(&ofn)) {
				filesystem::path myFile = ofn.lpstrFile;
				std::string pathString{ myFile.string() };
				mp3File = pathString;
			}

			std::filesystem::current_path(fileLocation);

			break;
		}
		case IDM_RENDER:
			startingTime = clock();
			PlaySoundA(mp3File.c_str(), NULL, SND_FILENAME | SND_ASYNC);

			RenderVideo();

			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}

		InvalidateRect(hWnd, NULL, true);
		UpdateWindow(hWnd);
	}
	break;
	case WM_PAINT:
	{
		/* Variable declaration. */
		HDC hdc;
		PAINTSTRUCT ps;
		HDC hdcMem;
		HBITMAP hbmMem;
		HANDLE hOld;

		/* Scroll bar. */
		SCROLLINFO si = { 0 };
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_ALL;
		si.nMin = 0;
		si.nMax = scrollWidth;
		si.nPage = windowWidth;
		si.nPos = scrollX;
		si.nTrackPos = 0;
		SetScrollInfo(hWnd, SB_HORZ, &si, true);

		/* Double-buffering. */
		hdc = BeginPaint(hWnd, &ps);
		hdcMem = CreateCompatibleDC(hdc);

		RECT windowRectangle;
		GetWindowRect(hWnd, &windowRectangle);
		int height = windowRectangle.bottom - windowRectangle.top;

		hbmMem = CreateCompatibleBitmap(hdc, scrollWidth, height);
		hOld = SelectObject(hdcMem, hbmMem);

		/* Solid, white background. */
		windowRectangle.left = 0;
		windowRectangle.top = 0;
		windowRectangle.right = scrollWidth;
		windowRectangle.bottom = windowHeight;
		HBRUSH backgroundBrush = CreateSolidBrush(RGB(255, 255, 255));
		FillRect(hdcMem, &windowRectangle, backgroundBrush);
		DeleteObject(backgroundBrush);

		/* Actual paint conditions. */
		if (UTAURead::WasFileRead() == true) {
			DrawNotes(hWnd, hdcMem);
			initialRead = true;
		}

		if ((isRangeSelected == false) && (RangeChecker(selectedRange) == true)) {
			SelectRange(hWnd, hdcMem);
		}

		/* Cleaning up double-buffering. */
		BitBlt(hdc, 0, 0, scrollWidth, windowHeight, hdcMem, scrollX, 0, SRCCOPY);
		SelectObject(hdcMem, hOld);
		DeleteObject(hbmMem);
		DeleteObject(hOld);
		DeleteDC(hdcMem);
		DeleteDC(hdc);

		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK ErrorBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK FontBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			HWND input = GetDlgItem(hDlg, IDC_EDIT1);
			DWORD dwInputLength = Edit_GetTextLength(input);

			userInput = new char[dwInputLength + 1];
			GetWindowTextA(input, userInput, dwInputLength + 1);

			EndDialog(hDlg, LOWORD(wParam));
			
			addFont = true;
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

#include <fstream>
#include <windows.h>
#include <commdlg.h>
#include <iostream>
#include <string>
#include <sstream>
#include <windowsx.h>
#include <WinUser.h>
#include <tuple>
#include <filesystem>

#include "framework.h"
#include "resource.h"
#include "shtypes.h"
#include "shlobj_core.h"
#include "UTAURead.h"

/* For music playing. */
#include <Mmsystem.h>
#include <mciapi.h>
#pragma comment(lib, "Winmm.lib")

/* For graphics displaying. */
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

#define MAX_LOADSTRING 100

/* My defines. */
#define minCorners(a,b) (((a)<(b))?(a):(b))
#define maxCorners(a,b) (((a)>(b))?(a):(b))

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

/* My global variables. */

/* Used for range selection. */
int rangeLeft = 0;
int rangeTop = 0;
int rangeRight = 0;
int rangeBottom = 0;

int scrollX = 0;
int scrollWidth = 0;

/* Used for left-side mouse clicking. */
int mouseX = 0;
int mouseY = 0;

int windowWidth = 1000;
int windowHeight = 550;
int windowX = 0;
int windowY = 0;

bool isRangeSelected = false;
bool initialRead = false;

RECT selectedRange = { 0, 0, 0, 0 };
RECT previousRange = { 0, 0, 0, 0 };

COLORREF colorToChange = NULL;
std::string mp3File;

std::vector<std::tuple<RECT, std::wstring, double, COLORREF>> noteRectangles;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    VideoProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ErrorBox(HWND, UINT, WPARAM, LPARAM);

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

	bool allNotes = true;

	for (std::tuple<RECT, std::wstring, double, COLORREF> tuple : noteRectangles) {
		if (std::get<3>(tuple) == RGB(255, 255, 255)) {
			allNotes = false;
			break;
		}
	}

	return allNotes;
}

bool RangeChecker(RECT rectangle) {

	if ((rectangle.left == 0) && (rectangle.right == 0) && (rectangle.top == 0) && (rectangle.bottom == 0)) {
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

			std::tuple<RECT, std::wstring, double, COLORREF> data = std::make_tuple(rectangle, note.lyric, note.length, RGB(255, 255, 255));
			noteRectangles.push_back(data);

			rectangle.left = rectangle.right + 1;
		}

		scrollWidth = temp;
	}
	else {

		for (std::tuple<RECT, std::wstring, double, COLORREF> tuple : noteRectangles) {

			RECT noteRectangle = std::get<0>(tuple);

			HBRUSH hbrush = CreateSolidBrush(std::get<3>(tuple));
			FillRect(hdc, &noteRectangle, hbrush);

			SetDCPenColor(hdc, RGB(0, 0, 0));
			SelectObject(hdc, GetStockObject(NULL_BRUSH));
			Rectangle(hdc, noteRectangle.left, noteRectangle.top, noteRectangle.right, noteRectangle.bottom);

			SetBkMode(hdc, TRANSPARENT);
			DrawText(hdc, LPCWSTR(std::get<1>(tuple).c_str()), -1, &noteRectangle, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

		}
	}
}

void DrawRange(HWND hWnd, HDC hdc) {

	SetDCPenColor(hdc, RGB(0, 0, 0));
	SetBkMode(hdc, OPAQUE);

	Rectangle(hdc, selectedRange.left, selectedRange.top, selectedRange.right, selectedRange.bottom);
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

	} else if (selectedRange.right < selectedRange.left) {
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

	for (std::tuple<RECT, std::wstring, double, COLORREF> tuple : noteRectangles) {

		RECT noteRectangle = std::get<0>(tuple);

		if (Overlap(noteRectangle, selectedRange) == true) {

			HBRUSH hbrush = CreateSolidBrush(RGB(100, 234, 255));
			FillRect(hdc, &noteRectangle, hbrush);

			SetDCPenColor(hdc, RGB(0, 0, 0));
			SelectObject(hdc, GetStockObject(NULL_BRUSH));
			Rectangle(hdc, noteRectangle.left, noteRectangle.top, noteRectangle.right, noteRectangle.bottom);

			SetBkMode(hdc, TRANSPARENT);
			DrawText(hdc, LPCWSTR(std::get<1>(tuple).c_str()), -1, &noteRectangle, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

		}
	}
}

bool MouseInRectangle(RECT menuRectangle) {

	if ((mouseX > menuRectangle.left) && (mouseX < menuRectangle.right) && (mouseY > menuRectangle.top) && (mouseY < menuRectangle.bottom)) {
		return true;
	}
	else {
		return false;
	}
}

void AddEffect(HWND hWnd, COLORREF colorToChange) {

	/* Adding an effect to the range of notes. */
	int index = 0;

	for (std::tuple<RECT, std::wstring, double, COLORREF> tuple : noteRectangles) {

		RECT noteRectangle = std::get<0>(tuple);

		if (Overlap(noteRectangle, selectedRange) == true) {

			std::tuple<RECT, std::wstring, double, COLORREF> updatedTuple = { std::get<0>(tuple) , std::get<1>(tuple), std::get<2>(tuple), colorToChange};
			noteRectangles[index] = updatedTuple;
		}

		index++;
	}
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
	case WM_RBUTTONDOWN: {

		if (isRangeSelected == false) {
			mouseX = GET_X_LPARAM(lParam);
			mouseY = GET_Y_LPARAM(lParam);
		}

		InvalidateRect(hWnd, NULL, false);

		break;
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
			scrollX = HIWORD(wParam);
			break;
		}

		InvalidateRect(hWnd, NULL, false);
		break;
	}
	case WM_LBUTTONDOWN:
	{

		if (UTAURead::WasFileRead() == true) {

			selectedRange = { 0 };
			rangeLeft = GET_X_LPARAM(lParam);
			rangeTop = GET_Y_LPARAM(lParam);
			selectedRange.left = rangeLeft;
			selectedRange.top = rangeTop;

			isRangeSelected = true;
		}

		break;
	}
	case WM_LBUTTONUP:
	{
		isRangeSelected = false;

		InvalidateRect(hWnd, NULL, false);
		break;
	}
	case WM_MOUSEMOVE:
	{
		if (isRangeSelected == true) {
			rangeRight = GET_X_LPARAM(lParam);
			rangeBottom = GET_Y_LPARAM(lParam);
			selectedRange.right = rangeRight;
			selectedRange.bottom = rangeBottom;

			InvalidateRect(hWnd, NULL, false);

			if (previousRange.right > selectedRange.right && previousRange.bottom > selectedRange.bottom) {
				InvalidateRect(hWnd, &previousRange, true);
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
				std::string pathString{ myFile.string()};
				mp3File = pathString;
			}

			break;
		}
		case IDM_RENDER:

			if (AllNotesEffected() == true) {

				WNDCLASS cldclass;
				cldclass.style = CS_HREDRAW | CS_VREDRAW;
				cldclass.cbClsExtra = 0;
				cldclass.cbWndExtra = 0;
				cldclass.lpfnWndProc = VideoProc;
				cldclass.hInstance = hInst;
				cldclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
				cldclass.hCursor = LoadCursor(NULL, IDC_ARROW);
				cldclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
				cldclass.lpszMenuName = NULL;
				cldclass.lpszClassName = L"VideoWindow";

				RegisterClass(&cldclass);

				HMENU hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDC_VIDEOWINDOW));
				HWND videoWindow = CreateWindow(L"VideoWindow", NULL, WS_VISIBLE | WS_OVERLAPPEDWINDOW, windowX, windowY, windowWidth, windowHeight, hWnd, hMenu, hInst, NULL);
				ShowWindow(videoWindow, 5);
				UpdateWindow(videoWindow);

			}
			else {
				DialogBox(hInst, MAKEINTRESOURCE(IDD_RENDERFAIL), hWnd, ErrorBox);
			}
			
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}

		InvalidateRect(hWnd, NULL, true);
		RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
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
		RECT clientRectangle = { 0 };
		GetClientRect(hWnd, &clientRectangle);
		SCROLLINFO si = { 0 };
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_ALL;
		si.nMin = 0;
		si.nMax = scrollWidth;
		si.nPage = (clientRectangle.right - clientRectangle.left);
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
		HBRUSH backgroundBrush = CreateSolidBrush(RGB(255, 255,255));
		FillRect(hdcMem, &windowRectangle, backgroundBrush);
		DeleteObject(backgroundBrush);
		
		/* Actual paint conditions. */
		if (UTAURead::WasFileRead() == true) {
			DrawNotes(hWnd, hdcMem);
			initialRead = true;
		}
		
		if (((rangeRight != 0) || (rangeBottom != 0)) && (isRangeSelected == true)) {
			previousRange = selectedRange;
			DrawRange(hWnd, hdcMem);
		}

		if ((isRangeSelected == false) && (RangeChecker(selectedRange) == true)) {
			previousRange = { 0, 0, 0, 0 };

			rangeLeft = 0;
			rangeTop = 0;
			rangeRight = 0;
			rangeBottom = 0;

			SelectRange(hWnd, hdcMem);
		}

		/* Cleaning up double-buffering. */
		BitBlt(hdc, 0, 0, scrollWidth, height, hdcMem, scrollX, 0, SRCCOPY);
		SelectObject(hdcMem, hOld);
		DeleteObject(hbmMem);
		DeleteDC(hdcMem);

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

LRESULT CALLBACK VideoProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message)
	{
	case WM_CREATE: {

		PlaySoundA(mp3File.c_str(), NULL, SND_FILENAME | SND_ASYNC);
		break;
	}
	case WM_PAINT: {

		/* Variable declaration. */
		HDC hdc;
		PAINTSTRUCT ps;
		HDC hdcMem;
		HBITMAP hbmMem;
		HANDLE hOld;

		/* Double-buffering. */
		hdc = BeginPaint(hWnd, &ps);
		hdcMem = CreateCompatibleDC(hdc);

		RECT windowRectangle;
		GetWindowRect(hWnd, &windowRectangle);

		hbmMem = CreateCompatibleBitmap(hdc, windowWidth, windowHeight);
		hOld = SelectObject(hdcMem, hbmMem);

		/* Solid, white background. */
		windowRectangle.left = 0;
		windowRectangle.top = 0;
		windowRectangle.right = windowWidth;
		windowRectangle.bottom = windowHeight;
		HBRUSH backgroundBrush = CreateSolidBrush(RGB(0, 0, 0));
		FillRect(hdcMem, &windowRectangle, backgroundBrush);
		DeleteObject(backgroundBrush);

		/* Cleaning up double-buffering. */
		BitBlt(hdc, 0, 0, windowWidth, windowHeight, hdcMem, scrollX, 0, SRCCOPY);
		SelectObject(hdcMem, hOld);
		DeleteObject(hbmMem);
		DeleteDC(hdcMem);

		EndPaint(hWnd, &ps);
		break;
	}
	case WM_DESTROY:
		PlaySound(NULL, 0, 0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

/* Functions for the actual text display. */

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

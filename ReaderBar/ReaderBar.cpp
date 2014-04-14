#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

#include "ReaderBar.h"
#include <Windows.h>
#include <strsafe.h>
#include <gdiplus.h>
#include "Timer.h"
#include "resource.h"

using namespace Gdiplus;

// Global Variables
HINSTANCE           Dll;
GdiplusStartupInput GdiStartup;
ULONG_PTR           GdiToken;
BLENDFUNCTION       GdiBlend;
int                 GdiAlpha;
int                 GdiBackgroundAlpha;
int                 FontSize;
HWND                BarHwnd;
HWND                DlgHwnd;
HWND                MessageHwnd;
Point               BarSize;
Point               BarPosition;
StringFormat        TextFormat;
wchar_t             ConfigPath[256];
wchar_t             TextBuffer[2048];
wchar_t             FontType[50];
int                 BackGroundRGB[3];
int                 TextRGB[3];
Timer               TouchTime;
HANDLE              WinProc;
HANDLE              TopProc;
Point               TouchOffset;
RECT                DownPosition;
int                 Movement;
int                 CharPerLine;

// Function Declaration
LRESULT CALLBACK BarProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void             OnPaint();
void             InputProc(UINT message, WPARAM wParam, LPARAM lParam);
void             OpenConfigWin();
void             SaveConfig();
DWORD WINAPI     WinLoop(LPVOID lpParameter);
DWORD WINAPI     TopLoop(LPVOID lpParameter);

// Initialize Gdi+ and load font
void Init(HWND Parent)
{
	MessageHwnd = Parent;
	BackGroundRGB[0] = 0;
	BackGroundRGB[1] = 0;
	BackGroundRGB[2] = 0;
	TextRGB[0] = 255;
	TextRGB[1] = 255;
	TextRGB[2] = 255;
	Movement = 0;
	DlgHwnd = nullptr;
	Dll = GetModuleHandle(L"ReaderBar.dll");
	GetModuleFileName(Dll, ConfigPath, 256);
	wcscpy(wcsstr(ConfigPath, L"dll"), L"ini");
	ZeroMemory(TextBuffer, sizeof(TextBuffer));
	GdiplusStartup(&GdiToken, &GdiStartup, NULL);
	GdiBlend.BlendOp = AC_SRC_OVER;
	GdiBlend.BlendFlags = 0;
	GdiBlend.AlphaFormat = AC_SRC_ALPHA;
	GdiBlend.SourceConstantAlpha = GdiAlpha = GetPrivateProfileInt(L"Settings", L"Alpha", 255, ConfigPath);
	GdiBackgroundAlpha = GetPrivateProfileInt(L"Settings", L"BackgroundAlpha", 50, ConfigPath);
	FontSize = GetPrivateProfileInt(L"Settings", L"FontSize", 50, ConfigPath);

	GetPrivateProfileString(L"Settings", L"Font", L"свт╡", FontType, 50, ConfigPath);

	CharPerLine = GetPrivateProfileInt(L"Settings", L"Width", 20, ConfigPath);
	TextFormat.GenericTypographic();
	WNDCLASS BarClass;
	BarClass.style = CS_HREDRAW | CS_VREDRAW;
	BarClass.lpfnWndProc = BarProc;
	BarClass.cbClsExtra = 0;
	BarClass.cbWndExtra = 0;
	BarClass.hInstance = Dll;
	BarClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	BarClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	BarClass.hbrBackground = (HBRUSH)0;
	BarClass.lpszMenuName = NULL;
	BarClass.lpszClassName = TEXT("AssistiveReader");
	RegisterClass(&BarClass);
	WinProc = CreateThread(NULL, 0, WinLoop, NULL, 0, NULL);
	TopProc = CreateThread(NULL, 0, TopLoop, NULL, 0, NULL);
};

// Deinitialize Gdi+ and destroy bar window
void DeInit()
{
	SaveConfig();
	CloseHandle(TopProc);
	CloseHandle(WinProc);
	DestroyWindow(BarHwnd);
	GdiplusShutdown(GdiToken);
	UnregisterClass(L"AssistiveReader", Dll);
};

// Set the visiblity of the bar
void Show(bool Visible)
{
	ShowWindow(BarHwnd, Visible);
	UpdateWindow(BarHwnd);
};

// Display a null terminated unicode string
void Display(wchar_t* Text)
{
	wcscpy(TextBuffer, Text);
	InvalidateRect(BarHwnd, NULL, true);
	UpdateWindow(BarHwnd);
};

// Clear the current display message from bar
void Clear()
{
	ZeroMemory(TextBuffer, sizeof(TextBuffer));
	InvalidateRect(BarHwnd, NULL, true);
	UpdateWindow(BarHwnd);
};

// WinProc Callback Function
LRESULT CALLBACK BarProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:					
		OnPaint();	
		break;
	case WM_DESTROY:
		SaveConfig();
		break;
	case WM_RBUTTONDOWN:
	case WM_NCRBUTTONDOWN:
	case WM_LBUTTONDOWN:
	case WM_NCLBUTTONDOWN:
	case WM_POINTERDOWN:
	case WM_NCPOINTERDOWN:
	case WM_LBUTTONUP:
	case WM_NCLBUTTONUP:
	case WM_POINTERUP:
	case WM_NCPOINTERUP:
	case WM_MOUSEMOVE:
	case WM_NCMOUSEMOVE:
	case WM_POINTERUPDATE:
	case WM_NCPOINTERUPDATE:
		InputProc(message, wParam, lParam);
		return MA_NOACTIVATEANDEAT;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	};
};

// Paint Function
void OnPaint()
{
	PAINTSTRUCT  Paint;
	HDC mBuffer = BeginPaint(BarHwnd, &Paint);
	RECT rc;
	GetWindowRect(BarHwnd, &rc);
	GdiBlend.SourceConstantAlpha = GdiAlpha;
	HDC mMemory = CreateCompatibleDC(mBuffer);
	HBITMAP hBitmap = CreateCompatibleBitmap(mBuffer, BarSize.X, BarSize.Y);
	SelectObject(mMemory, hBitmap);
	HDC Screen = ::GetDC(BarHwnd);	
	POINT WinPos = { rc.left, rc.top };

	Graphics Graph(mMemory);
	Graph.SetSmoothingMode(SmoothingModeAntiAlias);
	GraphicsPath RT;
	Font TextFont(FontType, FontSize);
	int NumberOfChars = wcslen(TextBuffer);
	int Lines = NumberOfChars / CharPerLine;
	if (NumberOfChars % CharPerLine > 0 || Lines == 0)
		Lines++;

	BarSize.X = CharPerLine * (FontSize * 96 / 72 + GetTextCharacterExtra(mMemory))+20;
	BarSize.Y = Lines* (FontSize * 96 / 72 + 22);

	RT.AddArc(0, 0, 40, 40, 180, 90);
	RT.AddArc(0 + BarSize.X - 40, 0, 40, 40, 270, 90);
	RT.AddArc(0 + BarSize.X - 40, 0 + BarSize.Y - 40, 40, 40, 0, 90);
	RT.AddArc(0, 0 + BarSize.Y - 40, 40, 40, 90, 90);
	SolidBrush   BackGroundBrush(Gdiplus::Color(GdiBackgroundAlpha, BackGroundRGB[0], BackGroundRGB[1], BackGroundRGB[2]));
	Graph.FillPath(&BackGroundBrush, &RT);
	SolidBrush   GxTextBrush(Gdiplus::Color(255, TextRGB[0], TextRGB[1], TextRGB[2]));
	Graph.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
	Graph.DrawString(TextBuffer, -1, &TextFont, RectF(5, 5, BarSize.X + 5, BarSize.Y + 5), &TextFormat, &GxTextBrush);

	SIZE WinSize = { BarSize.X, BarSize.Y };
	POINT Src = { 0, 0 };
	DWORD dwExStyle = GetWindowLong(BarHwnd, GWL_EXSTYLE);
	if ((dwExStyle & 0x80000) != 0x80000)
		SetWindowLong(BarHwnd, GWL_EXSTYLE, dwExStyle ^ 0x80000);
	GdiBlend.SourceConstantAlpha = GdiAlpha;
	UpdateLayeredWindow(BarHwnd, Screen, &WinPos, &WinSize, mMemory, &Src, 0, &GdiBlend, ULW_ALPHA);
	Graph.ReleaseHDC(mMemory);
	::ReleaseDC(BarHwnd, Screen);
	::ReleaseDC(BarHwnd, mBuffer);
	DeleteObject(hBitmap);
	DeleteDC(mMemory);
	EndPaint(BarHwnd, &Paint);
};

// Proceed User Inputs
void InputProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT rc;
	switch (message)
	{
	case WM_LBUTTONDOWN:
	case WM_NCLBUTTONDOWN:
	{SystemParametersInfo(SPI_SETDRAGFULLWINDOWS, TRUE, NULL, 0);
	SendMessage(BarHwnd, WM_SYSCOMMAND, 0xF012, 0);
	GetWindowRect(BarHwnd, &rc);
	BarPosition = { rc.top, rc.left };
	if (TouchTime.Elasped()>10000)
	{
		SaveConfig();
		SendMessage(MessageHwnd, WM_DESTROY, NULL, NULL);
	}
	return; }
	case WM_LBUTTONUP:
	case WM_NCLBUTTONUP:
		TouchTime.Start();
		return;
	case WM_RBUTTONDOWN:
	case WM_NCRBUTTONDOWN:
		OpenConfigWin();
		return;
	case WM_MOUSEMOVE:
	case WM_NCMOUSEMOVE:
		TouchTime.Start();
		return;
	case WM_POINTERDOWN:
	case WM_NCPOINTERDOWN:	
		TouchTime.Start();
		GetWindowRect(BarHwnd, &rc);
		GetWindowRect(BarHwnd, &DownPosition);
		TouchOffset.X = GET_X_LPARAM(lParam) - rc.left;
		TouchOffset.Y = GET_Y_LPARAM(lParam) - rc.top;
		return;
	case WM_POINTERUP:
	case WM_NCPOINTERUP:
		if (TouchTime.Elasped() > 1000 && Movement<30)
			OpenConfigWin();
		Movement = 0;
		return;
	case WM_POINTERUPDATE:
	case WM_NCPOINTERUPDATE:
		if (TouchTime.Elasped()>10000)
		{
			SaveConfig();
			SendMessage(MessageHwnd, WM_DESTROY, NULL, NULL);
		}
		MoveWindow(BarHwnd
			, GET_X_LPARAM(lParam) - TouchOffset.X
			, GET_Y_LPARAM(lParam) - TouchOffset.Y
			, BarSize.X
			, BarSize.Y
			, true);
		GetWindowRect(BarHwnd, &rc);
		BarPosition = { rc.top, rc.left };
		Movement = abs(DownPosition.left - rc.left) + abs(DownPosition.top - rc.top);
		return;
	};
};

// Save All configuration variables
void SaveConfig()
{
	wchar_t Buffer[50];
	swprintf(Buffer, L"%d", GdiAlpha);
	WritePrivateProfileString(L"Settings", L"Alpha", Buffer, ConfigPath);
	swprintf(Buffer, L"%d", GdiBackgroundAlpha);
	WritePrivateProfileString(L"Settings", L"BackgroundAlpha", Buffer, ConfigPath);
	swprintf(Buffer, L"%d", FontSize);
	WritePrivateProfileString(L"Settings", L"FontSize", Buffer, ConfigPath);
	swprintf(Buffer, L"%d", CharPerLine);
	WritePrivateProfileString(L"Settings", L"Width", Buffer, ConfigPath);
	WritePrivateProfileString(L"Settings", L"Font", FontType, ConfigPath);
	swprintf(Buffer, L"%d", BarPosition.X);
	WritePrivateProfileString(L"Settings", L"PosX", Buffer, ConfigPath);
	swprintf(Buffer, L"%d", BarPosition.Y);
	WritePrivateProfileString(L"Settings", L"PosY", Buffer, ConfigPath);
	swprintf(Buffer, L"%d", BarSize.Y);
	WritePrivateProfileString(L"Settings", L"Height", Buffer, ConfigPath);
};

// Create Window and Keep looping messages
DWORD WINAPI     WinLoop(LPVOID lpParameter)
{
	BarHwnd = CreateWindowEx(
		WS_EX_NOACTIVATE | WS_EX_TOPMOST,
		L"AssistiveReader",
		L"AssistiveReader",
		WS_POPUP | WS_VISIBLE,
		BarPosition.X = GetPrivateProfileInt(L"Settings", L"PosX", 500, ConfigPath),
		BarPosition.Y = GetPrivateProfileInt(L"Settings", L"PosY", 500, ConfigPath),
		BarSize.X = GetPrivateProfileInt(L"Settings", L"Width", 550, ConfigPath),
		BarSize.Y = GetPrivateProfileInt(L"Settings", L"Height", 100, ConfigPath),
		NULL,
		NULL,
		Dll,
		NULL
		);
	MSG msg;
	while (GetMessage(&msg, BarHwnd, NULL, NULL) > 0){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
};

// Bring Bar to Top every 5 seconds
DWORD WINAPI     TopLoop(LPVOID lpParameter)
{
	while (true)
	{
		BringWindowToTop(BarHwnd);
		Sleep(5000);
	}
};

// Open Configuration Window
void             OpenConfigWin()
{
	if (DlgHwnd == nullptr)
	{
		DlgHwnd = CreateDialog(Dll, MAKEINTRESOURCE(IDD_DIALOG1), GetDesktopWindow(), DlgProc);
		ShowWindow(DlgHwnd, SW_SHOW);
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)>0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		SaveConfig();
		DlgHwnd = nullptr;
	}	
};

// FontEnumProc Callback Function
INT CALLBACK FontEnumProc(LOGFONT *plf, TEXTMETRIC*, INT, LPARAM)
{
	SendMessage(GetDlgItem(DlgHwnd, IDC_COMBO1), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>((LPCTSTR)(plf->lfFaceName)));
	return TRUE;
}

// DlgProc Callback Function
INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	wchar_t Buffer[50];
	switch (message)
	{
	case WM_INITDIALOG:
		DlgHwnd = hWnd;
		swprintf(Buffer, L"%d", GdiAlpha);
		SetDlgItemText(DlgHwnd, IDC_EDIT7, Buffer);
		swprintf(Buffer, L"%d", GdiBackgroundAlpha);
		SetDlgItemText(DlgHwnd, IDC_EDIT8, Buffer);
		SetDlgItemText(DlgHwnd, IDC_COMBO1, FontType);
		swprintf(Buffer, L"%d", FontSize);
		SetDlgItemText(DlgHwnd, IDC_EDIT10, Buffer);
		swprintf(Buffer, L"%d", CharPerLine);
		SetDlgItemText(DlgHwnd, IDC_EDIT11, Buffer);

		EnumFontFamilies(GetWindowDC(NULL), (LPTSTR)NULL, (FONTENUMPROC)FontEnumProc, NULL);

		return 1;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:	
			GetDlgItemText(DlgHwnd, IDC_EDIT7, Buffer, 50);
			GdiAlpha = _wtoi(Buffer);
			GetDlgItemText(DlgHwnd, IDC_EDIT8, Buffer, 50);
			GdiBackgroundAlpha = _wtoi(Buffer);
			GetDlgItemText(DlgHwnd, IDC_COMBO1, FontType, 50);
			GetDlgItemText(DlgHwnd, IDC_EDIT10, Buffer, 50);
			FontSize = _wtoi(Buffer);
			GetDlgItemText(DlgHwnd, IDC_EDIT11, Buffer, 50);
			CharPerLine = _wtoi(Buffer);

			PostQuitMessage(0);
			DestroyWindow(DlgHwnd);
		}
		return 0;
	case WM_SYSCOMMAND:
		if (wParam == SC_CLOSE)
		{
			PostQuitMessage(0);
			DestroyWindow(DlgHwnd);
		}
		return 0;
	}
	return 0;
};


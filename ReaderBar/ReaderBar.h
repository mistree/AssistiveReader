#include <Windows.h>

// Initialize Gdi+ and load font
void Init(HWND Parent);

// Deinitialize Gdi+ and destroy the bar window
void DeInit();

// Set the visibility of the bar
void Show(bool Visible);

// Display a null terminated unicode string
void Display(wchar_t* Text);

// Clear the current display message from bar
void Clear();
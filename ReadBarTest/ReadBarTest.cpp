#define DllImport   __declspec( dllimport )

// Initialize Gdi+ and load font
void DllImport Init();

// Deinitialize Gdi+ and destroy the bar window
void DllImport DeInit();

// Set the visibility of the bar
void DllImport Show(bool Visible);

// Display a null terminated unicode string
void DllImport Display(wchar_t* Text);

// Clear the current display message from bar
void DllImport Clear();

void main()
{
	Init();
	Show(true);
	while (true)
	Display(L"这是一个测试消息");
	Show(false);
	DeInit();
}

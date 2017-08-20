#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>
#include <stdio.h>

#include "blockgame_platform.h"

#include "blockgame.h"
#include "blockgame.cpp"

#include "win32_blockgame.h"


static bool32 GlobalRunning;

static void
Win32ResizeDIBSection(struct game_screen_buffer *Buffer, uint32 Width, uint32 Height)
{
	if(Buffer->BitmapMemory)
	{
		VirtualFree(Buffer->BitmapMemory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;
	Buffer->BytesPerPixel = 4;
	Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;

	Buffer->BitmapInfo.BitmapOffset = sizeof(struct bitmap_header);
	Buffer->BitmapInfo.InfoHeader.Size = 40;
	Buffer->BitmapInfo.InfoHeader.Width = Width;
	Buffer->BitmapInfo.InfoHeader.Height = -Height;
	Buffer->BitmapInfo.InfoHeader.Planes = 1;
	Buffer->BitmapInfo.InfoHeader.BitsPerPixel = 32;
	Buffer->BitmapInfo.InfoHeader.Compression = BI_RGB;

	uint32 BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
	Buffer->BitmapMemory = (void *)VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	Assert(Buffer->BitmapMemory);
}

static void
Win32DrawScreenBufferToWindow(HDC DeviceContext, struct game_screen_buffer *Buffer,
							  uint32 X, uint32 Y, uint32 Width, uint32 Height)
{
	StretchDIBits(DeviceContext,
				  X, Y, Width, Height,
				  X, Y, Width, Height,
				  Buffer->BitmapMemory,
				  (BITMAPINFO *)&Buffer->BitmapInfo.InfoHeader,
				  DIB_RGB_COLORS, SRCCOPY);
}

static struct win32_window_dimensions
Win32GetWindowDimensions(HWND Window)
{
	RECT ClientRect = {};
	GetClientRect(Window, &ClientRect);

	struct win32_window_dimensions Result = {};
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return(Result);
}

inline static void
Win32ProcessKeyboardMessage(struct game_button_state *Button, bool32 IsDown)
{
	if(Button->EndedDown != IsDown)
	{
		Button->EndedDown = IsDown;
		++Button->HalfTransitionCount;
	}
	if(Button->EndedDown)
	{
		Button->Tapped = true;
	}
}

inline static LARGE_INTEGER
Win32GetWallClock()
{
	LARGE_INTEGER Result = {};
	QueryPerformanceCounter(&Result);
	return(Result);
}

inline static real32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	LARGE_INTEGER CPUFrequency = {};
	QueryPerformanceFrequency(&CPUFrequency);

	real32 Result = ((real32)(End.QuadPart - Start.QuadPart) / (real32)(CPUFrequency.QuadPart));
	return(Result);
}

PLATFORM_READ_ENTIRE_FILE_INTO_MEMORY(Win32ReadEntireFileIntoMemory)
{
	struct file_data Result = {};
	HANDLE File = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0);
	if(File != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;
		if(GetFileSizeEx(File, &FileSize))
		{
			uint32 FileSize32 = (uint32)FileSize.QuadPart;
			DWORD BytesRead = 0;

			Result.FileContents = (uint8 *)VirtualAlloc(0, FileSize32, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			if(ReadFile(File, Result.FileContents, FileSize32, &BytesRead, 0) &&
			   (FileSize32 == BytesRead))
			{
				Result.FileSize = BytesRead;
			}
			else
			{
				Result.FileSize = 0;
				Result.FileContents = 0;
			}
		}
		else
		{
			Result.FileSize = 0;
			Result.FileContents = 0;
		}

		CloseHandle(File);
	}

	return(Result);
}

PLATFORM_WRITE_TO_FILE(Win32WriteToFile)
{
	bool32 Result = false;
	HANDLE File = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if(File != INVALID_HANDLE_VALUE)
	{
		DWORD BytesWritten = 0;
		if(WriteFile(File, Data, Size, &BytesWritten, 0))
		{
			if(BytesWritten == Size)
			{
				Result = true;
			}
		}

		CloseHandle(File);
	}

	return(Result);
}

LRESULT CALLBACK
Win32WindowsCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	LRESULT Result = 0;

	switch(Message)
	{
		case WM_QUIT:
		case WM_DESTROY:
		{
			GlobalRunning = false;
			PostQuitMessage(0);
		} break;
		default:
		{
			Result = DefWindowProc(Window, Message, WParam, LParam);
		} break;
	}

	return(Result);
}

static void
Win32ProcessPendingMessages(struct game_input *Input)
{
	MSG Message;
	while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
	{
		switch(Message.message)
		{
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYUP:
			case WM_KEYDOWN:
			{
				uint32 VKCode = (uint32)Message.wParam;
				bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
				bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);
				if(IsDown != WasDown)
				{
					if((VKCode == 'W') ||
					   (VKCode == VK_UP))
					{
						Win32ProcessKeyboardMessage(&Input->ButtonUp, IsDown);
					}
					else if((VKCode == 'S') ||
							(VKCode == VK_DOWN))
					{
						Win32ProcessKeyboardMessage(&Input->ButtonDown, IsDown);
					}

					if((VKCode == 'A') ||
					   (VKCode == VK_LEFT))
					{
						Win32ProcessKeyboardMessage(&Input->ButtonLeft, IsDown);
					}
					else if((VKCode == 'D') ||
							(VKCode == VK_RIGHT))
					{
						Win32ProcessKeyboardMessage(&Input->ButtonRight, IsDown);
					}
					if(VKCode == VK_SPACE)
					{
						Win32ProcessKeyboardMessage(&Input->ButtonPause, IsDown);
					}
					if(VKCode == VK_RETURN)
					{
						Win32ProcessKeyboardMessage(&Input->ButtonAction, IsDown);
					}
					if(VKCode == VK_ESCAPE)
					{
						GlobalRunning = false;
					}

					if(VKCode == 'L')
					{
						Win32ProcessKeyboardMessage(&Input->ButtonDebugColors, IsDown);
					}
					if(VKCode == 'M')
					{
						Win32ProcessKeyboardMessage(&Input->ButtonDebugMouse, IsDown);
					}
					if(VKCode == 'R')
					{
						Win32ProcessKeyboardMessage(&Input->ButtonReset, IsDown);
					}
					if(VKCode == 'P')
					{
						Win32ProcessKeyboardMessage(&Input->ButtonScreenshot, IsDown);
					}
				}
			} break;
			default:
			{
				TranslateMessage(&Message);
				DispatchMessage(&Message);
			} break;
		}
	}
}

int WINAPI
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow)
{
	WNDCLASSEXA WindowClass = {};
	WindowClass.cbSize = sizeof(WNDCLASSEXA);
	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32WindowsCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "win32_blockgame";

	if(!RegisterClassEx(&WindowClass))
	{
		MessageBox(NULL, "Failed to register window class", "Error", MB_OK);
		return(-1);
	}

	HWND Window = CreateWindowEx(0,
								 WindowClass.lpszClassName,
								 "Win32_BlockGame",
								 WS_OVERLAPPEDWINDOW | WS_VISIBLE,
								 CW_USEDEFAULT, CW_USEDEFAULT,
								 496, 999,  // NOTE(rick): 480x960 play area
								 0, 0, Instance, 0);

	if(!Window)
	{
		MessageBox(NULL, "Failed to create window.", "Error", MB_OK);
		return(-2);
	}

	timeBeginPeriod(1);  // NOTE(rick): High resolution Sleep timing
	real32 TargetFPS = 120.0f;
	real32 TargetSecondsPerFrame = 1.0f / TargetFPS;
	real32 TargetMSPerFrame = TargetSecondsPerFrame * 1000.0f;

	struct game_screen_buffer ScreenBuffer = {};
	struct game_screen_buffer *GlobalScreenBuffer = &ScreenBuffer;
	struct win32_window_dimensions WindowDims = Win32GetWindowDimensions(Window);
	Win32ResizeDIBSection(&ScreenBuffer, WindowDims.Width, WindowDims.Height);

	struct game_memory GameMemory = {};
	GameMemory.PermanentStorageSize = Kilobytes(1024);
	GameMemory.TransientStorageSize = Kilobytes(4096);
	struct platform_state PlatformState = {};
	PlatformState.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
	PlatformState.GameMemoryBlock = VirtualAlloc(0, PlatformState.TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	GameMemory.PermanentStorage = PlatformState.GameMemoryBlock;
	GameMemory.TransientStorage = ((uint8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);
	GameMemory.PlatformReadEntireFileIntoMemory = Win32ReadEntireFileIntoMemory;
	GameMemory.PlatformWriteToFile = Win32WriteToFile;

	struct game_input Input[2] = {};
	struct game_input *NewInput = &Input[0];
	struct game_input *OldInput = &Input[1];

	GlobalRunning = true;
	while(GlobalRunning)
	{
		LARGE_INTEGER Start = Win32GetWallClock();

		*NewInput = {};
		NewInput->dtForFrame = TargetSecondsPerFrame;
		for(uint32 ButtonIndex = 0;
			ButtonIndex < ArrayCount(NewInput->Buttons);
			++ButtonIndex)
		{
			NewInput->Buttons[ButtonIndex].EndedDown = OldInput->Buttons[ButtonIndex].EndedDown;
		}

		Win32ProcessPendingMessages(NewInput);

		POINT MousePosition = {};
		GetCursorPos(&MousePosition);
		ScreenToClient(Window, &MousePosition);
		NewInput->MouseX = MousePosition.x;
		NewInput->MouseY = MousePosition.y;

		GameUpdateAndRender(&ScreenBuffer, &GameMemory, NewInput);

		HDC DeviceContext = GetDC(Window);
		struct win32_window_dimensions WindowDims = Win32GetWindowDimensions(Window);
		Win32DrawScreenBufferToWindow(DeviceContext, &ScreenBuffer,
									  0, 0, WindowDims.Width, WindowDims.Height);
		ReleaseDC(Window, DeviceContext);

		struct game_input *TempInput = NewInput;
		NewInput = OldInput;
		OldInput = TempInput;

		LARGE_INTEGER End = Win32GetWallClock();
		real32 SecondsElapsedForFrame = Win32GetSecondsElapsed(Start, End);
		real32 MSElapsedForFrame = SecondsElapsedForFrame * 1000.0f;

		if(MSElapsedForFrame < TargetMSPerFrame)
		{
			real32 MSToSleep = (real32)(TargetMSPerFrame - MSElapsedForFrame);
			if(MSToSleep > 0.0f)
			{
				Sleep(MSToSleep);
			}

#if 0
			real32 ActualFPS = 1.0f / Win32GetSecondsElapsed(Start, Win32GetWallClock());
			char FPSBuffer[64] = {0};
			_snprintf(FPSBuffer, 64, "%.02ff/s\n", ActualFPS);
			OutputDebugStringA(FPSBuffer);
#endif
		}
	}

	return(0);
}


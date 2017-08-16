#ifndef BLOCKGAME_PLATFORM_H

#include <stdint.h>

typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;
typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;
typedef int32_t bool32;
typedef float real32;
typedef double real64;

#define true 1
#define false 0

#define Assert(Condition) if(!(Condition)) { *(uint32 *)0 = 0; }

#define Kilobytes(Value) (Value * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#pragma pack(push, 1)
struct bitmap_header
{
	uint16 FileType;
	uint32 FileSize;
	uint16 ReservedOne;
	uint16 ReservedTwo;
	uint32 BitmapOffset;
	struct
	{
		int32 Size;
		int32 Width;
		int32 Height;
		uint16 Planes;
		uint16 BitsPerPixel;
		int32 Compression;
		int32 SizeOfBitmap;
		int32 XPelsPerMeter;
		int32 YPelsPerMeter;
		int32 ColorsUsed;
		int32 ColorsImportant;
	} InfoHeader;

	uint32 RedMask;
	uint32 GreenMask;
	uint32 BlueMask;
};
#pragma pack(pop)

struct game_screen_buffer
{
	void *BitmapMemory;
	int32 Width;
	int32 Height;
	int32 BytesPerPixel;
	int32 Pitch;
	struct bitmap_header BitmapInfo;
};

struct bitmap
{
	void *BitmapMemory;
	int32 Width;
	int32 Height;
	int32 BytesPerPixel;
	int32 Pitch;
};

struct platform_state
{
	uint64 TotalSize;
	void *GameMemoryBlock;
};

struct file_data
{
	uint8 *FileContents;
	uint32 FileSize;
};

#define PLATFORM_READ_ENTIRE_FILE_INTO_MEMORY(name) struct file_data name(char *FileName)
typedef PLATFORM_READ_ENTIRE_FILE_INTO_MEMORY(platform_read_entire_file_into_memory);

#define PLATFORM_WRITE_TO_FILE(name) bool32 name(char *FileName, void *Data, uint32 Size)
typedef PLATFORM_WRITE_TO_FILE(platform_write_to_file);

struct game_memory
{
	bool32 IsInitialized;

	uint64 PermanentStorageSize;
	void *PermanentStorage;

	uint64 TransientStorageSize;
	void *TransientStorage;

	platform_read_entire_file_into_memory *PlatformReadEntireFileIntoMemory;
	platform_write_to_file *PlatformWriteToFile;
};

struct memory_arena
{
	uint64 Size;
	uint64 Used;
	uint8 *Base;

	int32 TempCount;
};

struct temporary_memory
{
	memory_arena *Arena;
	uint64 Used;
};

#define PushStruct(Arena, Type) (Type *)(_PushMem(Arena, sizeof(Type)))
#define PushArray(Arena, Count, Type) (Type *)(_PushMem(Arena, Count * sizeof(Type)))
#define PushSize(Arena, Size) _PushMem(Arena, Size)

inline void *
_PushMem(struct memory_arena *Arena, uint64 Size)
{
	Assert(Arena->Used + Size <= Arena->Size);

	void *Result = Arena->Base + Arena->Used;
	Arena->Used += Size;

	return(Result);
}

inline void
InitializeArena(struct memory_arena *Arena, uint64 Size, void *Base)
{
	Arena->Size = Size;
	Arena->Base = (uint8 *)Base;
	Arena->Used = 0;
	Arena->TempCount = 0;
}

inline struct temporary_memory
BeginTemporaryArena(struct memory_arena *Arena)
{
	struct temporary_memory Result = {};
	Result.Arena = Arena;
	Result.Used = Arena->Used;
	++Arena->TempCount;

	return(Result);
}

inline void
EndTemporaryArena(struct temporary_memory TempMem)
{
	struct memory_arena *Arena = TempMem.Arena;

	Assert(Arena->Used >= TempMem.Used);
	Arena->Used = TempMem.Used;
	--Arena->TempCount;
}

inline void
ClearMemory(void *Memory, uint32 Size)
{
	uint8 *Byte = (uint8 *)Memory;
	while(--Size > 0)
	{
		*Byte++ = 0x00;
	}
}

#define GAME_UPDATE_AND_RENDER(name) void name(struct game_screen_buffer *Buffer, struct game_memory *Memory, struct game_input *Input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

struct game_button_state
{
	bool32 EndedDown;
	bool32 Tapped;
	int32 HalfTransitionCount;  // NOTE(rick): Do we need this?
};

struct game_input
{
	real32 dtForFrame;
	real32 MouseX;
	real32 MouseY;
	union
	{
		struct game_button_state Buttons[10];
		struct
		{
			struct game_button_state ButtonLeft;
			struct game_button_state ButtonRight;
			struct game_button_state ButtonUp;
			struct game_button_state ButtonDown;

			struct game_button_state ButtonAction;  // Enter
			struct game_button_state ButtonCancel;  // Escape

			struct game_button_state ButtonPause;  // Space

			struct game_button_state ButtonDebugColors;  // L
			struct game_button_state ButtonDebugMouse;  // M
			struct game_button_state ButtonReset;  // R
			struct game_button_state ButtonScreenshot;  // P
		};
	};
};


#define BLOCKGAME_PLATFORM_H
#endif

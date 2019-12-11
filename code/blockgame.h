#ifndef BLOCKGAME_H

#include "blockgame_random.h"

union v2
{
	struct
	{
		real32 X;
		real32 Y;
	};
	struct
	{
		real32 Width;
		real32 Height;
	};
	real32 E[2];
};

inline v2
V2(real32 A, real32 B)
{
	union v2 Result = {};
	Result.X = A;
	Result.Y = B;

	return(Result);
}

inline v2
V2i(int32 A, int32 B)
{
	union v2 Result = {};
	Result.X = (real32)A;
	Result.Y = (real32)B;

	return(Result);
}

inline v2
operator*(real32 A, v2 B)
{
	v2 Result = {};
	Result.X = A * B.X;
	Result.Y = A * B.Y;
	return(Result);
}
inline v2
operator*(v2 A, real32 B)
{
	v2 Result = {};
	Result.X = B * A.X;
	Result.Y = B * A.Y;
	return(Result);
}

inline v2
operator+(v2 A, v2 B)
{
	v2 Result = {};
	Result.X = A.X + B.X;
	Result.Y = A.Y + B.Y;
	return(Result);
}

inline v2
operator-(v2 A, v2 B)
{
	v2 Result = {};
	Result.X = A.X - B.X;
	Result.Y = A.Y - B.Y;
	return(Result);
}

inline real32 
Inner(v2 A, v2 B)
{
	real32 Result = A.X*B.X + A.Y+B.Y;
	return(Result);
}

union v3
{
	struct
	{
		real32 X;
		real32 Y;
		real32 Z;
	};
	struct
	{
		real32 R;
		real32 G;
		real32 B;
	};
	real32 E[3];
};

inline v3
V3(real32 A, real32 B, real32 C)
{
	union v3 Result = {};
	Result.X = A;
	Result.Y = B;
	Result.Z = C;

	return(Result);
}

inline v3
V3i(int32 A, int32 B, int32 C)
{
	union v3 Result = {};
	Result.X = (real32)A;
	Result.Y = (real32)B;
	Result.Z = (real32)C;

	return(Result);
}

union v4
{
	struct
	{
		real32 X;
		real32 Y;
		real32 Z;
		real32 W;
	};
	struct
	{
		real32 R;
		real32 G;
		real32 B;
		real32 A;
	};
	real32 E[4];
};

inline v4
V4(real32 A, real32 B, real32 C, real32 D)
{
	union v4 Result = {};
	Result.X = A;
	Result.Y = B;
	Result.Z = C;
	Result.W = D;

	return(Result);
}

inline v4
V4(v3 A, real32 B)
{
	union v4 Result = {};
	Result.X = A.X;
	Result.Y = A.Y;
	Result.Z = A.Z;
	Result.W = B;

	return(Result);
}

struct entity
{
	v2 Position;
	v2 Velocity;
	v2 Size;
};

struct room_chunk
{
	bool32 PlayerVisited;
	uint32 *RoomData;
	v2 RoomPosition;
	v2 RoomDims;
	v3 RoomColor;
};

struct world
{
	uint32 RoomHeightInGridCells;
	uint32 RoomWidthInGridCells;
	struct room_chunk Rooms[16];
};

union color_scheme
{
	v3 Colors[6];
	struct
	{
		union v3 Color1;
		union v3 Color2;
		union v3 Color3;
		union v3 Color4;
		union v3 Color5;
		union v3 Color6;

		union v3 PlayerColor;
		union v3 BackgroundColor;
	};
};

enum GameState
{
	GameState_MainMenu = 0,
	GameState_NewGame,
	GameState_PauseScreen,
	GameState_Playing,
	GameState_GameOver,

	GameState_Count,
};

struct main_menu
{
	uint32 MenuItemsCount;
	int32 SelectedMenuItem;
	struct bitmap *MenuItems[5];
};

struct game_state
{
	bool32 IsInitialized;
	bool32 CameraFollowingPlayer;
	bool32 EnableDebugMouse;

	real32 PixelsPerMeter;
	real32 MetersToPixels;
	real32 TileSideInPixels;
	real32 TileSideInMeters;

	real32 WorldShiftHeight;

	enum GameState State;

	struct main_menu MainMenu;

	struct world *World;

	struct memory_arena WorldArena;
	struct memory_arena TransArena;
	struct game_input Input;
	struct entity PlayerEntity;

	v2 Gravity;

	union color_scheme *Colors;
	union color_scheme ColorSchemeLight;
	union color_scheme ColorSchemeDark;

	uint32 Score;

	uint8 *CurrentScoreString;
	uint8 *PreviousScoreString;
	uint8 ScoreAsString[2][16];
	struct bitmap *ScoreBitmap;

	struct file_data FontData;
	struct bitmap *Text;
	struct bitmap *PauseScreenMessage;
	struct bitmap *Logo;
	struct bitmap *GameOverMessage;
};

#define BLOCKGAME_H
#endif

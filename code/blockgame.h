#ifndef BLOCKGAME_H

#include "blockgame_random.h"

union v2
{
	struct
	{
		real32 X;
		real32 Y;
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

struct entity
{
	v2 Position;
	v2 Velocity;
};

struct room_chunk
{
	uint32 *RoomData;
	v2 RoomPosition;
};

struct world
{
	uint32 RoomHeightInGridCells;
	uint32 RoomWidthInGridCells;
	struct room_chunk Rooms[16];
};

struct game_state
{
	bool32 IsInitialized;
	bool32 CameraFollowingPlayer;

	real32 PixelsPerMeter;
	real32 MetersToPixels;
	real32 TileSideInPixels;
	real32 TileSideInMeters;

	real32 WorldShiftHeight;

	struct world *World;

	struct memory_arena WorldArena;
	struct game_input Input;
	struct entity PlayerEntity;

	v2 Gravity;
};

#define BLOCKGAME_H
#endif

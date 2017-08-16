/**
 * TODO List
 * * Game Keys
 *   * Pause etc..
 * * Collision Detection
 * * TEXT RENDERING
 *   * Menus
 *   * Scoreboard? Local or Online
 * * Sound?
 */

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"

static void
ClearScreenToColor(struct game_screen_buffer *Buffer, v3 Color)
{
	uint32 PixelColor = ( (0xff << 24) |
					 ((int32)Color.R << 16) |
					 ((int32)Color.G << 8) |
					 ((int32)Color.B << 0) );

	uint8 *Base = (uint8*)Buffer->BitmapMemory;
	for(int32 Y = 0; Y < Buffer->Height; ++Y)
	{
		uint32 *Pixel = (uint32 *)(Base + (Y * Buffer->Pitch));
		for(int32 X = 0; X < Buffer->Width; ++X)
		{
			*Pixel++ = PixelColor;
		}
	}
}

static void
DrawRectangle(struct game_screen_buffer *Buffer, v2 Position, v2 Dims, v3 Color)
{
	int32 MinX = (int32)Position.X;
	int32 MinY = (int32)Position.Y;
	int32 MaxX = (int32)Position.X + Dims.X;
	int32 MaxY = (int32)Position.Y + Dims.Y;

	if(MinX < 0) { MinX = 0; }
	if(MinY < 0) { MinY = 0; }
	if(MaxX > Buffer->Width) { MaxX = Buffer->Width; }
	if(MaxY > Buffer->Height) { MaxY = Buffer->Height; }

	if(MaxX < MinX) { MaxX = MinX; }
	if(MaxY < MinY) { MaxY = MinY; }

	uint32 PixelColor = ( (0xff << 24) |
						  ((int32)Color.R << 16) |
						  ((int32)Color.G << 8) |
						  ((int32)Color.B << 0) );

	uint8 *Row = ((uint8*)Buffer->BitmapMemory + (MinY * Buffer->Pitch) + (MinX * Buffer->BytesPerPixel));
	for(uint32 Y = MinY; Y < MaxY; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for(uint32 X = MinX; X < MaxX; ++X)
		{
			*Pixel++ = PixelColor;
		}
		Row += Buffer->Pitch;
	}
}

static void
DrawBitmap(struct game_screen_buffer *Buffer, v2 Position, struct bitmap Bitmap)
{
	int32 MinX = (int32)Position.X;
	int32 MinY = (int32)Position.Y;
	int32 MaxX = (int32)Position.X + Bitmap.Width;
	int32 MaxY = (int32)Position.Y + Bitmap.Height;

	if(MinX < 0) { MinX = 0; }
	if(MinY < 0) { MinY = 0; }
	if(MaxX > Buffer->Width) { MaxX = Buffer->Width; }
	if(MaxY > Buffer->Height) { MaxY = Buffer->Height; }

	if(MaxX < MinX) { MaxX = MinX; }
	if(MaxY < MinY) { MaxY = MinY; }

	uint8 *Row = ((uint8*)Buffer->BitmapMemory + (MinY * Buffer->Pitch) + (MinX * Buffer->BytesPerPixel));
	uint32 *SourcePixel = (uint32 *)Bitmap.BitmapMemory;
	for(uint32 Y = MinY; Y < MaxY; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for(uint32 X = MinX; X < MaxX; ++X)
		{
			*Pixel++ = *SourcePixel++;
		}
		Row += Buffer->Pitch;
	}
}

static void
DrawTextBitmap(struct game_screen_buffer *Buffer, v2 Position, struct bitmap *Bitmap, v4 Color)
{
	int32 MinX = (int32)Position.X;
	int32 MinY = (int32)Position.Y;
	int32 MaxX = (int32)Position.X + Bitmap->Width;
	int32 MaxY = (int32)Position.Y + Bitmap->Height;

	int32 SourceOffsetX = 0;
	if(MinX < 0)
	{
		SourceOffsetX = -MinX;
		MinX = 0;
	}

	int32 SourceOffsetY = 0;
	if(MinY < 0)
	{
		SourceOffsetY = -MinY;
		MinY = 0;
	}

	if(MaxX > Buffer->Width)
	{
		MaxX = Buffer->Width;
	}

	if(MaxY > Buffer->Height)
	{
		MaxY = Buffer->Height;
	}

	if(MaxX < MinX) { MaxX = MinX; }
	if(MaxY < MinY) { MaxY = MinY; }

	uint8 *DestRow = ((uint8*)Buffer->BitmapMemory + (MinY * Buffer->Pitch) + (MinX * Buffer->BytesPerPixel));
	uint8 *SourceRow = ((uint8*)Bitmap->BitmapMemory + (SourceOffsetY * Bitmap->Pitch) + (SourceOffsetX * Bitmap->BytesPerPixel));
	for(uint32 Y = MinY; Y < MaxY; ++Y)
	{
		uint32 *Pixel = (uint32 *)DestRow;
		uint32 *SourcePixel = (uint32 *)SourceRow;
		for(uint32 X = MinX; X < MaxX; ++X)
		{
			if(*SourcePixel != 0)
			{
				real32 SourceAlpha = (real32)((*SourcePixel >> 24) & 0xff);
				real32 RSA = (SourceAlpha / 255.0f) * Color.A;
				real32 SourceRed = Color.A * Color.R;
				real32 SourceGreen = Color.A * Color.G;
				real32 SourceBlue = Color.A * Color.B;

				real32 DestAlpha = (real32)((*Pixel >> 24) & 0xff);
				real32 DestRed = (real32)((*Pixel >> 16) & 0xff);
				real32 DestGreen = (real32)((*Pixel >> 8) & 0xff);
				real32 DestBlue = (real32)((*Pixel >> 0) & 0xff);
				real32 RDA = DestAlpha / 255.0f;

				real32 InverseRSA = 1.0f - RSA;
				real32 OutAlpha = 255.0f * (RSA + RDA - RSA * RDA);
				real32 OutRed = (SourceRed * RSA) + (DestRed * (1.0f  - RSA));
				real32 OutGreen = (SourceGreen * RSA) + (DestGreen * (1.0f  - RSA));
				real32 OutBlue = (SourceBlue * RSA) + (DestBlue * (1.0f  - RSA));

				*Pixel = ( ((uint32)(OutAlpha + 0.5f) << 24) |
						   ((uint32)(OutRed + 0.5f) << 16) |
						   ((uint32)(OutGreen + 0.5f) << 8) |
						   ((uint32)(OutBlue + 0.5f) << 0) );
			}

			Pixel++, SourcePixel++;
		}
		DestRow += Buffer->Pitch;
		SourceRow += Bitmap->Pitch;
	}
}

static void
RenderDebugGrid(struct game_screen_buffer *Buffer, v2 GridDims)
{
	uint32 *Pixel = (uint32 *)Buffer->BitmapMemory;
	uint32 PixelColor = 0xff000000;
	for(uint32 Y = 0; Y < Buffer->Height; ++Y)
	{
		for(uint32 X = 0; X < Buffer->Width; ++X)
		{
			if(X % (int32)GridDims.X == 0)
			{
				*Pixel = PixelColor;
			}
			if(Y % (int32)GridDims.Y == 0)
			{
				*Pixel = PixelColor;
			}

			++Pixel;
		}
	}
}

static void
GenerateRoom(struct game_state *GameState, struct room_chunk *Room,
			 uint32 RoomWidth, uint32 RoomHeight)
{
	Room->RoomColor = GameState->Colors->Colors[(RandomInt32() % ArrayCount(GameState->Colors->Colors))];
	Room->PlayerVisited = false;
	Room->RoomDims = GameState->TileSideInPixels * V2(GameState->World->RoomWidthInGridCells, GameState->World->RoomHeightInGridCells);

	Room->RoomPosition = V2(0.0f, 0.0f);
	uint32 Gap = 4 + (RandomInt32() % (RoomWidth - 8));

	uint32 *CellData = Room->RoomData;
	for(uint32 Y = 0; Y < RoomHeight; ++Y)
	{
		for(uint32 X = 0; X < RoomWidth; ++X)
		{
			*CellData = 0;
			if(Y == 0)
			{
				if((X >= Gap - 2) && (X <= Gap + 2) )
				{
					*CellData = 0;
				}
				else
				{
					*CellData = 1;
				}
			}
			++CellData;
		}
	}

	// TODO(rick): We need to see why this is creating random blocks outside of
	// the random block spawning areas, such as at x = 0 or y = 1
	// TODO(rick): There is some strangeness going on with several rooms having
	// the exact same layout (exit and random blocks), should check to see if
	// this is just a side-effect of our psuedo-random number generator or an
	// actual bug.
	CellData = Room->RoomData;
	*(CellData + (((3 + (RandomInt32() % 3)) * RoomWidth) + (5 + (RandomInt32() % 10)))) = 1;
	*(CellData + (((9 + (RandomInt32() % 3)) * RoomWidth) + (5 + (RandomInt32() % 10)))) = 1;
}

static void
RecycleRoom(struct game_state *GameState, struct room_chunk * Room)
{
	struct room_chunk *HighestRoom = Room;
	for(uint32 RoomIndex = 0; RoomIndex < ArrayCount(GameState->World->Rooms); ++RoomIndex)
	{
		struct room_chunk *RoomToCheck = GameState->World->Rooms + RoomIndex;
		if(RoomToCheck->RoomPosition.Y < HighestRoom->RoomPosition.Y)
		{
			HighestRoom = RoomToCheck;
		}
	}

	GenerateRoom(GameState, Room, GameState->World->RoomWidthInGridCells, GameState->World->RoomHeightInGridCells);
	Room->RoomPosition.Y = HighestRoom->RoomPosition.Y - (GameState->TileSideInPixels * GameState->World->RoomHeightInGridCells);
}

static bool32
TestCollision(v2 TestEntityPosition, v2 TestEntityDims,
			  v2 ColliderPosition, v2 ColliderDims)
{
	bool32 Result = false;

	ColliderDims = ColliderDims + TestEntityDims;
	ColliderPosition = ColliderPosition - (0.5f * TestEntityDims);

	Result = ( (TestEntityPosition.X >= ColliderPosition.X) &&
			   (TestEntityPosition.X <= ColliderPosition.X + ColliderDims.Width) &&
			   (TestEntityPosition.Y >= ColliderPosition.Y) &&
			   (TestEntityPosition.Y <= ColliderPosition.Y + ColliderDims.Height) );

	return(Result);
}

static struct bitmap *
CreateEmptyBitmap(struct memory_arena *Arena)
{
	struct bitmap *Result = PushStruct(Arena, struct bitmap);
	Result->Width = 0;
	Result->Height = 0;
	Result->Pitch = 0;
	Result->BytesPerPixel = 0;
	Result->BitmapMemory = 0;

	return(Result);
}

static struct bitmap *
CreateEmptyBitmap(struct memory_arena *Arena, uint32 Width, uint32 Height)
{
	struct bitmap *Result = PushStruct(Arena, struct bitmap);
	Result->Width = Width;
	Result->Height = Height;
	Result->BytesPerPixel = 4;
	Result->Pitch = Result->Width * Result->BytesPerPixel;

	uint32 BufferSize = (Width * Height) * Result->BytesPerPixel;
	Result->BitmapMemory = PushArray(Arena, BufferSize, uint32);

	return(Result);
}

static struct bitmap *
TextToBitmap(struct memory_arena *Arena, struct game_state *GameState, real32 FontSize,
			 char *String, struct bitmap *UseThisBitmap = 0, int32 Width = 0, int32 Height = 0)
{
#if 1
	int32 Ascent = 0;
	int32 Baseline = 0;
	real32 Scale = 0.0f;
	real32 XPos = 2.0f;

	stbtt_fontinfo Font = {};
	stbtt_InitFont(&Font, GameState->FontData.FileContents, stbtt_GetFontOffsetForIndex(GameState->FontData.FileContents, 0));
	Scale = stbtt_ScaleForPixelHeight(&Font, FontSize);
	stbtt_GetFontVMetrics(&Font, &Ascent, 0, 0);
	Baseline = (int32)(Ascent * Scale);

	uint32 BitmapWidth = 0;
	uint32 BitmapHeight = 0;
	struct bitmap *Result = 0;
	if(!UseThisBitmap)
	{
		BitmapWidth = Width;
		BitmapHeight = Height;
		if((BitmapWidth == 0) || (BitmapHeight == 0))
		{
			for(char *TempString = String; *TempString != 0; ++TempString)
			{
				int32 Advance = 0;
				int32 LSB = 0;
				int32 X0, Y0;
				int32 X1, Y1;
				stbtt_GetCodepointHMetrics(&Font, *TempString, &Advance, &LSB);
				stbtt_GetCodepointBitmapBox(&Font, *TempString, Scale, Scale, &X0, &Y0, &X1, &Y1);

				BitmapWidth += (X1 - X0) + (Advance * Scale);

				uint32 CharacterHeight = Y1 - Y0;
				if(CharacterHeight > BitmapHeight)
				{
					BitmapHeight = CharacterHeight;
				}
			}
		}

		Result = CreateEmptyBitmap(Arena, BitmapWidth, BitmapHeight);
	}
	else
	{
		ClearMemory(UseThisBitmap->BitmapMemory, (UseThisBitmap->Width * UseThisBitmap->Height) * UseThisBitmap->BytesPerPixel);
		BitmapWidth = UseThisBitmap->Width;
		BitmapHeight= UseThisBitmap->Height;
		Result = UseThisBitmap;
	}

	struct temporary_memory TempArena = BeginTemporaryArena(Arena);
	struct bitmap *TempBitmap = CreateEmptyBitmap(Arena, BitmapWidth, BitmapHeight);
	ClearMemory(TempBitmap->BitmapMemory, (TempBitmap->Width * TempBitmap->Height) * TempBitmap->BytesPerPixel);
	uint8 Character = 0;
	while((Character = *String++) != 0)
	{
		int32 Advance = 0;
		int32 LSB = 0;
		int32 X0, Y0;
		int32 X1, Y1;
		real32 XShift = XPos - (int32)(XPos - 0.5f);

		stbtt_GetCodepointHMetrics(&Font, Character, &Advance, &LSB);
		stbtt_GetCodepointBitmapBox(&Font, Character, Scale, Scale, &X0, &Y0, &X1, &Y1);
		stbtt_MakeCodepointBitmap(&Font,
								  ((uint8 *)TempBitmap->BitmapMemory) + ((Baseline + Y0)) + (((int32)XPos + X0)),
								  X1-X0, Y1-Y0, TempBitmap->Width, Scale, Scale, Character);
		XPos += (Advance * Scale);
		if(*String)
		{
			XPos += Scale * stbtt_GetCodepointKernAdvance(&Font, Character, *String);
		}
	}

	uint8 *Source = (uint8 *)TempBitmap->BitmapMemory;
	uint8 *DestRow = (uint8 *)Result->BitmapMemory;// + (TempBitmap->Height - 1) * Result->Pitch;
	for(uint32 Y = 0; Y < TempBitmap->Height; ++Y)
	{
		uint32 *Dest = (uint32 *)DestRow;
		for(uint32 X = 0; X < TempBitmap->Width; ++X)
		{
			uint8 Alpha = *Source++;
			*Dest++ = ((Alpha << 24) |
					   (Alpha << 16) |
					   (Alpha << 8) |
					   (Alpha << 0));
		}

		DestRow += TempBitmap->Pitch;
	}
	EndTemporaryArena(TempArena);

	return(Result);
#else
	stbtt_fontinfo Font;
	stbtt_InitFont(&Font, GameState->FontData.FileContents, stbtt_GetFontOffsetForIndex(GameState->FontData.FileContents, 0));
	Assert(Font.numGlyphs == 4237);

	int32 Width;
	int32 Height;
	int32 XOffset;
	int32 YOffset;
	uint8 *MonoBitmap = stbtt_GetCodepointBitmap(&Font, 0, stbtt_ScaleForPixelHeight(&Font, 32.0f),
												 'B', &Width, &Height, &XOffset, &YOffset);

	struct bitmap *Result = CreateEmptyBitmap(Arena, Width, Height);

	uint8 *Source = MonoBitmap;
	uint8 *DestRow = (uint8 *)Result->BitmapMemory;
	for(uint32 Y = 0; Y < Height; ++Y)
	{
		uint32 *Dest = (uint32 *)DestRow;
		for(uint32 X = 0; X < Width; ++X)
		{
			uint8 Alpha = *Source++;
			*Dest++ = ((Alpha << 24) |
					   (Alpha << 16) |
					   (Alpha << 8) |
					   (Alpha << 0));
		}

		DestRow += Result->Pitch;
	}

	stbtt_FreeBitmap(MonoBitmap, 0);

	return(Result);
#endif
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	struct game_state *GameState = (game_state *)Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(struct game_state),
						(uint8 *)Memory->PermanentStorage + sizeof(struct game_state));
		InitializeArena(&GameState->TransArena, Memory->TransientStorageSize, Memory->TransientStorage);
		GameState->Gravity = V2(0.0f, 800.0f);
		GameState->PlayerEntity.Position = 0.5f * V2(Buffer->Width / 2.0f, Buffer->Height / 2.0f);
		GameState->PlayerEntity.Velocity = {};
		GameState->PixelsPerMeter = 42.0f;
		GameState->MetersToPixels = 1.0f / GameState->PixelsPerMeter;
		GameState->TileSideInPixels = 24.0f;

		GameState->PlayerEntity.Size = GameState->TileSideInPixels * V2(1.0f, 1.0f);

		GameState->World = PushStruct(&GameState->WorldArena, struct world);
		GameState->World->RoomHeightInGridCells = 16;
		GameState->World->RoomWidthInGridCells = Buffer->Width / GameState->TileSideInPixels;

		GameState->ColorSchemeLight.Color1 = V3(0x2c, 0x3e, 0x50);
		GameState->ColorSchemeLight.Color2 = V3(0xe7, 0x4c, 0x3c);
		GameState->ColorSchemeLight.Color3 = V3(0x34, 0x98, 0xdb);
		GameState->ColorSchemeLight.Color4 = V3(0xff, 0x35, 0x8b);
		GameState->ColorSchemeLight.Color5 = V3(0xae, 0xee, 0x00);
		GameState->ColorSchemeLight.Color6 = V3(0xff, 0x38, 0x00);
		GameState->ColorSchemeLight.PlayerColor = V3(0x1c, 0x1d, 0x21);
		GameState->ColorSchemeLight.BackgroundColor = V3(0xec, 0xf0, 0xf1);

		GameState->ColorSchemeDark.Color1 = V3(0xd9, 0x00, 0x00);
		GameState->ColorSchemeDark.Color2 = V3(0xff, 0x2d, 0x00);
		GameState->ColorSchemeDark.Color3 = V3(0xff, 0x8c, 0x00);
		GameState->ColorSchemeDark.Color4 = V3(0x04, 0x75, 0x6f);
		GameState->ColorSchemeDark.Color5 = V3(0xff, 0xd4, 0x62);
		GameState->ColorSchemeDark.Color6 = V3(0xc0, 0xcf, 0x3a);
		GameState->ColorSchemeDark.PlayerColor = V3(0xf2, 0xf1, 0xe9);
		GameState->ColorSchemeDark.BackgroundColor = V3(0x1d, 0x1e, 0x20);

		GameState->Colors = &GameState->ColorSchemeDark;

		uint32 RoomDataSize = GameState->World->RoomHeightInGridCells * GameState->World->RoomWidthInGridCells;
		for(uint32 RoomIndex = 0; RoomIndex < ArrayCount(GameState->World->Rooms); ++RoomIndex)
		{
			struct room_chunk *Room = (GameState->World->Rooms + RoomIndex);
			Room->RoomData = PushArray(&GameState->WorldArena, RoomDataSize, uint32);
			GenerateRoom(GameState, Room, GameState->World->RoomWidthInGridCells, GameState->World->RoomHeightInGridCells);
			Room->RoomPosition.Y = -((GameState->World->RoomHeightInGridCells * GameState->TileSideInPixels) * RoomIndex);
		}

		GameState->WorldShiftHeight = Buffer->Height * 0.3f;
		GameState->CameraFollowingPlayer = false;
		GameState->EnableDebugMouse = false;

		GameState->Score = 0;
		snprintf((char *)&GameState->ScoreAsString[0][0], ArrayCount(GameState->ScoreAsString[0]), "%d", GameState->Score);
		snprintf((char *)&GameState->ScoreAsString[1][0], ArrayCount(GameState->ScoreAsString[1]), "%d", GameState->Score);
		GameState->CurrentScoreString = GameState->ScoreAsString[0];
		GameState->PreviousScoreString = GameState->ScoreAsString[1];

		if(GameState->FontData.FileSize == 0)
		{
			GameState->FontData = Memory->PlatformReadEntireFileIntoMemory("arial.ttf");
			Assert(GameState->FontData.FileSize != 0);
			Assert(GameState->FontData.FileContents != 0);
		}
		GameState->Text = TextToBitmap(&GameState->TransArena, GameState, 32.0f, "BESTIE!!");
		GameState->ScoreBitmap = TextToBitmap(&GameState->TransArena, GameState, 64.0f, (char *)GameState->CurrentScoreString, 0, 256, 48);

		Memory->IsInitialized = true;
	}

	if(Input->ButtonDown.EndedDown)
	{
	}
	if(Input->ButtonLeft.Tapped)
	{
		GameState->PlayerEntity.Velocity = V2(-60.0f, -460.0f);
	}
	if(Input->ButtonRight.Tapped)
	{
		GameState->PlayerEntity.Velocity = V2(60.0f, -460.0f);
	}
#ifdef BLOCKGAME_DEBUG
	if(Input->ButtonUp.EndedDown)
	{
		GameState->PlayerEntity.Velocity = V2(0.0f, -820.0f);
	}
	if(Input->ButtonDebugColors.Tapped)
	{
		if(GameState->Colors == &GameState->ColorSchemeLight)
		{
			GameState->Colors = &GameState->ColorSchemeDark;
		}
		else
		{
			GameState->Colors = &GameState->ColorSchemeLight;
		}
	}
	if(Input->ButtonDebugMouse.Tapped)
	{
		GameState->EnableDebugMouse = !GameState->EnableDebugMouse;
	}
	if(Input->ButtonReset.Tapped)
	{
		Memory->IsInitialized = false;
	}
#endif

	GameState->PlayerEntity.Position = GameState->PlayerEntity.Position + (GameState->PlayerEntity.Velocity * Input->dtForFrame);
	GameState->PlayerEntity.Velocity = GameState->PlayerEntity.Velocity + (GameState->Gravity * Input->dtForFrame);
	// TODO(rick): Perhaps we can extract this out into a function, possibly
	// combining this with the collision detection when it's added. Not
	// required, but a probably nice to have.
	if((GameState->PlayerEntity.Position.X - (0.5f * GameState->TileSideInPixels) <= 0.0f) ||
	   (GameState->PlayerEntity.Position.X + (0.5f * GameState->TileSideInPixels) >= Buffer->Width))
	{
		GameState->PlayerEntity.Velocity.X = 0.0f;
		GameState->PlayerEntity.Velocity.Y = 0.7f * GameState->PlayerEntity.Velocity.Y;
		GameState->PlayerEntity.Velocity = GameState->PlayerEntity.Velocity + (GameState->Gravity * Input->dtForFrame);
		GameState->PlayerEntity.Position = GameState->PlayerEntity.Position + (GameState->PlayerEntity.Velocity * Input->dtForFrame);

		if(GameState->PlayerEntity.Position.X < Buffer->Width / 2.0f)
		{
			GameState->PlayerEntity.Position.X = 0.0f + (0.5f * GameState->TileSideInPixels);
		}
		else
		{
			GameState->PlayerEntity.Position.X = Buffer->Width - (0.5f * GameState->TileSideInPixels);
		}
	}
	if(GameState->EnableDebugMouse)
	{
		GameState->PlayerEntity.Position.X = Input->MouseX;
		GameState->PlayerEntity.Position.Y = Input->MouseY;
		GameState->PlayerEntity.Velocity = V2(0.0f, 0.0f);
	}

	if((GameState->PlayerEntity.Position.Y <= GameState->WorldShiftHeight) ||
	   (GameState->CameraFollowingPlayer))
	{
		GameState->PlayerEntity.Position.Y = GameState->WorldShiftHeight;
		for(uint32 RoomIndex = 0;
			RoomIndex < ArrayCount(GameState->World->Rooms);
			++RoomIndex)
		{
			struct room_chunk *Room = GameState->World->Rooms + RoomIndex;
			Room->RoomPosition = Room->RoomPosition + ((-1.0f * GameState->PlayerEntity.Velocity) * Input->dtForFrame);
			Room->RoomPosition.X = 0.0f;
		}
		for(uint32 RoomIndex = 0;
			RoomIndex < ArrayCount(GameState->World->Rooms);
			++RoomIndex)
		{
			struct room_chunk *Room = GameState->World->Rooms + RoomIndex;
			if(Room->RoomPosition.Y > Buffer->Height + (GameState->World->RoomHeightInGridCells * GameState->TileSideInPixels))
			{
				RecycleRoom(GameState, Room);
			}
		}
	}

	// NOTE(rick): Room collision for scoring... Maybe all collision detection
	for(uint32 RoomIndex = 0;
		RoomIndex < ArrayCount(GameState->World->Rooms);
		++RoomIndex)
	{
		struct room_chunk *Room = GameState->World->Rooms + RoomIndex;
		bool32 Collided = TestCollision(GameState->PlayerEntity.Position, GameState->PlayerEntity.Size,
										Room->RoomPosition, Room->RoomDims);
		if(Collided == true)
		{
			if(!Room->PlayerVisited)
			{
				Room->PlayerVisited = true;
				++GameState->Score;

				uint8 *TempScoreString = GameState->CurrentScoreString;
				GameState->CurrentScoreString = GameState->PreviousScoreString;
				GameState->PreviousScoreString = TempScoreString;
				snprintf((char *)GameState->CurrentScoreString, ArrayCount(GameState->CurrentScoreString), "%u", GameState->Score);

				GameState->ScoreBitmap = TextToBitmap(&GameState->WorldArena, GameState, 64.0f,
													  (char *)GameState->CurrentScoreString, GameState->ScoreBitmap);

				// TODO(rick): Compare CurrentScoreString to PreviousScoreString
				// to determine which character indexes have changed. Whichever
				// have changed then we should recreate the character bitmap.
				// TODO(rick): We need to think about this a bit, obviously only
				// creating a few bitmaps at a time is better than all at once,
				// but maybe just creating the score bitmap as one bitmap is
				// better.. definitely easier, plus we get proper kerning.
			}
			uint32 *CellData = Room->RoomData;
			for(uint32 Y = 0; Y < GameState->World->RoomHeightInGridCells; ++Y)
			{
				for(uint32 X = 0; X < GameState->World->RoomWidthInGridCells; ++X)
				{
					if(*CellData == 1)
					{
						v2 CellDimensions = GameState->TileSideInPixels * V2(1.0f, 1.0f);
						v2 CellPosition = V2(Room->RoomPosition.X + (GameState->TileSideInPixels * X),
											 Room->RoomPosition.Y + (GameState->TileSideInPixels * Y));
						bool32 Collided = TestCollision(GameState->PlayerEntity.Position, GameState->PlayerEntity.Size,
														CellPosition, CellDimensions);
						if(Collided)
						{
							Room->RoomColor = V3(0x00, 0x00, 0x00);
							GameState->PlayerEntity.Velocity = V2(0.0f, 0.0f);
							GameState->Gravity = V2(0.0f, 0.0f);
						}
					}
					++CellData;
				}
			}
		}
	}

	ClearScreenToColor(Buffer, GameState->Colors->BackgroundColor);
	for(uint32 RoomIndex = 0;
		RoomIndex < ArrayCount(GameState->World->Rooms);
		++RoomIndex)
	{
		v2 BlockDims = GameState->TileSideInPixels * V2(1.0f, 1.0f);
		struct room_chunk *Room = GameState->World->Rooms + RoomIndex;
		uint32 *RoomData = Room->RoomData;
		for(uint32 Y = 0; Y < GameState->World->RoomHeightInGridCells; ++Y)
		{
			for(uint32 X = 0; X < GameState->World->RoomWidthInGridCells; ++X)
			{
				if(*RoomData == 1)
				{
					v2 BlockPosition = Room->RoomPosition + (GameState->TileSideInPixels * V2(X, Y));
					DrawRectangle(Buffer, BlockPosition, BlockDims, Room->RoomColor);
				}
				++RoomData;
			}
		}
	}
	
	DrawRectangle(Buffer, GameState->PlayerEntity.Position - (0.5f * GameState->PlayerEntity.Size),
				  GameState->PlayerEntity.Size, GameState->Colors->PlayerColor);

	DrawTextBitmap(Buffer, GameState->PlayerEntity.Position + V2(10.0f, 25.0f), GameState->Text, V4(0x00, 0xff, 0xff, 1.00f));
	DrawTextBitmap(Buffer, V2(10, 10), GameState->ScoreBitmap, V4(GameState->Colors->PlayerColor.R,
																  GameState->Colors->PlayerColor.G,
																  GameState->Colors->PlayerColor.B,
																  0.80f));

	//RenderDebugGrid(Buffer, V2(GameState->TileSideInPixels, GameState->TileSideInPixels));
}

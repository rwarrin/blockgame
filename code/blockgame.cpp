/**
 * TODO List
 * * Random Room coloring
 * * Game Keys
 *   * Pause etc..
 * * Collision Detection
 * * TEXT RENDERING
 *   * Scoring
 *   * Menus
 *   * Scoreboard? Local or Online
 * * Sound?
 * * Color Schemes / Better graphics
 * * Performance (Smooth 60fps)
 */

static void
ClearScreenToColor(struct game_screen_buffer *Buffer, uint32 R, uint32 G, uint32 B)
{
	uint32 Color = ( (0xff << 24) | (R << 16) | (G << 8) | (B << 0) );

	uint8 *Base = (uint8*)Buffer->BitmapMemory;
	for(int32 Y = 0; Y < Buffer->Height; ++Y)
	{
		uint32 *Pixel = (uint32 *)(Base + (Y * Buffer->Pitch));
		for(int32 X = 0; X < Buffer->Width; ++X)
		{
			*Pixel++ = Color;
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
GenerateRoom(struct room_chunk *Room, uint32 RoomWidth, uint32 RoomHeight)
{
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

	GenerateRoom(Room, GameState->World->RoomWidthInGridCells, GameState->World->RoomHeightInGridCells);
	Room->RoomPosition.Y = HighestRoom->RoomPosition.Y - (GameState->TileSideInPixels * GameState->World->RoomHeightInGridCells);
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	struct game_state *GameState = (game_state *)Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(struct game_state),
						(uint8 *)Memory->PermanentStorage + sizeof(struct game_state));
		GameState->Gravity = V2(0.0f, 800.0f);
		GameState->PlayerEntity.Position = 0.5f * V2(Buffer->Width / 2.0f, Buffer->Height / 2.0f);
		GameState->PlayerEntity.Velocity = {};

		GameState->PixelsPerMeter = 42.0f;
		GameState->MetersToPixels = 1.0f / GameState->PixelsPerMeter;
		GameState->TileSideInPixels = 24.0f;

		GameState->World = PushStruct(&GameState->WorldArena, struct world);
		GameState->World->RoomHeightInGridCells = 16;
		GameState->World->RoomWidthInGridCells = Buffer->Width / GameState->TileSideInPixels;

		uint32 RoomDataSize = GameState->World->RoomHeightInGridCells * GameState->World->RoomWidthInGridCells;
		for(uint32 RoomIndex = 0; RoomIndex < ArrayCount(GameState->World->Rooms); ++RoomIndex)
		{
			struct room_chunk *Room = (GameState->World->Rooms + RoomIndex);
			Room->RoomData = PushArray(&GameState->WorldArena, RoomDataSize, uint32);
			GenerateRoom(Room, GameState->World->RoomWidthInGridCells, GameState->World->RoomHeightInGridCells);
			Room->RoomPosition.Y = -((GameState->World->RoomHeightInGridCells * GameState->TileSideInPixels) * RoomIndex);
		}

		GameState->WorldShiftHeight = Buffer->Height * 0.3f;
		GameState->CameraFollowingPlayer = false;

		Memory->IsInitialized = true;
	}

	if(Input->ButtonUp.EndedDown)
	{
		OutputDebugString("UP is pressed\n");
		GameState->PlayerEntity.Velocity = V2(0.0f, -820.0f);
	}
	if(Input->ButtonDown.EndedDown)
	{
		OutputDebugString("DOWN is pressed\n");
	}
	if(Input->ButtonLeft.Tapped)
	{
		OutputDebugString("LEFT is pressed\n");
		GameState->PlayerEntity.Velocity = V2(-60.0f, -420.0f);
	}
	if(Input->ButtonRight.Tapped)
	{
		OutputDebugString("RIGHT is pressed\n");
		GameState->PlayerEntity.Velocity = V2(60.0f, -420.0f);
	}

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

	ClearScreenToColor(Buffer, 0xff, 0xff, 0xff);
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
					DrawRectangle(Buffer, BlockPosition, BlockDims, V3(0x33, 0xaa, 0xff));
				}
				++RoomData;
			}
		}
	}
	v2 RectSize = GameState->TileSideInPixels * V2(1.0f, 1.0f);
	v2 RectPos = GameState->PlayerEntity.Position - (0.5f * RectSize);
	DrawRectangle(Buffer, RectPos, RectSize, V3(0x00, 0x00, 0x00));

	//RenderDebugGrid(Buffer, V2(GameState->TileSideInPixels, GameState->TileSideInPixels));
}

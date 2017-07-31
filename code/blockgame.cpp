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
	if(!Room->IsActiveRoom)
	{
		Room->IsActiveRoom = true;
		Room->RoomShift = V2(0.0f, 0.0f);
		uint32 Gap = RandomInt32() % RoomWidth;
		uint32 *CellData = Room->RoomData;
		for(uint32 Y = 0; Y < RoomHeight; ++Y)
		{
			for(uint32 X = 0; X < RoomWidth; ++X)
			{
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
				if(X == 0 || X == RoomWidth - 1)
				{
					*CellData = 1;
				}
				++CellData;
			}
		}
	}
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
		}

		GameState->WorldShiftHeight = Buffer->Height * 0.4f;
		GameState->CameraFollowingPlayer = false;

		Memory->IsInitialized = true;
	}


	if(Input->ButtonUp.EndedDown)
	{
		OutputDebugString("UP is pressed\n");
	}
	if(Input->ButtonDown.EndedDown)
	{
		OutputDebugString("DOWN is pressed\n");
	}
	if(Input->ButtonLeft.EndedDown)
	{
		OutputDebugString("LEFT is pressed\n");
		GameState->PlayerEntity.Velocity = V2(-50.0f, -380.0f);
	}
	if(Input->ButtonRight.EndedDown)
	{
		OutputDebugString("RIGHT is pressed\n");
		GameState->PlayerEntity.Velocity = V2(50.0f, -380.0f);
	}

	GameState->PlayerEntity.Position = GameState->PlayerEntity.Position + (GameState->PlayerEntity.Velocity * Input->dtForFrame);
	GameState->PlayerEntity.Velocity = GameState->PlayerEntity.Velocity + (GameState->Gravity * Input->dtForFrame);
	if((GameState->PlayerEntity.Position.Y <= GameState->WorldShiftHeight) ||
	   (GameState->CameraFollowingPlayer))
	{
		GameState->PlayerEntity.Position.Y = GameState->WorldShiftHeight;
		for(uint32 RoomIndex = 0;
			RoomIndex < ArrayCount(GameState->World->Rooms);
			++RoomIndex)
		{
			struct room_chunk *Room = GameState->World->Rooms + RoomIndex;
			if(Room->IsActiveRoom)
			{
				Room->RoomShift = Room->RoomShift + ((-1.0f * GameState->PlayerEntity.Velocity) * Input->dtForFrame);
				Room->RoomShift.X = 0.0f;
			}
		}
	}

	ClearScreenToColor(Buffer, 0x33, 0xaa, 0xff);
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
					v2 BlockPosition = (GameState->TileSideInPixels * V2(X, Y)) +
						V2(0.0f, RoomIndex * (GameState->TileSideInPixels * GameState->World->RoomHeightInGridCells)) +
						Room->RoomShift;
					DrawRectangle(Buffer, BlockPosition, BlockDims, V3(0xff, 0x33, 0x33));
				}
				++RoomData;
			}
		}
	}
	v2 RectSize = GameState->TileSideInPixels * V2(1.0f, 1.0f);
	v2 RectPos = GameState->PlayerEntity.Position - (0.5f * RectSize);
	DrawRectangle(Buffer, RectPos, RectSize, V3(0xff, 0xff, 0xff));

	RenderDebugGrid(Buffer, V2(GameState->TileSideInPixels, GameState->TileSideInPixels));
}

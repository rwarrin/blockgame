inline real32
Abs(real32 Val)
{
	real32 Result = Val;
	if(Result < 0)
	{
		Result = -Result;
	}

	return(Result);
}


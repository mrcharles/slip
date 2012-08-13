// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "..\slip\slip.h"
#include "time.h"

DECLARESLIPTAG(TestTag);
DECLARESLIPTAG(TestTag2);


int _tmain(int argc, _TCHAR* argv[])
{

	slip::enable();

	slip::begin(SLIP_TestTag);

	Sleep(1000);

	for(int i = 0; i < 10; i++)
	{
		slip::scoper s(SLIP_TestTag2);
		Sleep(10);
	}

	slip::end(SLIP_TestTag);

	slip::begin(SLIP_TestTag2);

	Sleep(25);

	slip::end(SLIP_TestTag2);

	slip::checkpoint();
	slip::disable();
	return 0;
}


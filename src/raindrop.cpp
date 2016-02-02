#include "pch.h"

#include "Screen.h"
#include "Application.h"

int main (int argc, char *argv[])
{
	Application App(argc, argv);
	App.Init();
	App.Run();
	App.Close();
	return 0;
}

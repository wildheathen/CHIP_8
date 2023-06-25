#include "chip8.h"


int main(int argc, char** argv)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
	{
		SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
		return -1;
	}

	chipotto::Emulator emulator;

	if (emulator.IsValid())
	{
		//emulator.LoadFromFile("C:\\Users\\david\\Documents\\AIV_3\\C++\\CHIP_8\\CHIP_8\\rsc\\TICTAC");
		emulator.LoadFromFile("C:\\Users\\david\\Documents\\AIV_3\\C++\\CHIP_8\\CHIP_8\\rsc\\PONG");
		while (true)
		{
			
			if (!emulator.Tick(SDL_GetTicks()))
			{
				break;
			}
		}
	}

	SDL_Quit();
	return 0;
}
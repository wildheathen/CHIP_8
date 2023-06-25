#include "chip8.h"

namespace chipotto
{
	int Emulator::key_map(unsigned int key) {
		switch (key) {
		case SDLK_1:
			return 0x1;
		case SDLK_2:
			return 0x2;
		case SDLK_3:
			return 0x4;
		case SDLK_4:
			return 0xC;
		case SDLK_q:
			return 0x4;
		case SDLK_w:
			return 0x5;
		case SDLK_e:
			return 0x6;
		case SDLK_r:
			return 0xD;
		case SDLK_a:
			return 0x7;
		case SDLK_s:
			return 0x8;
		case SDLK_d:
			return 0x9;
		case SDLK_f:
			return 0xE;
		case SDLK_z:
			return 0xA;
		case SDLK_x:
			return 0x0;
		case SDLK_c:
			return 0xB;
		case SDLK_v:
			return 0xF;
		default: //Invalid key
			return -1;
		}
	}

	bool Emulator::LoadFromFile(std::filesystem::path Path)
	{
		std::ifstream file;
		file.open(Path, std::ios::binary);
		if (!file.is_open()) return false;

		auto file_size = std::filesystem::file_size(Path);

		file.read(reinterpret_cast<char*>(MemoryMapping.data() + PC), file_size);
		file.close();
	}
	bool Emulator::Tick(Uint32 last_tick)
	{
		uint64_t tick = SDL_GetTicks64();

		if (DelayTimer > 0 && tick >= DeltaTimerTicks)
		{
			DelayTimer--;
			DeltaTimerTicks = 17 + SDL_GetTicks64();
		}

		//auto frame_start = SDL_GetTicks();


		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_KEYDOWN)
			{
				SDL_Keycode key = event.key.keysym.sym;			

				if (keymap.contains(key))
				{
					keys[key_map(key)] = 1;
					
					if (WaitingForKey)
					{
						Registers[WaitForKeyboardRegister_Index] = keymap[key];
						WaitingForKey = false;

						PC += 2;
					}
				}
			}
			if (event.type == SDL_KEYUP)
			{
				SDL_Keycode key = event.key.keysym.sym;
				
				if (keymap.contains(key))
					keys[key_map(key)] = 0;

			}
			if (event.type == SDL_QUIT)
			{
				return false;
			}
		}
		SDL_PumpEvents();

		if (WaitingForKey) return true;

		//if (SDL_GetTicks() - last_tick > 16) {
		//	//chip8.tickTimers();
		//	last_tick = SDL_GetTicks();
		//}

		uint16_t opcode = MemoryMapping[PC + 1] + (static_cast<uint16_t>(MemoryMapping[PC]) << 8);
		std::cout << std::hex << "0x" << PC << ": 0x" << opcode << "  -->  ";

		OpcodeStatus status = Opcodes[opcode >> 12](opcode);

		std::cout << std::endl;
		if (status == OpcodeStatus::IncrementPC)
		{
			PC += 2;
		}

		////TODO run with the option to speed up or slow down
		//auto frame_end = SDL_GetTicks();
		//if (frame_end - frame_start < 1000 / SPEED) //If running faster than desired speed, add delay
		//	SDL_Delay(1000 / SPEED - (frame_end - frame_start)); //run at 500Hz by default

		//SDL_Delay(20);
		return status != OpcodeStatus::NotImplemented && status != OpcodeStatus::StackOverflow && status != OpcodeStatus::Error;
	}
	bool Emulator::IsValid() const
	{
		if (!Window || !Renderer || !Texture)
			return false;
		return true;
	}
	OpcodeStatus Emulator::Opcode0(const uint16_t opcode)
	{
		if ((opcode & 0xFF) == 0xE0)
		{
			std::cout << "CLS";
			uint8_t* pixels = nullptr;
			int pitch;
			int result = SDL_LockTexture(Texture, nullptr, reinterpret_cast<void**>(&pixels), &pitch);
			memset(pixels, 0, pitch * height);
			SDL_UnlockTexture(Texture);
			SDL_RenderCopy(Renderer, Texture, nullptr, nullptr);
			SDL_RenderPresent(Renderer);
			return OpcodeStatus::IncrementPC;
		}
		else if ((opcode & 0xFF) == 0xEE)
		{
			if (SP > 0xF && SP < 0xFF) return OpcodeStatus::StackOverflow;
			std::cout << "RET";
			PC = Stack[SP & 0xF];
			SP -= 1;
			return OpcodeStatus::IncrementPC;
		}
		return OpcodeStatus::NotImplemented;
	}

	OpcodeStatus Emulator::Opcode1(const uint16_t opcode)
	{
		uint16_t address = opcode & 0x0FFF;
		std::cout << "JP 0x" << address;
		PC = address; //fixed
		return OpcodeStatus::NotIncrementPC;
	}

	OpcodeStatus Emulator::Opcode2(const uint16_t opcode)
	{
		uint16_t address = opcode & 0xFFF;
		std::cout << "CALL 0x" << (int)address;
		if (SP > 0xF)
		{
			SP = 0;
		}
		else
		{
			if (SP < 0xF)
			{
				SP += 1;
			}
			else
			{
				return OpcodeStatus::StackOverflow;
			}
		}
		Stack[SP] = PC;
		PC = address;
		return OpcodeStatus::NotIncrementPC;
	}

	OpcodeStatus Emulator::Opcode3(const uint16_t opcode)
	{
		uint8_t register_index = (opcode >> 8) & 0xF;
		uint8_t value = opcode & 0xFF;
		std::cout << "SE V" << (int)register_index << ", 0x" << (int)value;
		if (Registers[OPCODE_X(opcode)] == OPCODE_KK(opcode))
			PC += 2;
		return OpcodeStatus::IncrementPC;
	}

	OpcodeStatus Emulator::Opcode4(const uint16_t opcode)
	{
		uint8_t register_index = (opcode >> 8) & 0xF;
		uint8_t value = opcode & 0xFF;
		std::cout << "SNE V" << (int)register_index << ", 0x" << (int)value;
		if (Registers[OPCODE_X(opcode)] != OPCODE_KK(opcode))
			PC += 2;
		return OpcodeStatus::IncrementPC;
	}

	OpcodeStatus Emulator::Opcode5(const uint16_t opcode)
	{
		return OpcodeStatus::NotImplemented;
	}

	OpcodeStatus Emulator::Opcode6(const uint16_t opcode)
	{
		uint8_t register_index = (opcode >> 8) & 0xF;
		uint8_t register_value = opcode & 0xFF;
		Registers[register_index] = register_value;
		std::cout << "LD V" << (int)register_index << ", 0x" << (int)register_value;
		return OpcodeStatus::IncrementPC;
	}

	OpcodeStatus Emulator::Opcode7(const uint16_t opcode)
	{
		uint8_t register_index = OPCODE_X(opcode);
		uint8_t value = OPCODE_KK(opcode);
		std::cout << "ADD V" << (int)register_index << ", 0x" << (int)value;
		Registers[register_index] += value;
		return OpcodeStatus::IncrementPC;
	}

	OpcodeStatus Emulator::Opcode8(const uint16_t opcode)
	{
		if ((opcode & 0xF) == 0x0)
		{
			uint8_t registerX_index = (opcode >> 8) & 0xF;
			uint8_t registerY_index = (opcode >> 4) & 0xF;
			Registers[registerX_index] = Registers[registerY_index];
			std::cout << "LD V" << (int)registerX_index << ", V" << (int)registerY_index;
			return OpcodeStatus::IncrementPC;
		}
		else if ((opcode & 0xF) == 0x2)
		{
			uint8_t registerX_index = (opcode >> 8) & 0xF;
			uint8_t registerY_index = (opcode >> 4) & 0xF;
			Registers[registerX_index] &= Registers[registerY_index];
			std::cout << "AND V" << (int)registerX_index << ", V" << (int)registerY_index;
			return OpcodeStatus::IncrementPC;
		}
		else if ((opcode & 0xF) == 0x3)
		{
			uint8_t registerX_index = (opcode >> 8) & 0xF;
			uint8_t registerY_index = (opcode >> 4) & 0xF;
			Registers[registerX_index] ^= Registers[registerY_index];
			std::cout << "XOR V" << (int)registerX_index << ", V" << (int)registerY_index;
			return OpcodeStatus::IncrementPC;
		}
		else if ((opcode & 0xF) == 0x4)
		{
			uint8_t registerX_index = (opcode >> 8) & 0xF;
			uint8_t registerY_index = (opcode >> 4) & 0xF;
			int result = static_cast<int>(Registers[registerX_index]) + Registers[registerY_index];
			if (result > 255) Registers[0xF] = 1;
			else Registers[0xF] = 0;
			Registers[registerX_index] += Registers[registerY_index];
			std::cout << "ADD V" << (int)registerX_index << ", V" << (int)registerY_index;
			return OpcodeStatus::IncrementPC;
		}
		else if ((opcode & 0xF) == 0x5) //SUB Vx, Vy 0x80D5
		{
			uint8_t registerX_index = (opcode >> 8) & 0xF;
			uint8_t registerY_index = (opcode >> 4) & 0xF;

			if (Registers[registerX_index] > Registers[registerY_index])
				Registers[0xF] = 1;
			else
				Registers[0xF] = 0;

			Registers[registerX_index] -= Registers[registerY_index];
			std::cout << "SUB V" << (int)registerX_index << ", V" << (int)registerY_index;
			return OpcodeStatus::IncrementPC;
		}
		else if ((opcode & 0xF) == 0xE)
		{
			uint8_t registerX_index = (opcode >> 8) & 0xF;
			uint8_t registerY_index = (opcode >> 4) & 0xF;
			Registers[0xF] = Registers[registerX_index] >> 7;
			Registers[registerX_index] <<= 1;
			std::cout << "SHL V" << (int)registerX_index << "{, V" << (int)registerY_index << "}";
			return OpcodeStatus::IncrementPC;
		}
		else
		{
			return OpcodeStatus::NotImplemented;
		}
	}

	OpcodeStatus Emulator::Opcode9(const uint16_t opcode)
	{
		return OpcodeStatus::NotImplemented;
	}

	OpcodeStatus Emulator::OpcodeA(const uint16_t opcode)
	{
		uint16_t value = (opcode & 0xFFF);
		std::cout << "LD I, 0x" << (int)value;
		I = value;
		return OpcodeStatus::IncrementPC;
	}

	OpcodeStatus Emulator::OpcodeB(const uint16_t opcode)
	{
		return OpcodeStatus::NotImplemented;
	}

	OpcodeStatus Emulator::OpcodeC(const uint16_t opcode)
	{
		uint8_t register_index = (opcode >> 8) & 0xF;
		uint8_t random_mask = opcode & 0xFF;
		std::cout << "RND V" << (int)register_index << ", 0x" << (int)random_mask;
		Registers[register_index] = (std::rand() % 256) & random_mask;
		return OpcodeStatus::IncrementPC;
	}

	OpcodeStatus Emulator::OpcodeD(const uint16_t opcode)
	{
		uint8_t registerX_index = (opcode >> 8) & 0xF;
		uint8_t registerY_index = (opcode >> 4) & 0xF;
		uint8_t sprite_height = opcode & 0xF;
		std::cout << "DRW V" << (int)registerX_index << ", V" << (int)registerY_index << ", " << (int)sprite_height;

		uint8_t x_coord = Registers[registerX_index] % width;
		uint8_t y_coord = Registers[registerY_index] % height;

		uint8_t* pixels = nullptr;
		int pitch;
		int result = SDL_LockTexture(Texture, nullptr, reinterpret_cast<void**>(&pixels), &pitch);
		if (result != 0)
		{
			SDL_Log("Failed to lock texture");
			return OpcodeStatus::Error;
		}

		for (int y = 0; y < sprite_height; ++y)
		{
			if (y + y_coord >= height) break;
			uint8_t row_byte = MemoryMapping[I + y];
			for (int x = 0; x < 8; x++)
			{
				uint8_t pixel_color = (row_byte >> (7 - x)) & 0x1;
				uint8_t color = 0;
				if (pixel_color)
				{
					color = 0xFF;
				}
				if (x + x_coord >= width) break;
				int pixel_index = (x + x_coord) * 4 + pitch * (y + y_coord);
				uint8_t existing_pixel = pixels[pixel_index];
				color ^= existing_pixel;

				if (existing_pixel != 0 && color != 0)
				{
					Registers[0xF] = 0x1;
				}

				pixels[pixel_index + 0] = color;
				pixels[pixel_index + 1] = color;
				pixels[pixel_index + 2] = color;
				pixels[pixel_index + 3] = color;
			}
		}

		SDL_UnlockTexture(Texture);

		SDL_RenderCopy(Renderer, Texture, nullptr, nullptr);
		SDL_RenderPresent(Renderer);

		return OpcodeStatus::IncrementPC;
	}

	OpcodeStatus Emulator::OpcodeE(const uint16_t opcode)
	{
		char key = Registers[OPCODE_X(opcode)];

		if ((opcode & 0xFF) == 0x9E)/* EX9E: SKP - Skip next instruction if key V[X] is down. */
		{
			uint8_t register_index = (opcode >> 8) & 0xF;
			std::cout << "SKP V" << (int)register_index;

			if (keys[key] != 0)
			{
				PC += 2;
			}
			return OpcodeStatus::IncrementPC;
		}
		else if ((opcode & 0xFF) == 0xA1) /* EXA1: SKNP - Skip next instruction if key V[X] is not down. */
		{
			uint8_t register_index = (opcode >> 8) & 0xF;
			
			std::cout << "SKNP V" << (int)register_index;

			if (keys[key] == 0)
			{
				PC += 2;
			}
			return OpcodeStatus::IncrementPC;
		}
		return OpcodeStatus::NotImplemented;
	}

	OpcodeStatus Emulator::OpcodeF(const uint16_t opcode)
	{
		if ((opcode & 0xFF) == 0x0A)
		{
			uint8_t register_index = (opcode >> 8) & 0xF;
			std::cout << "LD V" << (int)register_index << ", K";

			WaitForKeyboardRegister_Index = register_index;
			WaitingForKey = true;
			return OpcodeStatus::WaitForKeyboard;
		}
		else if ((opcode & 0xFF) == 0x07)
		{
			uint8_t register_index = (opcode >> 8) & 0xF;
			std::cout << "LD V" << (int)register_index << ", DT";
			Registers[register_index] = DelayTimer;
			return OpcodeStatus::IncrementPC;
		}
		else if ((opcode & 0xFF) == 0x15)
		{
			uint8_t register_index = (opcode >> 8) & 0xF;
			std::cout << "LD DT, V" << (int)register_index;
			DelayTimer = Registers[register_index];
			DeltaTimerTicks = 17 + SDL_GetTicks64();
			return OpcodeStatus::IncrementPC;
		}
		else if ((opcode & 0xFF) == 0x18)
		{
			uint8_t register_index = (opcode >> 8) & 0xF;
			std::cout << "LD ST, V" << (int)register_index;
			return OpcodeStatus::IncrementPC;
		}
		else if ((opcode & 0xFF) == 0x29)
		{
			uint8_t register_index = OPCODE_X(opcode);
			std::cout << "LD F, V" << (int)register_index;
			I = 5 * Registers[register_index];
			return OpcodeStatus::IncrementPC;
		}
		else if ((opcode & 0xFF) == 0x33)
		{
			uint8_t register_index = (opcode >> 8) & 0xF;
			uint8_t value = Registers[register_index];
			MemoryMapping[I] = value / 100;
			MemoryMapping[I + 1] = (value / 10) % 10; 
			MemoryMapping[I + 2] = value % 10; 
			std::cout << "LD B, V" << (int)register_index;
			return OpcodeStatus::IncrementPC;
		}

		if ((opcode & 0xFF) == 0x55)
		{
			uint8_t register_index = OPCODE_X(opcode);
			std::cout << "LD [I], V" << (int)register_index;

			for (uint8_t i = 0; i <= register_index; ++i)
			{
				MemoryMapping[I + i & 0xFFF] = Registers[i];
			}
			return OpcodeStatus::IncrementPC;
		}
		else if ((opcode & 0xFF) == 0x65)
		{
			uint8_t register_index = OPCODE_X(opcode);
			std::cout << "LD V" << (int)register_index << ", [I]";

			for (uint8_t i = 0; i <= register_index; ++i)
			{
				Registers[i] = MemoryMapping[I + i] & 0xFFF; //fixed
			}
			return OpcodeStatus::IncrementPC;
		}	

		else if ((opcode & 0xFF) == 0x1E)
		{
			uint8_t register_index = (opcode >> 8) & 0xF;
			std::cout << "ADD I, V" << (int)register_index;
			I += Registers[register_index];
			return OpcodeStatus::IncrementPC;
		}	
		else
		{
			return OpcodeStatus::NotImplemented;
		}
	}

}
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <functional>
#include <unordered_map>
#include <map>
#define SDL_MAIN_HANDLED
#include <SDL.h>

namespace chipotto
{
	enum class OpcodeStatus
	{
		IncrementPC,
		NotIncrementPC,
		NotImplemented,
		StackOverflow,
		WaitForKeyboard,
		Error
	};

	class Emulator
	{
	public:
		
		const int SPEED = 1000;
		Emulator()
		{
			fill(keys.begin(), keys.end(), false);

			keymap.insert({ SDLK_1, 0x1 });
			keymap.insert({ SDLK_2, 0x2 });
			keymap.insert({ SDLK_3, 0x3 });
			keymap.insert({ SDLK_4, 0xC });

			keymap.insert({ SDLK_q, 0x4 });
			keymap.insert({ SDLK_w, 0x5 });
			keymap.insert({ SDLK_e, 0x6 });
			keymap.insert({ SDLK_r, 0xD });

			keymap.insert({ SDLK_a, 0x7 });
			keymap.insert({ SDLK_s, 0x8 });
			keymap.insert({ SDLK_d, 0x9 });
			keymap.insert({ SDLK_f, 0xE });

			keymap.insert({ SDLK_z, 0xA });
			keymap.insert({ SDLK_x, 0x0 });
			keymap.insert({ SDLK_c, 0xB });
			keymap.insert({ SDLK_v, 0xF });


			/*for (const auto& pair : keymap)
			{
				KeyboardValuesMap[pair.second] = static_cast<SDL_Scancode>(pair.first);
			}*/

			Opcodes[0x0] = std::bind(&Emulator::Opcode0, this, std::placeholders::_1);
			Opcodes[0x1] = std::bind(&Emulator::Opcode1, this, std::placeholders::_1);
			Opcodes[0x2] = std::bind(&Emulator::Opcode2, this, std::placeholders::_1);
			Opcodes[0x3] = std::bind(&Emulator::Opcode3, this, std::placeholders::_1);
			Opcodes[0x4] = std::bind(&Emulator::Opcode4, this, std::placeholders::_1);
			Opcodes[0x5] = std::bind(&Emulator::Opcode5, this, std::placeholders::_1);
			Opcodes[0x6] = std::bind(&Emulator::Opcode6, this, std::placeholders::_1);
			Opcodes[0x7] = std::bind(&Emulator::Opcode7, this, std::placeholders::_1);
			Opcodes[0x8] = std::bind(&Emulator::Opcode8, this, std::placeholders::_1);
			Opcodes[0x9] = std::bind(&Emulator::Opcode9, this, std::placeholders::_1);
			Opcodes[0xA] = std::bind(&Emulator::OpcodeA, this, std::placeholders::_1);
			Opcodes[0xB] = std::bind(&Emulator::OpcodeB, this, std::placeholders::_1);
			Opcodes[0xC] = std::bind(&Emulator::OpcodeC, this, std::placeholders::_1);
			Opcodes[0xD] = std::bind(&Emulator::OpcodeD, this, std::placeholders::_1);
			Opcodes[0xE] = std::bind(&Emulator::OpcodeE, this, std::placeholders::_1);
			Opcodes[0xF] = std::bind(&Emulator::OpcodeF, this, std::placeholders::_1);

			const unsigned int FONTSET_SIZE = 80;

			uint8_t fontset[FONTSET_SIZE] =
			{
				0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
				0x20, 0x60, 0x20, 0x20, 0x70, // 1
				0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
				0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
				0x90, 0x90, 0xF0, 0x10, 0x10, // 4
				0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
				0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
				0xF0, 0x10, 0x20, 0x40, 0x40, // 7
				0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
				0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
				0xF0, 0x90, 0xF0, 0x90, 0x90, // A
				0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
				0xF0, 0x80, 0x80, 0x80, 0xF0, // C
				0xE0, 0x90, 0x90, 0x90, 0xE0, // D
				0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
				0xF0, 0x80, 0xF0, 0x80, 0x80  // F
			};

			for (unsigned int i = 0; i < FONTSET_SIZE; i += 5) {
				for (unsigned int j = 0; j < 5; j++) {
					MemoryMapping[ i / 5 * 0x32 + j] = fontset[i + j];
				}
			}


			Window = SDL_CreateWindow("Chip-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width * 10, height * 10, 0);
			if (!Window)
			{
				SDL_Log("Unable to create window: %s", SDL_GetError());
				return;
			}
			Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			if (!Renderer)
			{
				SDL_Log("Unable to create renderer: %s", SDL_GetError());
				SDL_DestroyWindow(Window);
				return;
			}
			Texture = SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, width, height);
			if (!Texture)
			{
				SDL_Log("Unable to create texture: %s", SDL_GetError());
				SDL_DestroyRenderer(Renderer);
				SDL_DestroyWindow(Window);
				return;
			}
		}
		~Emulator() = default;

		Emulator(const Emulator& other) = delete;
		Emulator& operator=(const Emulator& other) = delete;
		Emulator(Emulator&& other) = delete;

		bool LoadFromFile(std::filesystem::path Path)
		{
			std::ifstream file;
			file.open(Path, std::ios::binary);
			if (!file.is_open()) return false;

			auto file_size = std::filesystem::file_size(Path);

			file.read(reinterpret_cast<char*>(MemoryMapping.data() + PC), file_size);
			file.close();
		}

		bool Tick(Uint32 last_tick)
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

					if (keymap.find(key) != keymap.end())
					{
						keys[keymap[key]] = true;

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
					if (keymap.find(key) != keymap.end())
					{
						keys[keymap[key]] = false;
					}
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

			SDL_Delay(20);
			return status != OpcodeStatus::NotImplemented && status != OpcodeStatus::StackOverflow && status != OpcodeStatus::Error;			
		}

		bool IsValid() const
		{
			if (!Window || !Renderer || !Texture)
				return false;
			return true;
		}

		OpcodeStatus Opcode0(const uint16_t opcode)
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

		OpcodeStatus Opcode1(const uint16_t opcode)
		{
			uint16_t address = opcode & 0x0FFF;
			std::cout << "JP 0x" << address;
			PC = address - 2;
			return OpcodeStatus::IncrementPC;
		}

		OpcodeStatus Opcode2(const uint16_t opcode)
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

		OpcodeStatus Opcode3(const uint16_t opcode)
		{
			uint8_t register_index = (opcode >> 8) & 0xF;
			uint8_t value = opcode & 0xFF;
			std::cout << "SE V" << (int)register_index << ", 0x" << (int)value;
			if (Registers[register_index] == value)
				PC += 2;
			return OpcodeStatus::IncrementPC;
		}

		OpcodeStatus Opcode4(const uint16_t opcode)
		{
			uint8_t register_index = (opcode >> 8) & 0xF;
			uint8_t value = opcode & 0xFF;
			std::cout << "SNE V" << (int)register_index << ", 0x" << (int)value;
			if (Registers[register_index] != value)
				PC += 2;
			return OpcodeStatus::IncrementPC;
		}

		OpcodeStatus Opcode5(const uint16_t opcode)
		{
			return OpcodeStatus::NotImplemented;
		}

		OpcodeStatus Opcode6(const uint16_t opcode)
		{
			uint8_t register_index = (opcode >> 8) & 0xF;
			uint8_t register_value = opcode & 0xFF;
			Registers[register_index] = register_value;
			std::cout << "LD V" << (int)register_index << ", 0x" << (int)register_value;
			return OpcodeStatus::IncrementPC;
		}

		OpcodeStatus Opcode7(const uint16_t opcode)
		{
			uint8_t register_index = (opcode >> 8) & 0xF;
			uint8_t value = opcode & 0xFF;
			std::cout << "ADD V" << (int)register_index << ", 0x" << (int)value;
			Registers[register_index] += value;
			return OpcodeStatus::IncrementPC;
		}

		OpcodeStatus Opcode8(const uint16_t opcode)
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

		OpcodeStatus Opcode9(const uint16_t opcode)
		{
			return OpcodeStatus::NotImplemented;
		}

		OpcodeStatus OpcodeA(const uint16_t opcode)
		{
			uint16_t value = (opcode & 0xFFF);
			std::cout << "LD I, 0x" << (int)value;
			I = value;
			return OpcodeStatus::IncrementPC;
		}

		OpcodeStatus OpcodeB(const uint16_t opcode)
		{
			return OpcodeStatus::NotImplemented;
		}

		OpcodeStatus OpcodeC(const uint16_t opcode)
		{
			uint8_t register_index = (opcode >> 8) & 0xF;
			uint8_t random_mask = opcode & 0xFF;
			std::cout << "RND V" << (int)register_index << ", 0x" << (int)random_mask;
			Registers[register_index] = (std::rand() % 256) & random_mask;
			return OpcodeStatus::IncrementPC;
		}

		OpcodeStatus OpcodeD(const uint16_t opcode)
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

		OpcodeStatus OpcodeE(const uint16_t opcode)
		{
			if ((opcode & 0xFF) == 0x9E)
			{
				uint8_t register_index = (opcode >> 8) & 0xF;
				std::cout << "SKP V" << (int)register_index;
				/*if (keys[keymap[register_index]])
				{
					PC += 2;
				}*/
				PC += keys[register_index] ? 2 : 0;
				/*const uint8_t* keys_state = SDL_GetKeyboardState(nullptr);
				if (keys_state[KeyboardValuesMap[Registers[register_index]]] == 1)
				{
					PC += 2;
				}*/
				return OpcodeStatus::NotIncrementPC;
			}
			else if ((opcode & 0xFF) == 0xA1)
			{
				uint8_t register_index = (opcode >> 8) & 0xF;
				std::cout << "SKNP V" << (int)register_index;
				/*if (!keys[keymap[register_index]])
				{
					PC += 2;
				}*/
				if (!keys[register_index])
				{
					PC += 2;
				}
				else
					PC += 0;
				/*const uint8_t* keys_state = SDL_GetKeyboardState(nullptr);
				if (keys_state[KeyboardValuesMap[Registers[register_index]]] == 0)
				{
					PC += 2;
				}*/
				return OpcodeStatus::NotIncrementPC;
			}
			return OpcodeStatus::NotImplemented;
		}

		OpcodeStatus OpcodeF(const uint16_t opcode)
		{
			if ((opcode & 0xFF) == 0x55)
			{
				uint8_t register_index = (opcode >> 8) & 0xF;
				std::cout << "LD [I], V" << (int)register_index;
				for (uint8_t i = 0; i < register_index; ++i)
				{
					MemoryMapping[I + i] = Registers[i];
				}
				return OpcodeStatus::IncrementPC;
			}
			else if ((opcode & 0xFF) == 0x65)
			{
				uint8_t register_index = (opcode >> 8) & 0xF;
				std::cout << "LD V" << (int)register_index << ", [I]";
				for (uint8_t i = 0; i < register_index; ++i)
				{
					Registers[i] = MemoryMapping[I + 1];
				}
				return OpcodeStatus::IncrementPC;
			}
			else if ((opcode & 0xFF) == 0x33)
			{
				uint8_t register_index = (opcode >> 8) & 0xF;
				uint8_t value = Registers[register_index];
				MemoryMapping[I] = value / 100;
				MemoryMapping[I + 1] = value - (MemoryMapping[I] * 100) / 10;
				MemoryMapping[I + 2] = value % 10;
				std::cout << "LD B, V" << (int)register_index;
				return OpcodeStatus::IncrementPC;
			}
			else if ((opcode & 0xFF) == 0x29)
			{
				uint8_t register_index = (opcode >> 8) & 0xF;
				std::cout << "LD F, V" << (int)register_index;
				I = 5 * Registers[register_index];
				return OpcodeStatus::IncrementPC;
			}

			else if ((opcode & 0xFF) == 0x0A)
			{
				uint8_t register_index = (opcode >> 8) & 0xF;
				std::cout << "LD V" << (int)register_index << ", K";

				WaitForKeyboardRegister_Index = register_index;
				WaitingForKey = true;
				return OpcodeStatus::WaitForKeyboard;
			}

			else if ((opcode & 0xFF) == 0x1E)
			{
				uint8_t register_index = (opcode >> 8) & 0xF;
				std::cout << "ADD I, V" << (int)register_index;
				I += Registers[register_index];
				return OpcodeStatus::IncrementPC;
			}
			else if ((opcode & 0xFF) == 0x18)
			{
				uint8_t register_index = (opcode >> 8) & 0xF;
				std::cout << "LD ST, V" << (int)register_index;
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
			else if ((opcode & 0xFF) == 0x07)
			{
				uint8_t register_index = (opcode >> 8) & 0xF;
				std::cout << "LD V" << (int)register_index << ", DT";
				Registers[register_index] = DelayTimer;
				return OpcodeStatus::IncrementPC;
			}
			else
			{
				return OpcodeStatus::NotImplemented;
			}
		}

	private:
		std::array<uint8_t, 0x1000> MemoryMapping;
		std::array<uint8_t, 0x10> Registers;
		std::array<uint16_t, 0x10> Stack;
		std::array<std::function<OpcodeStatus(const uint16_t)>, 0x10> Opcodes;

		std::map<SDL_Keycode, uint8_t> keymap;

		//std::array<SDL_Scancode, 0x10> KeyboardValuesMap;

		uint16_t I = 0x0;
		uint8_t DelayTimer = 0x0;
		uint8_t SoundTimer = 0x0;
		uint16_t PC = 0x200;
		uint8_t SP = 0xFF;

		std::vector<bool> keys = std::vector<bool>(16);
		bool WaitingForKey = false;
		uint8_t WaitForKeyboardRegister_Index = 0;
		uint64_t DeltaTimerTicks = 0;

		SDL_Window* Window = nullptr;
		SDL_Renderer* Renderer = nullptr;
		SDL_Texture* Texture = nullptr;
		int width = 64;
		int height = 32;

		
	};
}

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
#pragma once

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

	#define OPCODE_NNN(opcode) (opcode & 0xFFF)
	#define OPCODE_KK(opcode) (opcode & 0xFF)
	#define OPCODE_N(opcode) (opcode & 0xF)
	#define OPCODE_X(opcode) ((opcode >> 8) & 0xF)
	#define OPCODE_Y(opcode) ((opcode >> 4) & 0xF)
	#define OPCODE_P(opcode) (opcode >> 12)

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

			// Load fontset into memory
			for (int i = 0; i < 80; i++) {
				MemoryMapping[i] = fontset[i];
			}
			/*for (unsigned int i = 0; i < FONTSET_SIZE; i += 5) {
				for (unsigned int j = 0; j < 5; j++) {
					MemoryMapping[i / 5 * 0x32 + j] = fontset[i + j];
				}
			}*/


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

		bool LoadFromFile(std::filesystem::path Path);

		bool Tick(Uint32 last_tick);

		bool IsValid() const;

		OpcodeStatus Opcode0(const uint16_t opcode);

		OpcodeStatus Opcode1(const uint16_t opcode);

		OpcodeStatus Opcode2(const uint16_t opcode);

		OpcodeStatus Opcode3(const uint16_t opcode);

		OpcodeStatus Opcode4(const uint16_t opcode);

		OpcodeStatus Opcode5(const uint16_t opcode);

		OpcodeStatus Opcode6(const uint16_t opcode);

		OpcodeStatus Opcode7(const uint16_t opcode);

		OpcodeStatus Opcode8(const uint16_t opcode);

		OpcodeStatus Opcode9(const uint16_t opcode);

		OpcodeStatus OpcodeA(const uint16_t opcode);

		OpcodeStatus OpcodeB(const uint16_t opcode);

		OpcodeStatus OpcodeC(const uint16_t opcode);

		OpcodeStatus OpcodeD(const uint16_t opcode);

		OpcodeStatus OpcodeE(const uint16_t opcode);

		OpcodeStatus OpcodeF(const uint16_t opcode);

		int key_map(unsigned int key);

//private
		std::array<uint8_t, 0x1000> MemoryMapping;
		std::array<uint8_t, 0x10> Registers;
		std::array<uint16_t, 0x10> Stack;

		std::array<std::function<OpcodeStatus(const uint16_t)>, 0x10> Opcodes;

		uint16_t I = 0x0;
		uint8_t DelayTimer = 0x0;
		uint8_t SoundTimer = 0x0;
		uint16_t PC = 0x200;
		uint8_t SP = 0xFF;
	private:
		
		

		std::map<SDL_Keycode, uint8_t> keymap;

		//std::array<SDL_Scancode, 0x10> KeyboardValuesMap;
		
		

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

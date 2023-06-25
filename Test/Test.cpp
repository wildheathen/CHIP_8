#define CLOVE_SUITE_NAME OpCodes
#include "clove-unit.h"

//#define private public  //EVIL, DO NOT USE
#include "chip8.h"
#include "chip8.cpp"


static void put_opcode(uint16_t opcode, uint16_t pos)
{
	//emulator.mem[pos] = opcode >> 8;
	//emulator.mem[pos + 1] = opcode & 0xFF;
}

//puts 02 to register A
CLOVE_TEST(Test_0x6A02) 
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0x6A02;
	uint8_t register_index = (opcode >> 8) & 0xF;
	uint8_t register_value = opcode & 0xFF;
	emulator.PC = 0;
	
	emulator.Registers[register_index] = 0x04;

	emulator.Opcodes[opcode >> 12](opcode);
	
	CLOVE_UINT_EQ(emulator.Registers[register_index], 0x02);
}

//LDI
CLOVE_TEST(Test_0xA2EA)
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0xA2EA;

	emulator.PC = 0;
	emulator.I  = 0;

	emulator.Opcodes[opcode >> 12](opcode);

	CLOVE_UINT_EQ(emulator.I, 0x2EA);
	CLOVE_UINT_NE(emulator.I, 0x2EB);
}

//DRW
CLOVE_TEST(Test_0xDAB6)
{
	//LINE 305
	/*chipotto::Emulator emulator;
	uint16_t opcode = 0xDAB6;

	emulator.PC = 0;
	emulator.I = 0;

	emulator.Opcodes[opcode >> 12](opcode);

	CLOVE_UINT_EQ(emulator.I, 0x2EA);
	CLOVE_UINT_NE(emulator.I, 0x2EB);*/
}

//CALL
CLOVE_TEST(Test_0x22D4)
{
	//LINE 305
	chipotto::Emulator emulator;
	uint16_t opcode = 0x22D4;
	uint16_t address = opcode & 0xFFF;

	emulator.PC = 4;

	emulator.Opcodes[opcode >> 12](opcode);

	//INCREMENT THE STACK POINTER
	//PUT THE CURRENT PC ON TOP ON THE STACK
	//PC IS SET TO nnn

	CLOVE_UINT_EQ(emulator.Stack[emulator.SP], 4);
	CLOVE_UINT_EQ(emulator.PC, address);
}

CLOVE_TEST(Test_0xFx33)
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0xFE33;
	uint8_t register_index = (opcode >> 8) & 0xF;
	emulator.Registers[register_index] = 0x9C;

	emulator.Opcodes[opcode >> 12](opcode);

	CLOVE_UINT_EQ(emulator.MemoryMapping[emulator.I],   1);
	CLOVE_UINT_EQ(emulator.MemoryMapping[emulator.I+1], 5);
	CLOVE_UINT_EQ(emulator.MemoryMapping[emulator.I+2], 6);
}

CLOVE_TEST(Test_0xFx55)
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0xFE55;
	uint8_t register_index = (opcode >> 8) & 0xF;

	for (int i = 0; i < register_index; i++) {
		emulator.Registers[i] = 0x80 + i;
		emulator.MemoryMapping[0x400 + i] = 0xFF;
	}
	emulator.I = 0x400;
	emulator.PC = 0x00;

	emulator.Opcodes[opcode >> 12](opcode);

	for (int i = 0; i < register_index; i++) {
		CLOVE_UINT_EQ(0x80 + i, emulator.MemoryMapping[0x400 + i]);
	}
}

CLOVE_TEST(Test_0xFx65)
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0xFE65;
	uint8_t register_index = (opcode >> 8) & 0xF;

	for (int i = 0; i < register_index; i++) {
		emulator.MemoryMapping[0x400 + i] = 0x80 + i;
		emulator.Registers[i] = 0xFF;
	}
	emulator.I = 0x400;
	emulator.PC = 0x00;

	emulator.Opcodes[opcode >> 12](opcode);
	
	for (int i = 0; i < register_index; i++) {
		CLOVE_UINT_EQ(0x80 + i, emulator.Registers[i]);
	}
}

CLOVE_TEST(Test_0xFx29)
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0xF129;
	uint8_t register_index = (opcode >> 8) & 0xF;
	emulator.I = 0;

	emulator.Opcodes[opcode >> 12](opcode);
	
	CLOVE_UINT_EQ(emulator.I, emulator.Registers[register_index] * 5);	
}

CLOVE_TEST(Test_0x7xkk)
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0x7415;
	emulator.Registers[4] = 0x11;

	emulator.Opcodes[opcode >> 12](opcode);

	CLOVE_UINT_EQ(emulator.Registers[4], 0x26);
}

CLOVE_TEST(Test_0x00EE)
{
	//chipotto::Emulator emulator;
	//uint16_t opcode = 0x00EE;

	//emulator.Stack[(int)emulator.SP++] = 0x123;
	//emulator.Stack[(int)emulator.SP++] = 0x234;
	//emulator.PC = 0x345;

	//CLOVE_UINT_EQ(2, emulator.SP);

	//emulator.Opcodes[opcode >> 12](opcode);

	//CLOVE_UINT_EQ(emulator.PC, 0x234);
	//CLOVE_UINT_EQ(emulator.SP, 1); //TODO
}

CLOVE_TEST(Test_0xFx15)
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0xF015;

	emulator.PC = 0x00;
	emulator.Registers[0] = 0x55;
	emulator.DelayTimer = 0;

	emulator.Opcodes[opcode >> 12](opcode);

	CLOVE_UINT_EQ(0x55, emulator.DelayTimer);
}

CLOVE_TEST(Test_0xFx07)
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0xF007;

	emulator.PC = 0x00;
	emulator.Registers[0] = 0;
	emulator.DelayTimer = 0x55;

	emulator.Opcodes[opcode >> 12](opcode);

	CLOVE_UINT_EQ(0x55, emulator.Registers[0]);
}

CLOVE_TEST(Test_0x3xkk)
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0x3455; //controllare

	emulator.Registers[4] = 0x55;
	emulator.PC = 0x00;

	//GESTIRE LO STEP
	if (emulator.Opcodes[opcode >> 12](opcode) == chipotto::OpcodeStatus::IncrementPC)
	{
		emulator.PC += 2;
	}

	CLOVE_UINT_EQ(4, emulator.PC);
}

CLOVE_TEST(Test_0x4xkk)
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0x3455; //controllare

	emulator.Registers[4] = 0x55;
	emulator.PC = 0x00;

	//GESTIRE LO STEP
	if (emulator.Opcodes[opcode >> 12](opcode) == chipotto::OpcodeStatus::IncrementPC)
	{
		emulator.PC += 2;
	}

	CLOVE_UINT_EQ(4, emulator.PC);
}

CLOVE_TEST(Test_0x1nnn)
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0x1123; 
	emulator.PC = 0x00;

	emulator.Opcodes[opcode >> 12](opcode);

	CLOVE_UINT_EQ(0x123, emulator.PC);
}

CLOVE_TEST(Test_0x8xy0)
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0x8450;
	emulator.Registers[4] = 0x33;
	emulator.Registers[5] = 0x55;
	emulator.PC = 0x00;

	emulator.Opcodes[opcode >> 12](opcode);

	CLOVE_UINT_EQ(0x55, emulator.Registers[4]);
}

CLOVE_TEST(Test_0x8xy2)
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0x8452;
	emulator.Registers[4] = 0x40;
	emulator.Registers[5] = 0x04;
	emulator.PC = 0x00;

	emulator.Opcodes[opcode >> 12](opcode);

	CLOVE_UINT_EQ(0x00, emulator.Registers[4]);
}

CLOVE_TEST(Test_0x8xy3)
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0x8453;
	emulator.Registers[4] = 0x55;
	emulator.Registers[5] = 0xAA;
	emulator.PC = 0x00;

	emulator.Opcodes[opcode >> 12](opcode);

	CLOVE_UINT_EQ(0xFF, emulator.Registers[4]);
}

CLOVE_TEST(Test_0x8xy4_nocarry)
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0x8454;
	emulator.Registers[4] = 0x12;
	emulator.Registers[5] = 0x34;
	emulator.PC = 0x00;

	emulator.Opcodes[opcode >> 12](opcode);

	CLOVE_UINT_EQ(0x46, emulator.Registers[4]);
	CLOVE_UINT_EQ(0, emulator.Registers[0xf]);
}

CLOVE_TEST(Test_0x8xy4_carry)
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0x8454;
	emulator.Registers[4] = 0xF0;
	emulator.Registers[5] = 0xF0;
	emulator.PC = 0x00;

	emulator.Opcodes[opcode >> 12](opcode);

	CLOVE_UINT_EQ(0xE0, emulator.Registers[4]);
	CLOVE_UINT_EQ(1, emulator.Registers[0xf]);
}

CLOVE_TEST(Test_0x8xy5)
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0x8455;
	emulator.Registers[4] = 0x30;
	emulator.Registers[5] = 0x40;
	emulator.PC = 0x00;

	emulator.Opcodes[opcode >> 12](opcode);

	CLOVE_UINT_EQ(0xF0, emulator.Registers[4]);
	CLOVE_UINT_EQ(0, emulator.Registers[0xf]);
}

CLOVE_TEST(Test_0x8xyE_0)
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0x840E;
	emulator.Registers[4] = 0x08;
	emulator.PC = 0x00;

	emulator.Opcodes[opcode >> 12](opcode);

	CLOVE_UINT_EQ(0x10, emulator.Registers[4]);
	CLOVE_UINT_EQ(0, emulator.Registers[0xf]);
}

CLOVE_TEST(Test_0x8xyE_1)
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0x840E;
	emulator.Registers[4] = 0xC8;
	emulator.PC = 0x00;

	emulator.Opcodes[opcode >> 12](opcode);

	CLOVE_UINT_EQ(0x90, emulator.Registers[4]);
	CLOVE_UINT_EQ(1, emulator.Registers[0xf]);
}

CLOVE_TEST(Test_0xEx9E)
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0xE09E;
	//emulator. = &mock_poller;

	for (char key = 0; key < 16; key++) {
		emulator.PC = 0;
		emulator.Registers[0] = key;

		emulator.Opcodes[opcode >> 12](opcode);
		if (key == 2) {
			CLOVE_UINT_EQ(4, emulator.PC);
		}
		else {
			CLOVE_UINT_EQ(2, emulator.PC);
		}
	}	
}

CLOVE_TEST(Test_0xExA1)
{
	chipotto::Emulator emulator;
	uint16_t opcode = 0xE0A1;
	//emulator. = &mock_poller;

	for (char key = 0; key < 16; key++) {
		emulator.PC = 0;
		emulator.Registers[0] = key;

		emulator.Opcodes[opcode >> 12](opcode);
		if (key != 2) {
			CLOVE_UINT_EQ(4, emulator.PC);
		}
		else {
			CLOVE_UINT_EQ(2, emulator.PC);
		}
	}
}
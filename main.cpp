/*
	Sk8_Fix
    Copyright (C) 2017  pedro-javierf

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
Surprisingly, "Tony Hawk's American Sk8land" checksum algorithm is 'EXACTLY' the same
as the one for Transformers Autobots EUR, one I cracked time ago...
*/

#include <iostream> // I/O
#include <fstream>  //File I/O

#define SWAP_UINT16(x) (((x) >> 8) | ((x) << 8))

int logicalRightShift(int x, int n) {
	return (unsigned)x >> n;
}
int arithmeticRightShift(int x, int n) {
	if (x < 0 && n > 0)
		return x >> n | ~(~0U >> n);
	else
		return x >> n;
}
unsigned int CheckCalc(char * savegame, char * r12mem);

int main(int argc, char *argv[])
{
	std::cout << std::endl << "          #   Sk8_Fix 1.1  #" << std::endl
		<< "Developed and RE'd by pedro-javierf" << std::endl
		<< "# Usage: Sk8_Fix.exe filename" << std::endl
		<< "# Requires: IntMem.bin" << std::endl << std::endl;


	//Read stream
	std::ifstream InternalmemStream;
	//Open file by its end (EOF) so we'll know it's exact size
	InternalmemStream.open("IntMem.bin", std::ios::in | std::ios::binary | std::ios::ate);
	//Check
	if (!InternalmemStream.is_open())
	{
		std::cout << "[!] Can't Open IntMem.bin";
		exit(EXIT_FAILURE);
	}

	char * r12mem;
	//Get size 
	std::streampos size = InternalmemStream.tellg();
	std::cout << "[>] Internal Memory Size: " << size << " bytes" << std::endl;
	//Allocate memory for it
	r12mem = new char[size];
	//Seek to the start of the file
	InternalmemStream.seekg(0, std::ios::beg);
	//Load the dump into memory
	InternalmemStream.read(r12mem, size);
	std::cout << "[>] Internal Memory Loaded" << std::endl;
	InternalmemStream.close();


	//File stream 'fstream' can both read and write to files
	std::fstream saveStream;
	saveStream.open(argv[1], std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
	if (!saveStream.is_open())
	{
		std::cout << "[!] Can't Open " << argv[1] << std::endl;
		exit(EXIT_FAILURE);
	}

	char * savegame;
	std::streampos size2 = saveStream.tellg();//Since we opened the file by it's end we can retrieve it's size
	std::cout << "[>] Savegame Size: " << size2 << " bytes" << std::endl;
	savegame = new char[size2];
	saveStream.seekg(0, std::ios::beg);//The we just seek (move) to the begining of the file (0x0)
	saveStream.read(savegame, size2);
	//We are not closing the stream since we'll use it to write 

	unsigned int checksum;

	checksum = CheckCalc(savegame, r12mem);	//Get checksum in little endian. Must be writen to file in big endian
	saveStream.seekg(0x220C);					//Checksum possition
	std::cout << "[>] Patching... ";

	saveStream.write(reinterpret_cast<const char *>(&checksum), sizeof(checksum)); //Yep correct way of doing this

	//std::cout << checksum;

	std::cout << "Done!" << std::endl << std::endl;
	std::cout << "[>] Press any key to exit" << std::endl;
	std::cin.get();
	saveStream.close();
	exit(EXIT_SUCCESS);
	
	

    return 0;
}

unsigned int CheckCalc(char * savegame, char * r12mem)
{

	//Simulates registers
	unsigned int r0 = 0x0000A793; //Seed. Will also store final checksum. (First two savegame bytes actually)
	unsigned int r1 = 0x00000000; //!!!Memory pointer to internal memory. When reversing, this is 0x021FF81D, which is the ARM9 memory, but since we have a file ONLY with the memory we need, we'll adjust this value to make it fit our file. Since our file contains a bit of garbage at the beginning of it (unused data by the algorithm) this register won't point exactly at 0x0 (the beginning of the file) but a higher value.
	unsigned int r2 = 0x0000220B; //Counter

	/*Posible valid counter values

	no$gba --> breakpoint r1=02200507
	0x00001995 (r2) with starting address (r1) 02200093
	....
	0x1535                                 022004F3
	0x152C (r2) with starting address (r1) 022004FC
	0x1521 (r2) with starting address (r1) 02200507
	0x1520 (r2) with starting address (r1) 02200508
	*/
	unsigned int r3 = 0x00000092; //Byte pointed by r1. Puede ser uno menos osea 0x92


	bool breaker = false;

	//Calculation Loop
	while (1) 
	{

		r3 = savegame[r1]; r1++; //Where r1 is address where savedata is 0x0 for slot 1  and 0x00000100 for slot 2 ||THIS FAILS||

								  //COOL PATCH TO CORRECTLY FIX/LOAD R3 ADDRESSES
		r3 = r3 & 0x000000FF;



		if (r2 == 0) { breaker = true; }
		r2 = r2 - 1;

		r3 = r3 ^ (arithmeticRightShift(r0, 0x8));


		r3 = r3 << 1;



		if (r3 >= 0x2710) {
			std::cout << "R2: " << std::hex << r2 << std::endl;
			std::cout << "call to r12 mem very big: " << r3;
			std::cin.get();
		}

		//Gets byte at r3, merges it with byte at r3+1
		
		r3 = ((unsigned char)r12mem[r3] << 8) | (unsigned char)r12mem[r3 + 1]; 
		r3 = _byteswap_ushort(r3); 
								   //Unsigned int is 2 bytes -> 16 bits -> ushort

								   //Boring XORs and shifts right here..
		r0 = r3 ^ (r0 << 0x8);

		r0 = r0 << 0x10;

		r0 = logicalRightShift(r0, 0x10);


		if (breaker == true) { break; }
	}

	std::cout << "Checksum(r0): " << std::hex << r0 << std::endl;
	return r0;
}

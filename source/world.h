#include <stdio.h>
#include <vitasdk.h>

typedef struct _worldHeader _worldHeader, *P_worldHeader;

struct _worldHeader { /* PlaceHolder Structure */
    int field0_0x0;
    uint8_t field1_0x4;
    uint8_t field2_0x5;
    uint8_t field3_0x6;
    uint8_t field4_0x7;
    uint8_t field5_0x8;
    uint8_t field6_0x9;
    uint8_t field7_0xa;
    uint8_t field8_0xb;
    uint8_t field9_0xc;
    uint8_t field10_0xd;
    uint8_t field11_0xe;
    uint8_t field12_0xf;
    uint8_t field13_0x10;
    uint8_t field14_0x11;
    uint8_t field15_0x12;
    uint8_t field16_0x13;
    uint8_t field17_0x14;
    uint8_t field18_0x15;
    uint8_t field19_0x16;
    uint8_t field20_0x17;
    uint8_t field21_0x18;
    uint8_t field22_0x19;
    uint8_t field23_0x1a;
    uint8_t field24_0x1b;
    uint8_t field25_0x1c;
    uint8_t field26_0x1d;
    uint8_t field27_0x1e;
    uint8_t field28_0x1f;
    uint8_t field29_0x20;
    uint8_t field30_0x21;
    uint8_t field31_0x22;
    uint8_t field32_0x23;
    int field33_0x24;
    uint8_t field34_0x28;
    uint8_t field35_0x29;
    uint8_t field36_0x2a;
    uint8_t field37_0x2b;
    uint8_t field38_0x2c;
    uint8_t field39_0x2d;
    uint8_t field40_0x2e;
    uint8_t field41_0x2f;
    uint8_t field42_0x30;
    uint8_t field43_0x31;
    uint8_t field44_0x32;
    uint8_t field45_0x33;
    uint8_t field46_0x34;
    uint8_t field47_0x35;
    uint8_t field48_0x36;
    uint8_t field49_0x37;
    uint8_t field50_0x38;
    uint8_t field51_0x39;
    uint8_t field52_0x3a;
    uint8_t field53_0x3b;
    uint8_t field54_0x3c;
    uint8_t field55_0x3d;
    uint8_t field56_0x3e;
    uint8_t field57_0x3f;
    uint8_t field58_0x40;
    uint8_t field59_0x41;
    uint8_t field60_0x42;
    uint8_t field61_0x43;
    uint8_t field62_0x44;
    uint8_t field63_0x45;
    uint8_t field64_0x46;
    uint8_t field65_0x47;
    uint8_t field66_0x48;
    uint8_t field67_0x49;
    uint8_t field68_0x4a;
    uint8_t field69_0x4b;
    uint8_t field70_0x4c;
    uint8_t field71_0x4d;
    uint8_t field72_0x4e;
    uint8_t field73_0x4f;
    uint8_t field74_0x50;
    uint8_t field75_0x51;
    uint8_t field76_0x52;
    uint8_t field77_0x53;
    uint8_t field78_0x54;
    uint8_t field79_0x55;
    uint8_t field80_0x56;
    uint8_t field81_0x57;
    int field82_0x58;
    int field83_0x5c;
    uint8_t field84_0x60;
    uint8_t field85_0x61;
    uint8_t field86_0x62;
    uint8_t field87_0x63;
    int field88_0x64;
    uint8_t field89_0x68;
    uint8_t field90_0x69;
    uint8_t field91_0x6a;
    uint8_t field92_0x6b;
    uint8_t field93_0x6c;
    uint8_t field94_0x6d;
    uint8_t field95_0x6e;
    uint8_t field96_0x6f;
    uint8_t field97_0x70;
    uint8_t field98_0x71;
    uint8_t field99_0x72;
    uint8_t field100_0x73;
    int field101_0x74;
};

void worldAllocateSegments(_worldHeader *param_1)
{
	uint uVar1;
	char *pcVar2;
	int iVar3;
	uint uVar4;
	void *pTexFile;
	int iVar6;
	int *piVar7;
	int iVar8;
	int iVar9;
	int texWidth;
	int texHeight;
	uint32_t pallette;
	int iVar13;
	char cVar14;
	uint16_t *puVar15;
	int iVar16;
	int iVar17;
	int iVar18;
	int iVar19;
	int iVar20;
	int totalPixels;
	int iVar22;
	uint uVar23;
	int *piVar24;
	short *psVar25;
	uint8_t *pbVar26;
	int iVar27;
	int iVar28;
	uint64_t *puVar29;
	int iVar30;
	int iVar31;
	int iVar32;
	int iVar33;
	int iVar34;
	int iVar35;
	int iVar36;
	int *piVar37;
	void *__newTexPtr;
	uint uVar38;
	bool bVar39;
	float fVar40;
	int iVar41;

	//uint32_t local_598 = 0;
	//uint32_t uStack_594 = 0;
	// struct
	// {
	// 	uint32_t local_650;
	// 	uint32_t local_598;	/* data */
	// } patch;
	uint32_t local_650;

	struct {
		uint32_t local_598;
		uint32_t uStack_594;
		uint64_t uStack_590;
		uint64_t local_588;
		uint64_t uStack_580;
		uint64_t local_578;
		uint64_t uStack_570;
		uint64_t local_568;
		uint64_t uStack_560;
		uint64_t local_558;
		uint64_t uStack_550;
		uint64_t local_548;
		uint64_t uStack_540;
		uint64_t local_538;
		uint64_t uStack_530;
		uint64_t local_528;
		uint64_t uStack_520;
		uint64_t local_518;
		uint64_t uStack_510;
		uint64_t local_508;
		uint64_t uStack_500;
		uint64_t local_4f8;
		uint64_t uStack_4f0;
		uint64_t local_4e8;
		uint64_t uStack_4e0;
		uint64_t local_4d8;
		uint64_t uStack_4d0;
		uint64_t local_4c8;
		uint64_t uStack_4c0;
		uint64_t local_4b8;
		uint64_t uStack_4b0;
		uint64_t local_4a8;
		uint64_t uStack_4a0;
		uint16_t huffman_table [512];
	} lockrect_struct;


	char acStack_98 [80];


	// coreCurrentWorld is at 0x0054cad8 in so_mod
	char * coreCurrentWorld = LOC(0x0054cad8);
	logv_error("worldAllocateSegments: coreCurrentWorld: %s\n", coreCurrentWorld);
	
	iVar3 = param_1->field0_0x0;
	if (0 < iVar3) {

		logv_error("worldAllocateSegments: iVar3: %i\n", iVar3);

		iVar27 = 0;
		iVar19 = 0;
		do {
			if ((*(ushort *)(param_1->field33_0x24 + iVar27 + 0x30) & 0x800) != 0) {	
				sprintf((char *)lockrect_struct.huffman_table,"%s.lmp",*(uint32_t *)(param_1->field33_0x24 + iVar27));
				logv_error("worldAllocateSegments: will load %s\n", (char *)lockrect_struct.huffman_table);
				lumpLoad((char *)lockrect_struct.huffman_table);
				iVar3 = param_1->field0_0x0;
			}

			iVar19 = iVar19 + 1;
			iVar27 = iVar27 + 0x38;
		} while (iVar19 < iVar3);
	}
	if (param_1->field101_0x74 == 0) {
		param_1->field101_0x74 = 1;
		iVar27 = param_1->field82_0x58;
		iVar36 = param_1->field83_0x5c;
		iVar19 = param_1->field88_0x64; // Pointer to some world data
		

		sprintf(acStack_98,"res\\%s.tex",coreCurrentWorld);
		iVar3 = machHostOpen(acStack_98,"rb");
		uVar4 = machHostSeek(iVar3,0,2);
		logv_error("worldAllocateSegments: seek uVar4: %u\n", uVar4); // uVar4: 3383469 (.tex file size)
		pTexFile = malloc(uVar4);	// Just load the entire .tex file into memory
		machHostSeek(iVar3,0,0);
		machHostRead(iVar3,pTexFile,uVar4);
		machHostClose(iVar3);

		logv_error("worldAllocateSegments: iVar36: %i\n", iVar36); // 5050
		logv_error("worldAllocateSegments: iVar27: %i\n", iVar27); // 4949
		logv_error("worldAllocateSegments: iVar19: 0x%X\n", iVar19);

		if (iVar36 < iVar27) {
			free(pTexFile);
		}
		else {
			__newTexPtr = (void *)0x0;
			iVar3 = 0;
			do { // while (iVar3 != (iVar36 - iVar27) + 1);  0101+1
				// Probably loops for each texture
				// Maybe iterate map objects and find their texture in the .tex file ?
				iVar6 = *(int *)(iVar19 + iVar3 * 8 + 4); // ivar19 is the pointer to some world data like 0x833DA220 (probably the .gob file)
				bVar39 = iVar6 != 0; // probably iVar6 here is like a boolean flag
				if (bVar39) {
					// if not 0, then iVar6 is the offset in the .tex file to the next group
					iVar6 = *(int *)(iVar19 + iVar3 * 8);
				}
				if (bVar39 && iVar6 != 0) { 
					logv_error("worldAllocateSegments: iVar6: %i\n", iVar6); // 2048 in the first iteration. All zeros in tavern.txt file
					iVar20 = *(int *)((int)pTexFile + iVar6);					// iVar20 is the number of objects in the group
					logv_error("worldAllocateSegments: iVar20: %i\n", iVar20);
					piVar7 = (int *)malloc(iVar20 * 0x38 | 4);
					*(int **)(iVar19 + iVar3 * 8) = piVar7;
					piVar24 = (int *)((int)pTexFile + iVar6) + 0x10;
					*piVar7 = iVar20;		// write the number of objects in the group to the first byte of the allocated memory
					__aeabi_memcpy4(*(int *)(iVar19 + iVar3 * 8) + 4, piVar24, iVar20 * 0x38);
					// iVar20 = 7, 8, 122, 14 in tavern map
					// 7 + 8 + 122 + 14 = 151 -> Total number of objects in tavern map according to world explorer tool
					// probably objects are grouped 
					// iVar20 -> total number of objects in the group
					if (0 < iVar20) {
						logv_error("worldAllocateSegments: iVar20: %i\n", iVar20);

						local_650 = 0;
						piVar7 = piVar24; // piVar7 points to the first object in the group
						// Iterate over each object in the group
						do { // while (lockrect_struct.local_650 != iVar20);
							// ivar20 == 7, 8, 122, 14

							// piVar7 is increased by 0xE every iteration
							// piVar7 points to "width" of the object
							iVar8 = piVar7[2]; 	// 120384: offset to data?
							iVar9 = *(int *)((int)piVar7 + iVar8);
							logv_error("worldAllocateSegments: iVar8: %i\n", iVar8); // 120384
							logv_error("worldAllocateSegments: iVar9: %i\n", iVar9); // iVar9=400
							iVar6 = iVar9 - (int)piVar24;
							logv_error("worldAllocateSegments: iVar6: %i\n", iVar6); 
							logv_error("worldAllocateSegments: piVar24: %i\n", piVar24); 
							logv_error("worldAllocateSegments: piVar7: %i\n", piVar7); 
							//*LOC(0x003f8a60) = (int)piVar24 + (int)piVar7 + iVar6 + 0x400;
							*LOC(0x003f8a60) = (int)piVar7 + iVar9 + 0x400; // + 1424 or 0x590
							logv_error("worldAllocateSegments: *LOC(0x003f8a60): %i\n", *LOC(0x003f8a60)); // 120384 + 0x400 = 120784
							*LOC(0x003f8a64) = (int)piVar7 + iVar6 + 0xc04; // + 3476 or 0xD94
							logv_error("worldAllocateSegments: *LOC(0x003f8a64): %i\n", *LOC(0x003f8a64)); // 120384 + 0xc04 = 121388
							*LOC(0x003f8a68) = *LOC(0x003f8a64) + *(int *)((int)piVar7 + iVar6 + 0xc00) * 2;
							logv_error("worldAllocateSegments: *LOC(0x003f8a68): %i\n", *LOC(0x003f8a68)); // 121388 + 0x1E0 = 121568
							*LOC(0x003f8a6c) = *LOC(0x003f8a68) + 0x48;
							logv_error("worldAllocateSegments: *LOC(0x003f8a6c): %i\n", *LOC(0x003f8a6c)); // 121568 + 0x48 = 121616
													
							texWidth = lowestPowerof2NotLessThan((int)*(short *)piVar7);
							//texWidth = (int)*(short *)piVar7;
							iVar6 = texWidth;
							if (texWidth < 0x41) {
								iVar6 = 0x40;
							}

							texHeight = lowestPowerof2NotLessThan((int)*(short *)((int)piVar7 + 2));
							//texHeight = (int)*(short *)((int)piVar7 + 2);

							if (__newTexPtr != (void *)0x0) {
								free(__newTexPtr);
							}

							// log the width and height of the texture
							logv_error("--------: texWidth: %i, texHeight: %i\n", texWidth, texHeight);
							logv_error("--------: texWidthRaw: %i, texHeightRaw: %i\n", (int)*(short *)piVar7, (int)*(short *)((int)piVar7 + 2));
							// log the actual width and height of the texture

							piVar37 = (int *)((int)piVar7 + iVar8) + 1;
							totalPixels = iVar6 * texHeight;
							__newTexPtr = malloc(totalPixels + 0x19000); // 102400
							__aeabi_memclr(__newTexPtr, totalPixels);
							iVar41 = *LOC(0x003f8a6c);
							iVar34 = *LOC(0x003f8a68);
							iVar8 = *LOC(0x003f8a64);
							uVar4 = 0;
							iVar13 = *(int *)(LOC(0x003f8a6c + 4));
							puVar15 = (uint16_t *)((uint)lockrect_struct.huffman_table | 2);
							
							// Probably creating the Huffman table 
							do { 
								// This loops 256 times and seems to fill
								// the 512 byte huffman_table array with some values 
								// 2 bytes each time
								// iVar13 and iVar41 contain something from memory
								// uVar4 is the loop counter
							
								if (iVar13 < (int)(uVar4 >> 7)) {
									if ((int)(uVar4 >> 6) <= *(int *)(iVar41 + 8)) {
										uVar38 = uVar4 >> 6;
										iVar18 = 2;
										goto LAB_00131f0c;
									}
									if ((int)(uVar4 >> 5) <= *(int *)(iVar41 + 0xc)) {
										uVar38 = uVar4 >> 5;
										iVar18 = 3;
										goto LAB_00131f0c;
									}
									if ((int)(uVar4 >> 4) <= *(int *)(iVar41 + 0x10)) {
										uVar38 = uVar4 >> 4;
										iVar18 = 4;
										goto LAB_00131f0c;
									}
									if ((int)(uVar4 >> 3) <= *(int *)(iVar41 + 0x14)) {
										uVar38 = uVar4 >> 3;
										iVar18 = 5;
										goto LAB_00131f0c;
									}
									if ((int)(uVar4 >> 2) <= *(int *)(iVar41 + 0x18)) {
										uVar38 = uVar4 >> 2;
										iVar18 = 6;
										goto LAB_00131f0c;
									}
									if ((int)(uVar4 >> 1) <= *(int *)(iVar41 + 0x1c)) {
										uVar38 = uVar4 >> 1;
										iVar18 = 7;
										goto LAB_00131f0c;
									}
									iVar18 = 8;
									uVar38 = uVar4;
									if ((int)uVar4 <= *(int *)(iVar41 + 0x20))
										goto LAB_00131f0c;
									iVar18 = 0;
									puVar15[-1] = 0;
								}
								else {
									uVar38 = uVar4 >> 7;
									iVar18 = 1;
									LAB_00131f0c:
									puVar15[-1] = 
									*(uint16_t *)(iVar8 + (*(int *)(iVar34 + iVar18 * 4) + uVar38) * 2);
								}


								uVar4 = uVar4 + 1;
								*puVar15 = (short)iVar18;  // iVar18 is som
								iVar18 = *LOC(0x003f8a60);
								puVar15 = puVar15 + 2;
							} while (uVar4 != 0x100);  // 256


							// This is the texture data it seems
							// uses the huffman_table array 
							// called once for each object in the group
							cVar14 = *(char *)piVar37;
							if (cVar14 != -1) {
								logv_error("-----------worldAllocateSegments: cVar14: %i\n", cVar14);
								do { //  while (cVar14 != -1); try to find the end of the texture data ?
									iVar22 = (int)*(char *)((int)piVar37 + 1);
									iVar16 = (int)cVar14;
									pcVar2 = (char *)((int)piVar37 + 3);
									cVar14 = *(char *)((int)piVar37 + 2);
									piVar37 = piVar37 + 1;
									iVar13 = (int)*pcVar2;
									// log result of (iVar13 - iVar22)
									logv_error("   worldAllocateSegments: (iVar13 - iVar22): %i\n", (iVar13 - iVar22));
									// log result of (cVar14 - iVar16)
									logv_error("   worldAllocateSegments: (cVar14 - iVar16): %i\n", (cVar14 - iVar16));

									if (*pcVar2 < iVar22) {
										iVar13 = iVar22;
									}

									iVar28 = 0;
									do { // (iVar28 != (iVar13 - iVar22) + 1);  // Repeat for each line ?
										if (iVar16 <= cVar14) {
											iVar17 = 0;
											do { //  while (iVar17 != (cVar14 - iVar16) + 1); // Repeat for each pixel in the line ? 
												iVar30 = *piVar37;
												iVar35 = 0;
												uVar38 = 0;
												uVar4 = 0;
												iVar31 = iVar17 + iVar16;
												// This loops 256 times
												// iVar35 is the loop counter
												// uses huffman_table 
												do { // while (iVar35 != 0x100); // 256
													puVar15 = (uint16_t *)((int)piVar7 + ((int)uVar4 >> 4) * 2 + iVar30);
													// Reads bit by bit 
													uVar1 = CONCAT22(*puVar15, puVar15[1]) << (uVar4 & 0xf);
													uVar23 = uVar1 >> 0x18; // 24, RGB?
													psVar25 = lockrect_struct.huffman_table + uVar23 * 2;
													iVar32 = (int)lockrect_struct.huffman_table[uVar23 * 2 + 1];
													if (iVar32 == 0) {
														uVar1 = uVar1 >> 0x10;
														iVar33 = 0;
														do {
															iVar32 = iVar33 * -4;
															uVar23 = iVar33 + 7;
															iVar33 = iVar33 + -1;
														} while (*(int *)(iVar41 + 0x24 + iVar32) < (int)(uVar1 >> (uVar23 & 0xff)));
														iVar32 = 8 - iVar33;
														psVar25 = (short *)(iVar8 + (*(int *)(iVar34 + 0x20 + iVar33 * -4) + (uVar1 >> (uVar23 & 0xff))) * 2);
													}
													uVar23 = (uint)*psVar25;
													if (0xff < (int)uVar23) {
														if ((int)uVar23 < 0x105) {
															pbVar26 = (uint8_t *)((int)&lockrect_struct.local_598 + iVar35 + (char)(LOC(0x000b0128))[uVar23]);
														}
														else {
															pbVar26 = (uint8_t *)(uVar23 + iVar18 + uVar38 * 8 + -0x105);
														}
														uVar23 = (uint)*pbVar26;
													}
													uVar38 = uVar23;
													uVar4 = iVar32 + uVar4;
													*(char *)((int)&lockrect_struct.local_598 + iVar35) = (char)uVar38;
													iVar35 = iVar35 + 1;
												} while (iVar35 != 0x100); // 256
												iVar17 = iVar17 + 1;
												piVar37 = piVar37 + 1;
												iVar30 = iVar31 * 0x10 + (iVar28 + iVar22) * iVar6 * 0x10;
												// iVar6 is the Width of the texture
												// iVar30 is the offset in the texture
												*(uint64_t *)((int)__newTexPtr + iVar30) = CONCAT44(lockrect_struct.uStack_594,lockrect_struct.local_598);
												((uint64_t *)((int)__newTexPtr + iVar30))[1] = lockrect_struct.uStack_590;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6);
												*puVar29 = lockrect_struct.local_588;
												puVar29[1] = lockrect_struct.uStack_580;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 2);
												*puVar29 = lockrect_struct.local_578;
												puVar29[1] = lockrect_struct.uStack_570;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 3);
												*puVar29 = lockrect_struct.local_568;
												puVar29[1] = lockrect_struct.uStack_560;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 4);
												*puVar29 = lockrect_struct.local_558;
												puVar29[1] = lockrect_struct.uStack_550;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 5);
												*puVar29 = lockrect_struct.local_548;
												puVar29[1] = lockrect_struct.uStack_540;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 6);
												*puVar29 = lockrect_struct.local_538;
												puVar29[1] = lockrect_struct.uStack_530;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 7);
												*puVar29 = lockrect_struct.local_528;
												puVar29[1] = lockrect_struct.uStack_520;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 8);
												*puVar29 = lockrect_struct.local_518;
												puVar29[1] = lockrect_struct.uStack_510;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 9);
												*puVar29 = lockrect_struct.local_508;
												puVar29[1] = lockrect_struct.uStack_500;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 10);
												*puVar29 = lockrect_struct.local_4f8;
												puVar29[1] = lockrect_struct.uStack_4f0;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 0xb);
												*puVar29 = lockrect_struct.local_4e8;
												puVar29[1] = lockrect_struct.uStack_4e0;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 0xc);
												*puVar29 = lockrect_struct.local_4d8;
												puVar29[1] = lockrect_struct.uStack_4d0;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 0xd);
												*puVar29 = lockrect_struct.local_4c8;
												puVar29[1] = lockrect_struct.uStack_4c0;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 0xe);
												*puVar29 = lockrect_struct.local_4b8;
												puVar29[1] = lockrect_struct.uStack_4b0;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 0xf);
												*puVar29 = lockrect_struct.local_4a8;
												puVar29[1] = lockrect_struct.uStack_4a0;
											} while (iVar17 != (cVar14 - iVar16) + 1);
										}
										iVar28 = iVar28 + 1;
									} while (iVar28 != (iVar13 - iVar22) + 1);
									cVar14 = *(char *)piVar37;
								} while (cVar14 != -1);
							}


							lockLoadingMutex(true);
							iVar34 = 0;
							iVar8 = *(int *)(iVar19 + iVar3 * 8);

							pallette = D3DDevice_CreatePalette2(0);
							iVar6 = D3DPalette_Lock2(pallette,0);
							do { // 1024 : 256 * RGBA = Initialize the 256 palette RGBA colors
								pbVar26 = (uint8_t *)((int)piVar7 + iVar34 + iVar9);
								fVar40 = (float)(long long)(int)((uint)pbVar26[1] + (uint)*pbVar26 + (uint)pbVar26[2]) * 0.3333333;
								iVar41 = (int)(fVar40 + ((float)(unsigned long)(uint)pbVar26[2] - fVar40) * 1.3 + 0.5);
								iVar13 = (int)(fVar40 + ((float)(unsigned long)(uint)*pbVar26 - fVar40) * 1.3 + 0.5);
								iVar18 = (int)(fVar40 + ((float)(unsigned long)(uint)pbVar26[1] - fVar40) * 1.3 + 0.5);
								
								// uVar4 = UnsignedSaturate(iVar41,8);
								// UnsignedDoesSaturate(iVar41,8);
								// iVar41 = UnsignedSaturate(iVar13,8);
								// UnsignedDoesSaturate(iVar13,8);
								// iVar13 = UnsignedSaturate(iVar18,8);
								// UnsignedDoesSaturate(iVar18,8);

								uVar4  = UnsignedSaturate8(iVar41);
								iVar41 = UnsignedSaturate8(iVar13);
								iVar13 = UnsignedSaturate8(iVar18);

								*(uint *)(iVar6 + iVar34) = uVar4 | (uint)pbVar26[3] << 0x18 | iVar41 << 0x10 | iVar13 << 8;
								iVar34 = iVar34 + 4;
							} while (iVar34 != 0x400); // 1024 : 256 * RGBA
							iVar8 = iVar8 + local_650 * 0x38;
							// Store the palette in the world (?) object for later use
							*(uint32_t *)(iVar8 + 0x14) = pallette;

							//- 0x8B is  D3DRS_DXT1NOISEENABLE??
							//   see: https://github.com/vncloudsco/original-xbox-kernel-source/blob/e19dd436a9add10da24e832ee14a6bf857b8a3a4/public/sdk/inc/d3d8types.h#L532
							/*
								UINT                Width,
								UINT                Height,       
								UINT                Depth,          1 
								UINT                Levels,         0
								DWORD               Usage,          0x0 	is D3DUSAGE_RENDERTARGET
								D3DFORMAT           Format,         0x8B 	is D3DRS_DXT1NOISEENABLE?? -> probably just means 1 byte per pixel
								D3DRESOURCETYPE     D3DResource     3    	is D3DRTYPE_TEXTURE
							*/
							pallette = D3DDevice_CreateTexture2(texWidth, texHeight, 1, 0, 0, 0x8b, 3);
							//logv_error("worldAllocateSegments: D3DDevice_CreateTexture2 passed return value: %i\n", pallette);

							// print addressof  lockrect_struct.local_598 and lockrect_struct.uStack_594
							//logv_error("worldAllocateSegments: lockrect_struct.local_598: %p, lockrect_struct.uStack_594: %p\n", &lockrect_struct.local_598, &lockrect_struct.uStack_594);

							D3DTexture_LockRect(pallette, 0, (int*)&lockrect_struct.local_598, 0, 0);
							//print contents of lockrect_struct.local_598 and lockrect_struct.uStack_594
							logv_error("worldAllocateSegments: lockrect_struct.local_598: %i, lockrect_struct.uStack_594: %p\n", lockrect_struct.local_598, lockrect_struct.uStack_594);
							//int lockedRect[2];
							//D3DTexture_LockRect(pallette, 0, lockedRect, NULL, 0);
							// int pitch    = lockedRect[0];
							// int ptrValue = lockedRect[1];

							//log_error("worldAllocateSegments: D3DTexture_LockRect passed \n");
							//logv_error("Will call __aeabi_memcpy with totalPixels: %i, __newTexPtr: %p, lockrect_struct.uStack_594: %p\n", totalPixels, __newTexPtr, lockrect_struct.uStack_594);
							__aeabi_memcpy(lockrect_struct.uStack_594, __newTexPtr, totalPixels);
							//__aeabi_memcpy((void*)ptrValue, __newTexPtr, totalPixels);

							//log_error("worldAllocateSegments: __aeabi_memcpy passed \n");
							// // Pseudocode for a row-by-row copy:
							// uint8_t* dst = (uint8_t*) lockrect_struct.uStack_594;   // from LockRect
							// const uint8_t* src = (uint8_t*) __newTexPtr;  // your CPU buffer
							// int pitch = lockrect_struct.local_598;                 // from LockRect
							// int rows = actualHeight;               // or however you track it
							// int rowBytes = actualWidth;            // 1 byte per texel if palettized

							// for (int y = 0; y < rows; ++y) {
							// 	memcpy(dst, src, rowBytes);
							// 	dst += pitch;
							// 	src += rowBytes;
							// }

							D3DTexture_UnlockRect(pallette,0);
							*(uint32_t *)(iVar8 + 0x10) = pallette;
							releaseLoadingMutex();
							local_650 = local_650 + 1;
							piVar7 = piVar7 + 0xe;
						} while (local_650 != iVar20);
					}
				}
				iVar3 = iVar3 + 1;
			} while (iVar3 != (iVar36 - iVar27) + 1);
			free(pTexFile);
			if (__newTexPtr != (void *)0x0) {
				free(__newTexPtr);
			}
		}
	}
}
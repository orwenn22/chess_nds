#include <nds.h>
#include <stdio.h>

#include "Board.h"
#include "main.h"
#include "spritesheet.h"
#include "spritesheetpal.h"

///////////////////////////////
//// Globales

//Variable globales pour les graphismes

//bg0 : pièces
//bg1 : sélections
//bg2 : grille
int bg0, bg1, bg2;

u16 *bg0_map;   //modifié par Board.c
u16 *bg0_gfx;
u16 *bg1_map;   //modifié par Board.c
u16 *bg1_gfx;
u16 *bg2_gfx;


// Input
touchPosition touch;
int down;


///////////////////////////////////////
////Forward declarations
void InitGraphics();


int main(void) {
	InitGraphics();

	Board *b = MakeBoard();

	while(1) {
		scanKeys();
		down = keysDown();
		if(down & KEY_TOUCH) touchRead(&touch);

		UpdateBoard(b);
		DrawBoard(b);

		swiWaitForVBlank();
	}

	DeleteBoard(b);
}



void InitGraphics() {
	consoleDemoInit();
	lcdMainOnBottom();

	videoSetMode(MODE_5_2D);		//Mode 5 pour pouvoir avoir un bitmap en BG2
	vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
	vramSetBankB(VRAM_B_MAIN_BG_0x06020000);	//necessaire pour stocker entièrement le bg de la grille (sinon ça dépasse)

	//BG 0 (but : dessiner les pièces de jeux dessus)
	//BgType_Text : tilemap 16 bits par tile
	//gfx du BG : 8bit par pixel (index de palette, les couleurs sont encodés sur 16 bits)
	//BgSize_T_256x256 : taille de (256/8) * (256/8) = 32*32 tiles (1 tile = 2 octets) (taille en mémoire : 32*32*2 = 0d2048 = 0x800)
	//adresse map : 0x06000000 + 0x800 * 0   (map base)
	//adresse gfx : 0x06000000 + 0x4000 * 1   (tile base)
	bg0 = bgInit(0, BgType_Text8bpp, BgSize_T_256x256, 0, 1);

	//BG 1 (but : dessiner les sélections)
	//adresse map : 0x06000000 + 0x800 * 1
	//adresse gfx : 0x06000000 + 0x4000 * 1   (tile base) (même gfx que bg0)
	bg1 = bgInit(1, BgType_Text8bpp, BgSize_T_256x256, 1, 1);

	//BG 2 (but : griller d'arrière plan)
	//BgType_Bmp16 : bitmap 16 bit par pixel
	//BgSize_B16_256x256 : taille de 256*256 pixels
	//adresse gfx : 0x06000000 + 0x4000 * 3   ( !!! map base !!! )
	//													  v ça, ça définit le gfx (contrairement à au dessus)
	bg2 = bgInit(2, BgType_Bmp16, BgSize_B16_256x256, 3, 0);

	bgSetPriority(bg1, 0);		//sélections
	bgSetPriority(bg0, 1);		//pièces de jeu
	bgSetPriority(bg2, 3);		//grille

	bg0_map = bgGetMapPtr(bg0);
	bg0_gfx = bgGetGfxPtr(bg0);
	bg1_map = bgGetMapPtr(bg1);
	bg1_gfx = bgGetGfxPtr(bg1);
	bg2_gfx = bgGetGfxPtr(bg2);
	printf("Graphics memmap :\n");
	printf("  bg0 map : 0x%p\n", bg0_map);
	printf("  bg0 gfx : 0x%p\n", bg0_gfx);
	printf("  bg1 map : 0x%p\n", bg1_map);
	printf("  bg1 gfx : 0x%p\n", bg1_gfx);
	printf("  bg2 gfx : 0x%p\n", bg2_gfx);


	//Copier les sprites dans la mémoire vidéo pour le BG0 et BG1
	dmaCopy(spritesheetpal, BG_PALETTE, sizeofspritesheetpal);
	dmaCopy(spritesheet, bg0_gfx, sizeofspritesheet);

	////Pour débugger : afficher tous les sprites dans bg0
	//for(int i = 0; i < sizeofspritesheet/(8*8); i++) {
	//	bg0_map[i] = i;
	//}
	////Pour débugger : srpites de sélections
	//for(int i = 0; i < sizeofspritesheet/(8*8); i++) {
	//	bg1_map[i] = 2;
	//}

	//Dessiner la grille dans le BG2
	const u16 grid_colors[] = {
		RGB15(0b11011, 0b11011, 0b11011) | (1 << 15),
		RGB15(0b00100, 0b00100, 0b00100) | (1 << 15),
	};
	for(int y = 0; y < 8; y++) for(int x = 0; x < 8; x++) {
		u16 c = grid_colors[(x+y)%2];
		for(int py = y*24; py < y*24 + 24; py++) for(int px = x*24; px < x*24 + 24; px++) {
			bg2_gfx[px + py*256] = c;
		}
	}
}

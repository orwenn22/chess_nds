#ifndef MAIN_H
#define MAIN_H

#include <nds.h>

//Variable globales pour les graphismes

//bg0 : pièces
//bg1 : sélections
//bg2 : grille
extern int bg0, bg1, bg2;

extern u16 *bg0_map;   //modifié par Board.c
extern u16 *bg0_gfx;
extern u16 *bg1_map;   //modifié par Board.c
extern u16 *bg1_gfx;
extern u16 *bg2_gfx;


// Input
extern touchPosition touch;
extern int down;

#endif

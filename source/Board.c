#include "Board.h"

#include <nds.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "main.h"







//////////////////////////////////////////////
//// Board

//forward declarations
static void BoardHandleClick(Board* b);
static bool CheckCanCaptureKing(Board* b, enum PawnTeam team);

static void SetPossibleMove(Board* b,  int x, int y, unsigned char value);
static unsigned char GetPossibleMove(Board* b, int x, int y);

static bool PossibleMovePawn(Board* b, int pawn_x, int pawn_y);
static bool PossibleMoveTower(Board* b, int pawn_x, int pawn_y);
static bool PossibleMoveKnight(Board* b, int pawn_x, int pawn_y);
static bool PossibleMoveCrazy(Board* b, int pawn_x, int pawn_y);
static bool PossibleMoveQueen(Board* b, int pawn_x, int pawn_y);
static bool PossibleMoveKing(Board* b, int pawn_x, int pawn_y);


Board* MakeBoard() {
    Board* r = malloc(sizeof(Board));
    r->cells = malloc(sizeof(unsigned  char) * 8 * 8);
    r->possible_moves = malloc(sizeof(unsigned  char) * 8 * 8);
    for(int i = 0; i < 64; i++) {
        r->cells[i] = 0;
        r->possible_moves[i] = 0;
    }

    const enum PawnType layout[8] = {
            PawnType_Tower, PawnType_Knight, PawnType_Crazy, PawnType_Queen, PawnType_King, PawnType_Crazy, PawnType_Knight, PawnType_Tower
    };

    for(int i = 0; i < 8; i++) {
        BoardSetPawn(r, i, 0, PawnTeam_Black, layout[i]);
        BoardSetPawn(r, i, 1, PawnTeam_Black, PawnType_Pawn);

        BoardSetPawn(r, i, 6, PawnTeam_White, PawnType_Pawn);
        BoardSetPawn(r, i, 7, PawnTeam_White, layout[i]);
    }

    r->current_turn = PawnTeam_White;
    r->selected_cell = -1;

    r->overred_x = 0;
    r->overred_y = 0;

    r->need_redraw = 1;

    return r;
}

void DeleteBoard(Board* b) {
    if(b == NULL) return;

    if(b->cells != NULL) {
        free(b->cells);
        b->cells = NULL;
    }

    if(b->possible_moves != NULL) {
        free(b->possible_moves);
        b->possible_moves = NULL;
    }

    free(b);
}

void UpdateBoard(Board* b) {
    int w = 192 / 8;
    int h = 192 / 8;

    b->overred_x = touch.px / w;
    b->overred_y = touch.py / h;


    if(down & KEY_TOUCH) {
        BoardHandleClick(b);
    }
}

void DrawBoard(Board* b) {
    if(b->need_redraw == 0) return;
    b->need_redraw = 0;


    for(int y = 0; y < 8; y++) {
        for(int x = 0; x < 8; x++) {
            //Afficher la pièce sur la case s'il y en a une
            unsigned char pawn = BoardGetPawn(b, x, y);
            
            if((pawn & 0x0f) != PawnType_None) {
                //déterminer les propriétés de la pièce
                unsigned char team = (pawn >> 4) - 1;
                unsigned char texture_id = (pawn & 0x0f) - 1;

                //déterminer le numéro de sprite
                u16 sprite_num = 3 + (team*3*3*6) + (3*3*texture_id);

                //afficher sur bg0
                for(int ty = 0; ty < 3; ty++) {
                    for(int tx = 0; tx < 3; tx++) {
                        bg0_map[(x*3)+tx + ((y*3)+ty)*32] = sprite_num + (tx + ty*3);
                    }
                }
            }
            else {  //case vide
                for(int ty = 0; ty < 3; ty++) {
                    for(int tx = 0; tx < 3; tx++) {
                        //il y a des *3 car une case fait 3 tiles
                        bg0_map[(x*3)+tx + ((y*3)+ty)*32] = 0;
                    }
                }
            }

            //Si le move de la pièce selectionné est possible sur cette case l'afficher en vert (sur bg1)
            u16 bg1_newval = (b->possible_moves[x + y*8] == 1)*2;
            for(int ty = 0; ty < 3; ty++) {
                for(int tx = 0; tx < 3; tx++) {
                    //il y a des *3 car une case fait 3 tiles
                    bg1_map[(x*3)+tx + ((y*3)+ty)*32] = bg1_newval;
                }
            }
        }
    }


    //Case actuellement sélectionné (sur bg1)
    u16 x = b->selected_cell%8;
    u16 y = b->selected_cell/8;
    for(int ty = 0; ty < 3; ty++) {
        for(int tx = 0; tx < 3; tx++) {
            //il y a des *3 car une case fait 3 tiles
            bg1_map[(x*3)+tx + ((y*3)+ty)*32] = 1;
        }
    }
}




unsigned char BoardGetPawn(Board* b, int x, int y) {
    if(x < 0 || x >= 8 || y < 0 || y >= 8) return 0;
    return b->cells[x + y*8];
}

void BoardSetPawn(Board* b, int x, int y, enum PawnTeam team, enum PawnType type) {
    if(x < 0 || x >= 8 || y < 0 || y >= 8) return;
    b->cells[x + y*8] = (team << 4) | type;
}


static void BoardHandleClick(Board* b) {
    b->need_redraw = 1;
    unsigned char cell = BoardGetPawn(b, b->overred_x, b->overred_y);   //valeur de la case qui vient d'être cliqué
    int overred_index = (char)b->overred_x + (char)b->overred_y*8;  //index de la case qui vient d'être cliqué

    //Vérifier si la case qui vient d'être cliqué ne possède pas de pièce du joueur actuel
    if((cell >> 4) != b->current_turn) {
        //S'il n'y a pas de case déjà sélectionné, ne pas essayer de bouger
        if(b->selected_cell == -1) return;

        //vérifier si la pièce selectionné peut bouger sur la case cliqué
        if(b->possible_moves[overred_index] == 1) {
            //si oui déplacer la case
            unsigned char overred_backup = b->cells[overred_index];
            b->cells[overred_index] = b->cells[b->selected_cell];
            b->cells[b->selected_cell] = 0;


            //vérifier si l'autre team peut capturer le roi de la team actuel et si oui undo le mouvement
            enum PawnTeam other_team = (b->current_turn == PawnTeam_White) ? PawnTeam_Black : PawnTeam_White;
            printf("Checking\n");
            if(CheckCanCaptureKing(b, other_team)) {
                printf("OH NO\n");
                //Undo le mouvement
                b->cells[b->selected_cell] = b->cells[overred_index];
                b->cells[overred_index] = overred_backup;

                //Reset les variables lié au déplacement
                b->selected_cell = -1;
                for(int i = 0; i < 64; i++) b->possible_moves[i] = 0;
                return;
            }

            //Reset les variables lié au déplacement
            b->selected_cell = -1;
            for(int i = 0; i < 64; i++) b->possible_moves[i] = 0;

            //Passer au tour du joueur suivant si le roi n'est pas en échec
            b->current_turn = other_team;
        }
        return;
    }

    b->selected_cell = overred_index;

    ////déterminer les cases sur lesquelles la pièce choisi peut se déplacer
    //"reset" les possibilités
    for(int i = 0; i < 64; i++) b->possible_moves[i] = 0;

    //Determiner le type de la pièce
    switch(cell & 0x0f) {
        case PawnType_Pawn: PossibleMovePawn(b, b->overred_x, b->overred_y); break;
        case PawnType_Tower: PossibleMoveTower(b, b->overred_x, b->overred_y); break;
        case PawnType_Knight: PossibleMoveKnight(b,b->overred_x, b->overred_y); break;
        case PawnType_Crazy: PossibleMoveCrazy(b, b->overred_x, b->overred_y); break;
        case PawnType_Queen: PossibleMoveQueen(b, b->overred_x, b->overred_y); break;
        case PawnType_King: PossibleMoveKing(b, b->overred_x, b->overred_y); break;
        default: /* rien */ break;
    }
}


//vérifier si la team "team" peut capturer le roi adverse
static bool CheckCanCaptureKing(Board* b, enum PawnTeam team) {
    for(int y = 0; y < 8; y++) {
        for(int x = 0; x < 8; x++) {
            unsigned char cell = BoardGetPawn(b, x, y);
            if((cell >> 4) != team) continue;
            bool result = false;

            switch(cell & 0x0f) {
                case PawnType_Pawn: result = PossibleMovePawn(b, x, y); break;
                case PawnType_Tower: result = PossibleMoveTower(b, x, y); break;
                case PawnType_Knight: result = PossibleMoveKnight(b,x, y); break;
                case PawnType_Crazy: result = PossibleMoveCrazy(b, x, y); break;
                case PawnType_Queen: result = PossibleMoveQueen(b, x, y); break;
                case PawnType_King: result = PossibleMoveKing(b, x, y); break;
                default: result = false; break;
            }

            if(result == true) return true;
        }
    }

    return false;
}


///////////////////////////////////////////////////////////
/// Calculer les moves possibles pour chaque pièces

static void SetPossibleMove(Board* b, int x, int y, unsigned char value) {
    if(x < 0 || x >= 8 || y < 0 || y >= 8) return;
    b->possible_moves[x+y*8] = value;
}

static unsigned char GetPossibleMove(Board* b, int x, int y) {
    if(x < 0 || x >= 8 || y < 0 || y >= 8) return 0;
    return b->possible_moves[x+y*8];
}

//Vérifie si la pièce target est le roi adverse (si oui retourne 2), puis si non vérifie si target est dans une team différente que source (si oui retourne 1, sinon 0)
//note : target ne doit PAS être 0
static int CanCapture(unsigned char source, unsigned char target) {
    if(((target & 0x0f) == PawnType_King) && (source >> 4) != (target >> 4)) return 2;  //ne peut pas capturer le roi adverse (retourne 2)
    return ((source >> 4) != (target >> 4));        //si les team sont différentes retourner true, sinon false
}


//Toutes les fonctions ci-dessous suivent la même forme : PossibleMove[...](b, pawn_x, pawn_y)
// b : board
// pawn_x : position x de la pièce dans b
// pawn_y : position y de la pièce dans b

static bool PossibleMovePawn(Board* b, int pawn_x, int pawn_y) {
    //printf("Pawn\n");
    unsigned char pawn = BoardGetPawn(b, pawn_x, pawn_y);

    int direction = ((pawn >> 4) == PawnTeam_White) ? -1 : 1;

    bool can_eat_king = false;  //true si la pièce est en mesure de capturer le roi adverse

    //Vérifier si une pièce est devant le pion
    if(BoardGetPawn(b, pawn_x, pawn_y + 1*direction) == 0) {
        //si non, la pièce peut bouger en avant
        SetPossibleMove(b, pawn_x, pawn_y + 1*direction, 1);

        //La pièce peut avancer de 2 uniquement si la pièce est dans son état initiale
        if((direction == 1 && pawn_y == 1) || (direction == -1 && pawn_y == 6)) {
            //Vérifier si la case est déjà occupé
            if(BoardGetPawn(b, pawn_x, pawn_y + 2*direction) == 0) {
                //Si non la pièce peut bouger de 2 vers l'avant
                SetPossibleMove(b, pawn_x, pawn_y + 2 * direction, 1);
            }
        }
    }

    //vérifier si une pièce de l'équipe adverse est en diag droit
    unsigned char diag_right = BoardGetPawn(b, pawn_x+1, pawn_y + 1*direction);
    int diag_right_capture = CanCapture(pawn, diag_right);
    if(diag_right_capture == 2) can_eat_king = true;
    if(((diag_right >> 4) != 0) && (diag_right_capture == 1)) {  //vérifier team adverse
        SetPossibleMove(b, pawn_x+1, pawn_y + 1*direction, 1);
    }

    //vérifier si une pièce de l'équipe adverse est en diag gauche
    unsigned char diag_left = BoardGetPawn(b, pawn_x-1, pawn_y + 1*direction);
    int diag_left_capture = CanCapture(pawn, diag_left);
    if(diag_left_capture == 2) can_eat_king = true;
    if(((diag_left >> 4) != 0) && (diag_left_capture == 1)) {  //vérifier team adverse
        SetPossibleMove(b, pawn_x-1, pawn_y + 1*direction, 1);
    }

    return can_eat_king;
}

static bool PossibleMoveTower(Board* b, int pawn_x, int pawn_y) {
    //printf("Tower\n");
    unsigned char pawn = BoardGetPawn(b, pawn_x, pawn_y);

    bool can_eat_king = false;  //true si la pièce est en mesure de capturer le roi adverse

    //haut
    for(int y = pawn_y-1; y >= 0; y--) {
        unsigned char pawn_other = BoardGetPawn(b, pawn_x, y);
        if(pawn_other == 0) {   //case vide
            SetPossibleMove(b, pawn_x, y, 1);
            continue;   //prochaine itération de la boucle
        }
        else {
            int capture = CanCapture(pawn, pawn_other);
            if(capture == 1) {  //pas la même équipe
                SetPossibleMove(b, pawn_x, y, 1);
            }
            else if(capture == 2) { //roi adverse
                can_eat_king = true;
            }
            break;
        }
    }

    //bas
    for(int y = pawn_y+1; y < 8; y++) {
        unsigned char pawn_other = BoardGetPawn(b, pawn_x, y);
        if(pawn_other == 0) {   //case vide
            SetPossibleMove(b, pawn_x, y, 1);
            continue;   //prochaine itération de la boucle
        }
        else {
            int capture = CanCapture(pawn, pawn_other);
            if(capture == 1) {  //pas la même équipe
                SetPossibleMove(b, pawn_x, y, 1);
            }
            else if(capture == 2) { //roi adverse
                can_eat_king = true;
            }
            break;
        }
    }

    //gauche
    for(int x = pawn_x-1; x >= 0; x--) {
        unsigned char pawn_other = BoardGetPawn(b, x, pawn_y);
        if(pawn_other == 0) {   //case vide
            SetPossibleMove(b, x, pawn_y, 1);
            continue;   //prochaine itération de la boucle
        }
        else {
            int capture = CanCapture(pawn, pawn_other);
            if(capture == 1) {  //pas la même équipe
                SetPossibleMove(b, x, pawn_y, 1);
            }
            else if(capture == 2) { //roi adverse
                can_eat_king = true;
            }
            break;
        }
    }

    //droite
    for(int x = pawn_x+1; x < 8; x++) {
        unsigned char pawn_other = BoardGetPawn(b, x, pawn_y);
        if(pawn_other == 0) {   //case vide
            SetPossibleMove(b, x, pawn_y, 1);
            continue;   //prochaine itération de la boucle
        }
        else {
            int capture = CanCapture(pawn, pawn_other);
            if(capture == 1) {  //pas la même équipe
                SetPossibleMove(b, x, pawn_y, 1);
            }
            else if(capture == 2) { //roi adverse
                can_eat_king = true;
            }
            break;
        }
    }

    return can_eat_king;
}

static bool PossibleMoveKnight(Board* b, int pawn_x, int pawn_y) {
    //printf("Knight\n");
    unsigned char pawn = BoardGetPawn(b, pawn_x, pawn_y);

    bool can_eat_king = false;  //true si la pièce est en mesure de capturer le roi adverse

    unsigned char pawn_other = 0;
    int capture = 0;

    const int relative_positions[8][2] = {
            {-1,-2},    //2haut 1gauche
            {1,-2},     //2haut 1droit
            {-1,2},     //2bas 1gauche
            {1,2},      //2bas 1droit
            {-2,-1},    //2gauche 1haut
            {-2,1},     //2gauche 1bas
            {2,-1},     //2droit 1haut
            {2,1}       //2droit 1bas
    };

    for(int i = 0; i < 8; i++) {
        int offset_x = relative_positions[i][0];
        int offset_y = relative_positions[i][1];

        pawn_other = BoardGetPawn(b, pawn_x+offset_x, pawn_y+offset_y);
        capture = CanCapture(pawn, pawn_other);
        if(capture == 2) can_eat_king = true;
        if((pawn_other == 0) || (capture == 1)) {   //case vide ou pièce d'une autre team
            SetPossibleMove(b, pawn_x+offset_x, pawn_y+offset_y, 1);
        }
    }

    return can_eat_king;
}

static bool PossibleMoveCrazy(Board* b, int pawn_x, int pawn_y) {
    //printf("Crazy\n");
    unsigned char pawn = BoardGetPawn(b, pawn_x, pawn_y);

    bool can_eat_king = false;  //true si la pièce est en mesure de capturer le roi adverse
    const int relative_positions[4][2] = {
            {-1,-1},    //haut gauche
            {-1,1},     //haut droite
            {1,-1},     //bas gauche
            {1,1},      //bas droite
    };

    //Faire toutes les diagonales de la pièce
    for(int i = 0; i < 4; i++) {
        int offset_x = relative_positions[i][0];
        int offset_y = relative_positions[i][1];

        int pos_x = pawn_x + offset_x;
        int pos_y = pawn_y + offset_y;

        while(pos_x >= 0 && pos_x < 8 && pos_y >= 0 && pos_y < 8) {
            unsigned char pawn_other = BoardGetPawn(b, pos_x, pos_y);
            int capture = CanCapture(pawn, pawn_other);
            if(capture == 2) {      //roi adverse
                can_eat_king = true;
                break;
            }
            else if(pawn_other == 0 || capture == 1) {  //aucune pièce sur la case ou pièce adverse
                SetPossibleMove(b, pos_x, pos_y, 1);
            }

            if(pawn_other != 0) break;

            pos_x += offset_x;
            pos_y += offset_y;
        }
    }

    return can_eat_king;
}

static bool PossibleMoveQueen(Board* b, int pawn_x, int pawn_y) {
    return PossibleMoveTower(b, pawn_x, pawn_y) | PossibleMoveCrazy(b, pawn_x, pawn_y);
}

static bool PossibleMoveKing(Board* b, int pawn_x, int pawn_y) {
    //printf("King\n");
    unsigned char pawn = BoardGetPawn(b, pawn_x, pawn_y);

    bool can_eat_king = false;  //true si la pièce est en mesure de capturer le roi adverse

    for(int offset_y = -1; offset_y < 2; offset_y++) {
        for(int offset_x = -1; offset_x < 2; offset_x++) {
            if(offset_x == 0 && offset_y == 0) continue;

            unsigned char pawn_other = BoardGetPawn(b, pawn_x+offset_x, pawn_y+offset_y);
            int capture = CanCapture(pawn, pawn_other);

            if(capture == 2) {      //roi adverse
                can_eat_king = true;
            }
            else if(pawn_other == 0 || capture == 1) {  //aucune pièce sur la case ou pièce adverse
                SetPossibleMove(b, pawn_x+offset_x, pawn_y+offset_y, 1);
            }
        }
    }

    return can_eat_king;
}

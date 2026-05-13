#include "piece.h"

int piece_score(PieceType type) {
    switch (type) {
    case PIECE_INFANTRY:    return 1;
    case PIECE_TANK:        return 2;
    case PIECE_SNIPER:      return 2;
    case PIECE_ARTILLERY:   return 3;
    case PIECE_EXCAVATOR:   return 3;
    case PIECE_MISSILE:     return 4;
    case PIECE_AIR_DEFENSE: return 4;
    case PIECE_BOMBER:      return 5;
    case PIECE_COMMANDER:   return COMMANDER_SCORE;
    default: return 0;
    }
}

const char* piece_name(PieceType type) {
    switch (type) {
    case PIECE_INFANTRY:    return "INFANTRY";
    case PIECE_TANK:        return "TANK";
    case PIECE_SNIPER:      return "SNIPER";
    case PIECE_ARTILLERY:   return "ARTILLERY";
    case PIECE_MISSILE:     return "MISSILE";
    case PIECE_AIR_DEFENSE: return "AIR DEFENSE";
    case PIECE_BOMBER:      return "BOMBER";
    case PIECE_COMMANDER:   return "COMMANDER";
    case PIECE_EXCAVATOR:   return "EXCAVATOR";
    default: return "---";
    }
}

const char* piece_short_name(PieceType type) {
    switch (type) {
    case PIECE_INFANTRY:    return "INF";
    case PIECE_TANK:        return "TNK";
    case PIECE_SNIPER:      return "SNP";
    case PIECE_ARTILLERY:   return "ART";
    case PIECE_MISSILE:     return "MIS";
    case PIECE_AIR_DEFENSE: return "ADF";
    case PIECE_BOMBER:      return "BMB";
    case PIECE_COMMANDER:   return "CMD";
    case PIECE_EXCAVATOR:   return "EXC";
    default: return "---";
    }
}

char piece_letter(PieceType type) {
    switch (type) {
    case PIECE_INFANTRY:    return 'I';
    case PIECE_TANK:        return 'T';
    case PIECE_SNIPER:      return 'S';
    case PIECE_ARTILLERY:   return 'A';
    case PIECE_MISSILE:     return 'M';
    case PIECE_AIR_DEFENSE: return 'D';
    case PIECE_BOMBER:      return 'B';
    case PIECE_COMMANDER:   return 'C';
    case PIECE_EXCAVATOR:   return 'E';
    default: return '.';
    }
}

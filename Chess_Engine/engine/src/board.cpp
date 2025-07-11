// For my board im going to use 12-bitboard layout (one 64 bit word for every{colour, piece type} combination)
#include <bit>
#include <cstdint>
enum Colour { WHITE = 0, BLACK = 1 }; // Enum for colours
enum Piece { PAWN = 0, KNIGHT, BISHOP, ROOK, QUEEN, KING }; // Enum for piece types
using Bitboard = uint64_t; // convenience
struct Position{ // Represents chess positions using bitboards
    Bitboard bb[2][6];   // [color][piece]
    Bitboard occ[2];     // per-color
    Bitboard allOcc;     // union
    Colour stm;        // side to move
    uint64_t zobrist;    // hash key for the state of board

};

//Inlining substitutes the function body at each call site, and optimises the code around as one big chunk
//Constexpr executes the expression while compiling and emits the final literal value directly into the code.
//Useful for constants and small functions that can be evaluated at compile time as reduces overhead (resources used to call func) useful as these will be called many times

//Square is a number from 0 to 63 representing a square on the chessboard (0 = A1, 63 = H8)

inline constexpr Bitboard sq_bb(int square) {
    return 1ULL << square;
}
inline constexpr int rank_of(int square ) {return square >> 3;} // Get the rank (0-7) of a square (could also just divide by 8 but this more efficient)
inline constexpr int file_of(int square) {return square & 7;} // Get the file (0-7) of a square (0 = A, 7 = H)
inline constexpr int make_sq(int rank, int file) {return (rank << 3) | file;} // Create a square from rank and file (rank * 8 + file)

inline constexpr int has(const Bitboard& bb, int square) { return (bb & sq_bb(square)) != 0; } //Check if the bitboard has a piece on the specified square
inline constexpr int set(const Bitboard& bb, int square) {return (bb | sq_bb(square));} // Set a piece on the specified square in the bitboard
inline constexpr int clear(const Bitboard& bb, int square) {return (bb & ~sq_bb(square));} // Clear a piece from the specified square in the bitboard
inline int popcount(Bitboard bb)               { return std::popcount(bb); }          // count the number of bits set in the bitboard
inline int lsb(Bitboard bb)                    { return std::countr_zero(bb); }       // get the index of the least significant bit (LSB) set in the bitboard
/// Extract and remove the LS1B in one shot (classic pop-lsb)
inline int   pop_lsb(Bitboard& bb)
{
    int sq = lsb(bb);
    bb &= bb - 1;          // clear LS1B (blsr on x86)
    return sq;
}

inline constexpr Bitboard file_mask(int file)    { return 0x0101010101010101ULL << file; } // Create a bitboard mask for the specified file (0 = A, 7 = H)
inline constexpr Bitboard rank_mask(int rank)    { return 0xFFULL << (rank * 8); } // Create a bitboard mask for the specified rank (0 = 1st rank, 7 = 8th rank)

inline constexpr Bitboard dark_squares   = 0xAA55AA55AA55AA55ULL; // Bitboard mask for dark squares (A1, B2, C3, etc.)
inline constexpr Bitboard light_squares  = ~dark_squares; // Bitboard mask for light squares (A2, B1, C4, etc.)

inline void addPiece(Position& pos, Colour c, Piece p, int sq) { // "Position&" makes it a reference of the Position struct, so we can modify it directly instead of a copy
    uint64_t m = 1ULL << sq;
    pos.bb[c][p] |= m;
    pos.occ[c] |= m;
    pos.allOcc |= m;
    pos.zobrist  ^= ZKEY[c][p][sq];
}
inline void removePiece(Position& pos, Colour c, Piece p, int sq) {
    uint64_t m = 1ULL << sq;
    pos.bb[c][p] &= ~m;
    pos.occ[c]   &= ~m;
    pos.allOcc   &= ~m;
    pos.zobrist  ^= ZKEY[c][p][sq];
}
inline bool isOccupied(const Position& pos, int sq) {
    return (pos.allOcc & (1ULL << sq)) != 0; // Check if the square is occupied by any piece
}
inline void move_piece(Position& pos, Colour c, Piece p, int from, int to) {
    Bitboard m = sq_bb(from) | sq_bb(to);   // Toggle both squares
    pos.bb[c][p] ^= m;
    pos.occ[c] ^= m;
    pos.allOcc ^= m;
    pos.zobrist  ^= ZKEY[c][p][from] ^ ZKEY[c][p][to];
}

/// Iterate every bit set in `bb` and call lambda f(int sq)
template <typename F>
inline void for_each(Bitboard bb, F&& f) {
    while (bb)
        f(pop_lsb(bb));
}
/*
for_each(pos.bb[WHITE][KNIGHT], [&](int from) {
    Bitboard moves = knightAttacks[from] & ~pos.occ[WHITE];
    for_each(moves, [&](int to) {
        // push move {from,to,KNIGHT}
    });
});
 */


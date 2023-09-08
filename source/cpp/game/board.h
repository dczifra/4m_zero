/*
 * board.h
 * Originally from an unreleased project back in 2010, modified since.
 * Authors: brettharrison (original), David Wu (original and later modifications).
 */

#ifndef GAME_BOARD_H_
#define GAME_BOARD_H_

#include "../core/global.h"
#include "../core/hash.h"

#ifndef COMPILE_MAX_BOARD_LEN
#define COMPILE_MAX_BOARD_LEN 20
#endif

#define RULE_M COMPILE_MAX_BOARD_LEN

#define TIDX(a) ((a) >> 6)
#define BITINDEX(a) ((a) - (((a) >> 6) << 6))
#define SUBBOARD(a) (1ULL << BITINDEX((a)))
#define EXPAND(a, s) (a.t[TIDX(s)] |= SUBBOARD(s))
#define REDUCE(a, s) (a.t[TIDX(s)] ^= SUBBOARD(s))
#define REDUCE_DELETE(a, s) (a.t[TIDX(s)] &= (~SUBBOARD(s)))
#define IS_ELEMENT(a, s) (bool(a.t[TIDX(s)] & SUBBOARD(s)))

struct BitBoard
{
    BitBoard();
    BitBoard(const BitBoard& other);
    BitBoard& operator=(const BitBoard&) = default;

	static const int index64[64];
	uint64_t t[2];
	
	inline void null()
	{
		this -> t[0] = this -> t[1] = 0; 
	}	
	inline bool operator!()
	{
		if (this -> t[0] != 0 || this -> t[1] != 0)
		{
			return true;
		}
		return false;
	}
	inline bool operator==(const BitBoard& rhs)
	{
		if (this -> t[0] == rhs.t[0] && this -> t[1] == rhs.t[1])
		{
			return true;
		}
		return false;
	}
	inline bool operator!=(const BitBoard& rhs)
	{
		if (this -> t[0] == rhs.t[0] && this -> t[1] == rhs.t[1])
		{
			return false;
		}
		return true;
	}				
	inline BitBoard operator|=(const BitBoard& rhs)
	{
		this -> t[0] |= rhs.t[0];
		this -> t[1] |= rhs.t[1];
		return * this;
	}
	inline BitBoard operator~()
	{
		BitBoard ret;
		ret.t[0] = (~(this->t[0]));
		ret.t[1] = (~(this->t[1]));
		return ret;
	}
	inline BitBoard operator&(const BitBoard& rhs)
	{
		BitBoard ret;
		ret.t[0] = (this -> t[0] & rhs.t[0]);
		ret.t[1] = (this -> t[1] & rhs.t[1]);
		return ret;
	}
	inline BitBoard operator|(const BitBoard& rhs)
	{
		BitBoard ret;
		ret.t[0] = (this -> t[0] | rhs.t[0]);
		ret.t[1] = (this -> t[1] | rhs.t[1]);
		return ret;
	}
	inline int bitScanForward() 
	{
		assert (operator!());
		if (t[0] != 0)
		{
			return index64[((t[0] ^ (t[0] - 1)) * (0x03f79d71b4cb0a89)) >> 58];
		}
		if (t[1] != 0)
		{
			return index64[((t[1] ^ (t[1] - 1)) * (0x03f79d71b4cb0a89)) >> 58] + 64;
		}
		return -1;
	}
	inline int count()
	{
		return __builtin_popcountll(t[0]) + __builtin_popcountll(t[1]);
	}

	void print(std::ostream& out) const
	{
		int m = RULE_M;
		int i;
		
		for (i = 0; i < 4 * m; i++)
		{
			if ((t[i >> 6] & (1ULL << (i - ((i >> 6) << 6)))) != 0)
			{
				out << "1 ";
				if (i % m == m - 1)
				{
					out << "\n";
				}
				continue;
			}
			out << "0 ";
			if (i % m == m - 1)
			{
				out <<"\n";
			}
		}
		out << "\n";
	}
};

// This will after inint and every times Board.colors is changing
struct State
{
	BitBoard O;
	BitBoard X;
	BitBoard winThreat; // Only O is threatening
	BitBoard forcingMoves[4];
	int winningSetsLeft;
	BitBoard legal;
};


//TYPES AND CONSTANTS-----------------------------------------------------------------

struct Board;

//Player
typedef int8_t Player;
static constexpr Player P_BLACK = 1;
static constexpr Player P_WHITE = 2;

//Color of a point on the board
typedef int8_t Color;
static constexpr Color C_EMPTY = 0;
static constexpr Color C_BLACK = 1;
static constexpr Color C_WHITE = 2;
static constexpr Color C_WALL = 3;
static constexpr int NUM_BOARD_COLORS = 4;

static inline Color getOpp(Color c)
{return c ^ 3;}

//Conversions for players and colors
namespace PlayerIO {
  char colorToChar(Color c);
  std::string playerToStringShort(Player p);
  std::string playerToString(Player p);
  bool tryParsePlayer(const std::string& s, Player& pla);
  Player parsePlayer(const std::string& s);
}

//Location of a point on the board
//(x,y) is represented as (x+1) + (y+1)*(x_size+1)
typedef short Loc;
namespace Location
{
  Loc getLoc(int x, int y, int x_size);
  int getX(Loc loc, int x_size);
  int getY(Loc loc, int x_size);

  void getAdjacentOffsets(short adj_offsets[8], int x_size);
  bool isAdjacent(Loc loc0, Loc loc1, int x_size);
  Loc getMirrorLoc(Loc loc, int x_size, int y_size);
  Loc getCenterLoc(int x_size, int y_size);
  bool isCentral(Loc loc, int x_size, int y_size);
  int distance(Loc loc0, Loc loc1, int x_size);
  int euclideanDistanceSquared(Loc loc0, Loc loc1, int x_size);

  std::string toString(Loc loc, int x_size, int y_size);
  std::string toString(Loc loc, const Board& b);
  std::string toStringMach(Loc loc, int x_size);
  std::string toStringMach(Loc loc, const Board& b);

  bool tryOfString(const std::string& str, int x_size, int y_size, Loc& result);
  bool tryOfString(const std::string& str, const Board& b, Loc& result);
  Loc ofString(const std::string& str, int x_size, int y_size);
  Loc ofString(const std::string& str, const Board& b);

  std::vector<Loc> parseSequence(const std::string& str, const Board& b);
}

//Simple structure for storing moves. Not used below, but this is a convenient place to define it.
STRUCT_NAMED_PAIR(Loc,loc,Player,pla,Move);

//Fast lightweight board designed for playouts and simulations, where speed is essential.
//Does not enforce player turn order.

struct Board
{
  //Initialization------------------------------
  //Initialize the zobrist hash.
  //MUST BE CALLED AT PROGRAM START!
  static void initBoardStruct();

  //Board parameters and Constants----------------------------------------

  static const int MAX_LEN = COMPILE_MAX_BOARD_LEN;  //Maximum edge length allowed for the board
  static const int MAX_PLAY_SIZE = MAX_LEN * MAX_LEN;  //Maximum number of playable spaces
  static const int MAX_ARR_SIZE = (MAX_LEN+1)*(MAX_LEN+2)+1; //Maximum size of arrays needed

  //Location used to indicate an invalid spot on the board.
  static const Loc NULL_LOC = 0;

  //Zobrist Hashing------------------------------
  static bool IS_INITALIZED;
  static Hash128 ZOBRIST_SIZE_X_HASH[MAX_LEN+1];
  static Hash128 ZOBRIST_SIZE_Y_HASH[MAX_LEN+1];
  static Hash128 ZOBRIST_BOARD_HASH[MAX_ARR_SIZE][4];
  static Hash128 ZOBRIST_PLAYER_HASH[4];
  static const Hash128 ZOBRIST_GAME_IS_OVER;

  //Structs---------------------------------------

  //Move data passed back when moves are made to allow for undos
  struct MoveRecord {
    Player pla;
    Loc loc;
  };

  //Constructors---------------------------------
  Board();  //Create Board of size (19,19)
  Board(int x, int y); //Create Board of size (x,y)
  Board(const Board& other);

  Board& operator=(const Board&) = default;

  //Functions------------------------------------
  //Check if moving here is legal.
  bool isLegal(Loc loc, Player pla) const;
  //Check if this location is on the board
  bool isOnBoard(Loc loc) const;
  //Check if this location is adjacent to stones of the specified color
  bool isAdjacentToPla(Loc loc, Player pla) const;
  bool isAdjacentOrDiagonalToPla(Loc loc, Player pla) const;
  //Is this board empty?
  bool isEmpty() const;
  //Is this board full?
  bool isFull() const;
  //Count the number of stones on the board
  int numStonesOnBoard() const;

  //Sets the specified stone if possible. Returns true usually, returns false location or color were out of range.
  bool setStone(Loc loc, Color color);

  //Attempts to play the specified move. Returns true if successful, returns false if the move was illegal.
  bool playMove(Loc loc, Player pla);

  //Plays the specified move, assuming it is legal.
  void playMoveAssumeLegal(Loc loc, Player pla);

  //Plays the specified move, assuming it is legal, and returns a MoveRecord for the move.
  MoveRecord playMoveRecorded(Loc loc, Player pla);

  //Undo the move given by record. Moves MUST be undone in the order they were made.
  void undo(MoveRecord record);

  //Get what the position hash would be if we were to play this move.
  //Assumes the move is on an empty location.
  Hash128 getPosHashAfterMove(Loc loc, Player pla) const;

  static Board parseBoard(int xSize, int ySize, const std::string& s);
  static Board parseBoard(int xSize, int ySize, const std::string& s, char lineDelimiter);
  static void printBoard(std::ostream& out, const Board& board, Loc markLoc, const std::vector<Move>* hist);
  static std::string toStringSimple(const Board& board, char lineDelimiter);
  static const std::vector<std::vector<BitBoard>>& getWinningSets();
  //Data--------------------------------------------

  int x_size;                  //Horizontal size of board
  int y_size;                  //Vertical size of board
  Color colors[MAX_ARR_SIZE];  //Color of each location on the board
  int num_stones;              //Number of stones on the board
  State state;                 //This could be incrementally computed

  Hash128 pos_hash; //A zobrist hash of the current board position (does not include player to move)
  short adj_offsets[8]; //Indices 0-3: Offsets to add for adjacent points. Indices 4-7: Offsets for diagonal points. 2 and 3 are +x and +y.

  private:
  void init(int xS, int yS);
  void removeSingleStone(Loc loc);
  void updateState();

  friend std::ostream& operator<<(std::ostream& out, const Board& board);
};

#endif // GAME_BOARD_H_

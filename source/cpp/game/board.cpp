#include "../game/board.h"

/*
 * board.cpp
 * Originally from an unreleased project back in 2010, modified since.
 * Authors: brettharrison (original), David Wu (original and later modificationss).
 */

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

#include "../core/rand.h"

using namespace std;

const int BitBoard::index64[64] = 
{
	0, 47,  1, 56, 48, 27,  2, 60,
	57, 49, 41, 37, 28, 16,  3, 61,
	54, 58, 35, 52, 50, 42, 21, 44,
	38, 32, 29, 23, 17, 11,  4, 62,
	46, 55, 26, 59, 40, 36, 15, 53,
	34, 51, 20, 43, 31, 22, 10, 45,
	25, 39, 14, 33, 19, 30,  9, 24,
	13, 18,  8, 12,  7,  6,  5, 63
};

BitBoard::BitBoard()
{
  null();
}

BitBoard::BitBoard(const BitBoard& other)
{
	memcpy(t, other.t, sizeof(uint64_t)*2);
}

//STATIC VARS-----------------------------------------------------------------------------
bool Board::IS_INITALIZED = false;
Hash128 Board::ZOBRIST_SIZE_X_HASH[MAX_LEN+1];
Hash128 Board::ZOBRIST_SIZE_Y_HASH[MAX_LEN+1];
Hash128 Board::ZOBRIST_BOARD_HASH[MAX_ARR_SIZE][4];
Hash128 Board::ZOBRIST_PLAYER_HASH[4];
const Hash128 Board::ZOBRIST_GAME_IS_OVER = //Based on sha256 hash of Board::ZOBRIST_GAME_IS_OVER
  Hash128(0xb6f9e465597a77eeULL, 0xf1d583d960a4ce7fULL);

//LOCATION--------------------------------------------------------------------------------
Loc Location::getLoc(int x, int y, int x_size)
{
  return (x+1) + (y+1)*(x_size+1);
}
int Location::getX(Loc loc, int x_size)
{
  return (loc % (x_size+1)) - 1;
}
int Location::getY(Loc loc, int x_size)
{
  return (loc / (x_size+1)) - 1;
}
void Location::getAdjacentOffsets(short adj_offsets[8], int x_size)
{
  adj_offsets[0] = -(x_size+1);
  adj_offsets[1] = -1;
  adj_offsets[2] = 1;
  adj_offsets[3] = (x_size+1);
  adj_offsets[4] = -(x_size+1)-1;
  adj_offsets[5] = -(x_size+1)+1;
  adj_offsets[6] = (x_size+1)-1;
  adj_offsets[7] = (x_size+1)+1;
}

bool Location::isAdjacent(Loc loc0, Loc loc1, int x_size)
{
  return loc0 == loc1 - (x_size+1) || loc0 == loc1 - 1 || loc0 == loc1 + 1 || loc0 == loc1 + (x_size+1);
}

Loc Location::getMirrorLoc(Loc loc, int x_size, int y_size) {
  if(loc == Board::NULL_LOC)
    return loc;
  return getLoc(x_size-1-getX(loc,x_size),y_size-1-getY(loc,x_size),x_size);
}

Loc Location::getCenterLoc(int x_size, int y_size) {
  if(x_size % 2 == 0 || y_size % 2 == 0)
    return Board::NULL_LOC;
  return getLoc(x_size / 2, y_size / 2, x_size);
}

bool Location::isCentral(Loc loc, int x_size, int y_size) {
  int x = getX(loc,x_size);
  int y = getY(loc,x_size);
  return x >= (x_size-1)/2 && x <= x_size/2 && y >= (y_size-1)/2 && y <= y_size/2;
}


#define FOREACHADJ(BLOCK) {int ADJOFFSET = -(x_size+1); {BLOCK}; ADJOFFSET = -1; {BLOCK}; ADJOFFSET = 1; {BLOCK}; ADJOFFSET = x_size+1; {BLOCK}};
#define ADJ0 (-(x_size+1))
#define ADJ1 (-1)
#define ADJ2 (1)
#define ADJ3 (x_size+1)

//CONSTRUCTORS AND INITIALIZATION----------------------------------------------------------

Board::Board()
{
  init(RULE_M, 4);
}

Board::Board(int /*x*/, int /*y*/)
{
  init(RULE_M, 4);
}

Board::Board(const Board& other)
{
  x_size = other.x_size;
  y_size = other.y_size;

  memcpy(colors, other.colors, sizeof(Color)*MAX_ARR_SIZE);
  num_stones = other.num_stones;
  pos_hash = other.pos_hash;
  state = other.state;

  memcpy(adj_offsets, other.adj_offsets, sizeof(short)*8);
}

void Board::init(int xS, int yS)
{
  assert(IS_INITALIZED);
  if(xS < 0 || yS < 0 || xS > MAX_LEN || yS > MAX_LEN)
    throw StringError("Board::init - invalid board size");

  x_size = xS;
  y_size = yS;

  for(int i = 0; i < MAX_ARR_SIZE; i++)
    colors[i] = C_WALL;

  for(int y = 0; y < y_size; y++)
  {
    for(int x = 0; x < x_size; x++)
    {
      Loc loc = (x+1) + (y+1)*(x_size+1);
      colors[loc] = C_EMPTY;
      // empty_list.add(loc);
    }
  }

  num_stones = 0;
  pos_hash = ZOBRIST_SIZE_X_HASH[x_size] ^ ZOBRIST_SIZE_Y_HASH[y_size];
  updateState();
  Location::getAdjacentOffsets(adj_offsets,x_size);
}

void Board::initBoardStruct()
{
  if(IS_INITALIZED)
    return;
  Rand rand("Board::initHash()");

  auto nextHash = [&rand]() {
    uint64_t h0 = rand.nextUInt64();
    uint64_t h1 = rand.nextUInt64();
    return Hash128(h0,h1);
  };

  for(int i = 0; i<4; i++)
    ZOBRIST_PLAYER_HASH[i] = nextHash();

  //Do this second so that the player hashes are not
  //afffected by the size of the board we compile with.
  for(int i = 0; i<MAX_ARR_SIZE; i++)
  {
    for(Color j = 0; j<4; j++)
    {
      if(j == C_EMPTY || j == C_WALL)
        ZOBRIST_BOARD_HASH[i][j] = Hash128();
      else
        ZOBRIST_BOARD_HASH[i][j] = nextHash();
    }
  }

  //Reseed the random number generator so that these size hashes are also
  //not affected by the size of the board we compile with
  rand.init("Board::initHash() for ZOBRIST_SIZE hashes");
  for(int i = 0; i<MAX_LEN+1; i++) {
    ZOBRIST_SIZE_X_HASH[i] = nextHash();
    ZOBRIST_SIZE_Y_HASH[i] = nextHash();
  }
  getWinningSets();
  IS_INITALIZED = true;
}

static std::vector<std::vector<BitBoard>> getWinningSetsImpl()
{
	// HAS TO BE IMPLEMENTED
	int m = RULE_M;
	std::vector<std::vector<BitBoard>> ret(4);

	//0 == E, 1 == N, 2 == NE, 3 == NW
	// E
	std::vector<BitBoard> curr;
	curr.clear();
	for(int y = 0; y<4; y++) {
      for(int x = 0; x<m; x++) {
		BitBoard currWinningSet;
		currWinningSet.null();
		if (x + 6 <= m - 1)
		{
			for (int t = 0; t < 7; t++)
			{
				EXPAND(currWinningSet, y * m + x + t);
			}
			curr.push_back(currWinningSet);
		}
      }
    }
	ret[0] = curr;

	// N
	curr.clear();
	for(int x = 0; x<m; x++) {
		BitBoard currWinningSet;
		currWinningSet.null();
		for (int t = 0; t < 4; ++t)
		{
			EXPAND(currWinningSet, x + t * m);
		}
		curr.push_back(currWinningSet);
    }
	ret[1] = curr;

	// NE
	curr.clear();
	for(int x = 3; x<m; x++) {
		BitBoard currWinningSet;
		currWinningSet.null();
		for (int t = 0; t < 4; ++t)
		{
			EXPAND(currWinningSet, x + (m - 1) * t);
		}
		curr.push_back(currWinningSet);
    }
	ret[2] = curr;

	// NW
	curr.clear();
	for(int x = 0; x<= m - 4; x++) {
		BitBoard currWinningSet;
		currWinningSet.null();
		for (int t = 0; t < 4; ++t)
		{
			EXPAND(currWinningSet, x + (m + 1) * t);
		}
		curr.push_back(currWinningSet);
    }
	ret[3] = curr;

	// Shorts
	// E 
	for(int y = 0; y<4; y++) {
		BitBoard currWinningSet;
		currWinningSet.null();
		for (int t = 0; t < 4; t++)
		{
			EXPAND(currWinningSet, y * m + t);
		}
		ret[0].push_back(currWinningSet);
		currWinningSet.null();
		for (int t = 0; t < 4; t++)
		{
			EXPAND(currWinningSet, m - 4 + y * m + t);
		}		
		ret[0].push_back(currWinningSet);
    }
	// NE
	BitBoard currWinningSet;
	currWinningSet.null();
	EXPAND(currWinningSet, 1);
	EXPAND(currWinningSet, m);
	ret[2].push_back(currWinningSet);
	currWinningSet.null();
	EXPAND(currWinningSet, 2);
	EXPAND(currWinningSet, m + 1);
	EXPAND(currWinningSet, 2 * m);
	ret[2].push_back(currWinningSet);
	currWinningSet.null();
	EXPAND(currWinningSet, m + m - 1);
	EXPAND(currWinningSet, m + 2 * m - 2);
	EXPAND(currWinningSet, m + 3 * m - 3);
	ret[2].push_back(currWinningSet);

	// NW
	currWinningSet.null();
	EXPAND(currWinningSet, m - 2);
	EXPAND(currWinningSet, 2 * m - 1);
	ret[3].push_back(currWinningSet);
	currWinningSet.null();
	EXPAND(currWinningSet, m - 3);
	EXPAND(currWinningSet, 2 * m - 2);
	EXPAND(currWinningSet, 3 * m - 1);
	ret[3].push_back(currWinningSet);
	currWinningSet.null();
	EXPAND(currWinningSet, m);
	EXPAND(currWinningSet, 2 * m + 1);
	EXPAND(currWinningSet, 3 * m + 2);
	ret[3].push_back(currWinningSet);
	return ret;	
}

const std::vector<std::vector<BitBoard>>& Board::getWinningSets()
{
	static std::vector<std::vector<BitBoard>> ret = getWinningSetsImpl();
	return ret;
}

bool Board::isOnBoard(Loc loc) const {
  return loc >= 0 && loc < MAX_ARR_SIZE && colors[loc] != C_WALL;
}

//Check if moving here is legal.
bool Board::isLegal(Loc loc, Player /*pla*/) const
{
	if (isOnBoard(loc) == false)
	{
		return false;
	}
	if (colors[loc] != C_EMPTY)
	{
		return false;
	}
	return true;
}

bool Board::isAdjacentToPla(Loc loc, Player pla) const {
  FOREACHADJ(
    Loc adj = loc + ADJOFFSET;
    if(colors[adj] == pla)
      return true;
  );
  return false;
}

bool Board::isAdjacentOrDiagonalToPla(Loc loc, Player pla) const {
  for(int i = 0; i<8; i++) {
    Loc adj = loc + adj_offsets[i];
    if(colors[adj] == pla)
      return true;
  }
  return false;
}

bool Board::isEmpty() const {
  for(int y = 0; y < y_size; y++) {
    for(int x = 0; x < x_size; x++) {
      Loc loc = Location::getLoc(x,y,x_size);
      if(colors[loc] != C_EMPTY)
        return false;
    }
  }
  return true;
}

bool Board::isFull() const {
  return (num_stones == (x_size * y_size));
}

int Board::numStonesOnBoard() const {
  int num = 0;
  for(int y = 0; y < y_size; y++) {
    for(int x = 0; x < x_size; x++) {
      Loc loc = Location::getLoc(x,y,x_size);
      if(colors[loc] == C_BLACK || colors[loc] == C_WHITE)
        num += 1;
    }
  }
  return num;
}

bool Board::setStone(Loc loc, Color color)
{
  if(loc < 0 || loc >= MAX_ARR_SIZE || colors[loc] == C_WALL)
    return false;
  if(color != C_BLACK && color != C_WHITE && color != C_EMPTY)
    return false;

  if(colors[loc] == color)
  {}
  else if(colors[loc] == C_EMPTY)
    playMoveAssumeLegal(loc,color);
  else if(color == C_EMPTY)
    removeSingleStone(loc);
  else {
    removeSingleStone(loc);
    playMoveAssumeLegal(loc,color);
  }

  return true;
}

//Remove a single stone
void Board::removeSingleStone(Loc loc)
{
  pos_hash ^= ZOBRIST_BOARD_HASH[loc][colors[loc]];
  colors[loc] = C_EMPTY;
  num_stones -= 1;
  updateState();
}


//Attempts to play the specified move. Returns true if successful, returns false if the move was illegal.
bool Board::playMove(Loc loc, Player pla)
{
  if(isLegal(loc,pla))
  {
    playMoveAssumeLegal(loc,pla);
    return true;
  }
  return false;
}

//Plays the specified move, assuming it is legal, and returns a MoveRecord for the move
Board::MoveRecord Board::playMoveRecorded(Loc loc, Player pla)
{
  MoveRecord record;
  record.loc = loc;
  record.pla = pla;

  playMoveAssumeLegal(loc, pla);
  return record;
}

//Undo the move given by record. Moves MUST be undone in the order they were made.
void Board::undo(Board::MoveRecord record)
{
  Loc loc = record.loc;

  //Delete the stone played here.
  pos_hash ^= ZOBRIST_BOARD_HASH[loc][colors[loc]];
  colors[loc] = C_EMPTY;
  num_stones -= 1;
  updateState();
}

Hash128 Board::getPosHashAfterMove(Loc loc, Player pla) const {
  assert(loc != NULL_LOC);

  Hash128 hash = pos_hash;
  hash ^= ZOBRIST_BOARD_HASH[loc][pla];

  return hash;
}

//Plays the specified move, assuming it is legal.
void Board::playMoveAssumeLegal(Loc loc, Player pla)
{
  //Add the new stone 
  colors[loc] = pla;
  // calc states
  pos_hash ^= ZOBRIST_BOARD_HASH[loc][pla];
  num_stones += 1;
  updateState();
}

int Location::distance(Loc loc0, Loc loc1, int x_size) {
  int dx = getX(loc1,x_size) - getX(loc0,x_size);
  int dy = (loc1-loc0-dx) / (x_size+1);
  return (dx >= 0 ? dx : -dx) + (dy >= 0 ? dy : -dy);
}

int Location::euclideanDistanceSquared(Loc loc0, Loc loc1, int x_size) {
  int dx = getX(loc1,x_size) - getX(loc0,x_size);
  int dy = (loc1-loc0-dx) / (x_size+1);
  return dx*dx + dy*dy;
}

//TACTICAL STUFF--------------------------------------------------------------------


//IO FUNCS------------------------------------------------------------------------------------------

char PlayerIO::colorToChar(Color c)
{
  switch(c) {
  case C_BLACK: return 'O';
  case C_WHITE: return 'X';
  case C_EMPTY: return '.';
  default:  return '#';
  }
}

string PlayerIO::playerToString(Color c)
{
  switch(c) {
  case C_BLACK: return "Black";
  case C_WHITE: return "White";
  case C_EMPTY: return "Empty";
  default:  return "Wall";
  }
}

string PlayerIO::playerToStringShort(Color c)
{
  switch(c) {
  case C_BLACK: return "B";
  case C_WHITE: return "W";
  case C_EMPTY: return "E";
  default:  return "";
  }
}

bool PlayerIO::tryParsePlayer(const string& s, Player& pla) {
  string str = Global::toLower(s);
  if(str == "black" || str == "b") {
    pla = P_BLACK;
    return true;
  }
  else if(str == "white" || str == "w") {
    pla = P_WHITE;
    return true;
  }
  return false;
}

Player PlayerIO::parsePlayer(const string& s) {
  Player pla = C_EMPTY;
  bool suc = tryParsePlayer(s,pla);
  if(!suc)
    throw StringError("Could not parse player: " + s);
  return pla;
}

string Location::toStringMach(Loc loc, int x_size)
{
  if(loc == Board::NULL_LOC)
    return string("null");
  char buf[128];
  sprintf(buf,"(%d,%d)",getX(loc,x_size),getY(loc,x_size));
  return string(buf);
}

string Location::toString(Loc loc, int x_size, int y_size)
{
  if(x_size > 25*25)
    return toStringMach(loc,x_size);
  if(loc == Board::NULL_LOC)
    return string("null");
  const char* xChar = "ABCDEFGHJKLMNOPQRSTUVWXYZ";
  int x = getX(loc,x_size);
  int y = getY(loc,x_size);
  if(x >= x_size || x < 0 || y < 0 || y >= y_size)
    return toStringMach(loc,x_size);

  char buf[128];
  if(x <= 24)
    sprintf(buf,"%c%d",xChar[x],y_size-y);
  else
    sprintf(buf,"%c%c%d",xChar[x/25-1],xChar[x%25],y_size-y);
  return string(buf);
}

string Location::toString(Loc loc, const Board& b) {
  return toString(loc,b.x_size,b.y_size);
}

string Location::toStringMach(Loc loc, const Board& b) {
  return toStringMach(loc,b.x_size);
}

static bool tryParseLetterCoordinate(char c, int& x) {
  if(c >= 'A' && c <= 'H')
    x = c-'A';
  else if(c >= 'a' && c <= 'h')
    x = c-'a';
  else if(c >= 'J' && c <= 'Z')
    x = c-'A'-1;
  else if(c >= 'j' && c <= 'z')
    x = c-'a'-1;
  else
    return false;
  return true;
}

bool Location::tryOfString(const string& str, int x_size, int y_size, Loc& result) {
  string s = Global::trim(str);
  if(s.length() < 2)
    return false;
  if(Global::isEqualCaseInsensitive(s,string("pass")) || Global::isEqualCaseInsensitive(s,string("pss"))) {
    result = Board::NULL_LOC;
    return true;
  }
  if(s[0] == '(') {
    if(s[s.length()-1] != ')')
      return false;
    s = s.substr(1,s.length()-2);
    vector<string> pieces = Global::split(s,',');
    if(pieces.size() != 2)
      return false;
    int x;
    int y;
    bool sucX = Global::tryStringToInt(pieces[0],x);
    bool sucY = Global::tryStringToInt(pieces[1],y);
    if(!sucX || !sucY)
      return false;
    result = Location::getLoc(x,y,x_size);
    return true;
  }
  else {
    int x;
    if(!tryParseLetterCoordinate(s[0],x))
      return false;

    //Extended format
    if((s[1] >= 'A' && s[1] <= 'Z') || (s[1] >= 'a' && s[1] <= 'z')) {
      int x1;
      if(!tryParseLetterCoordinate(s[1],x1))
        return false;
      x = (x+1) * 25 + x1;
      s = s.substr(2,s.length()-2);
    }
    else {
      s = s.substr(1,s.length()-1);
    }

    int y;
    bool sucY = Global::tryStringToInt(s,y);
    if(!sucY)
      return false;
    y = y_size - y;
    if(x < 0 || y < 0 || x >= x_size || y >= y_size)
      return false;
    result = Location::getLoc(x,y,x_size);
    return true;
  }
}

bool Location::tryOfString(const string& str, const Board& b, Loc& result) {
  return tryOfString(str,b.x_size,b.y_size,result);
}

Loc Location::ofString(const string& str, int x_size, int y_size) {
  Loc result;
  if(tryOfString(str,x_size,y_size,result))
    return result;
  throw StringError("Could not parse board location: " + str);
}

Loc Location::ofString(const string& str, const Board& b) {
  return ofString(str,b.x_size,b.y_size);
}

vector<Loc> Location::parseSequence(const string& str, const Board& board) {
  vector<string> pieces = Global::split(Global::trim(str),' ');
  vector<Loc> locs;
  for(size_t i = 0; i<pieces.size(); i++) {
    string piece = Global::trim(pieces[i]);
    if(piece.length() <= 0)
      continue;
    locs.push_back(Location::ofString(piece,board));
  }
  return locs;
}

void Board::printBoard(ostream& out, const Board& board, Loc markLoc, const vector<Move>* hist) {
  if(hist != NULL)
    out << "MoveNum: " << hist->size() << " ";
  out << "HASH: " << board.pos_hash << "\n";
  bool showCoords = board.x_size <= 50 && board.y_size <= 50;
  if(showCoords) {
    const char* xChar = "ABCDEFGHJKLMNOPQRSTUVWXYZ";
    out << "  ";
    for(int x = 0; x < board.x_size; x++) {
      if(x <= 24) {
        out << " ";
        out << xChar[x];
      }
      else {
        out << "A" << xChar[x-25];
      }
    }
    out << "\n";
  }

  for(int y = 0; y < board.y_size; y++)
  {
    if(showCoords) {
      char buf[16];
      sprintf(buf,"%2d",board.y_size-y);
      out << buf << ' ';
    }
    for(int x = 0; x < board.x_size; x++)
    {
      Loc loc = Location::getLoc(x,y,board.x_size);
      char s = PlayerIO::colorToChar(board.colors[loc]);
      if(board.colors[loc] == C_EMPTY && markLoc == loc)
        out << '@';
      else
        out << s;

      bool histMarked = false;
      if(hist != NULL) {
        for(int i = (int)hist->size()-3; i<hist->size(); i++) {
          if(i >= 0 && (*hist)[i].loc == loc) {
            out << i - (hist->size()-3) + 1;
            histMarked = true;
            break;
          }
        }
      }

      if(x < board.x_size-1 && !histMarked)
        out << ' ';
    }
    out << "\n";
  }
  //board.state.O.print(out);
  //const auto& t = Board::getWinningSets();
  const auto& s = board.state;
  out << "O=\n";
  s.O.print(out);
  out << "X=\n";
  s.X.print(out);
  out << "winThreat=";
  s.winThreat.print(out);
  for (int dir = 0; dir < 4; dir++)
  {
    out << "for dir=" << dir << " forcingMoves=\n";
    s.forcingMoves[dir].print(out);
  }
  out << "winningSetsLeft=" << s.winningSetsLeft << "\n";
  out << "legal=";
  s.legal.print(out);
  out << "OFork=" << s.OFork << "\n";
}

ostream& operator<<(ostream& out, const Board& board) {
  Board::printBoard(out,board,Board::NULL_LOC,NULL);
  return out;
}


string Board::toStringSimple(const Board& board, char lineDelimiter) {
  string s;
  for(int y = 0; y < board.y_size; y++) {
    for(int x = 0; x < board.x_size; x++) {
      Loc loc = Location::getLoc(x,y,board.x_size);
      s += PlayerIO::colorToChar(board.colors[loc]);
    }
    s += lineDelimiter;
  }
  return s;
}

Board Board::parseBoard(int xSize, int ySize, const string& s) {
  return parseBoard(xSize,ySize,s,'\n');
}

Board Board::parseBoard(int xSize, int ySize, const string& s, char lineDelimiter) {
  Board board(xSize,ySize);
  vector<string> lines = Global::split(Global::trim(s),lineDelimiter);

  //Throw away coordinate labels line if it exists
  if(lines.size() == ySize+1 && Global::isPrefix(lines[0],"A"))
    lines.erase(lines.begin());

  if(lines.size() != ySize)
    throw StringError("Board::parseBoard - string has different number of board rows than ySize");

  for(int y = 0; y<ySize; y++) {
    string line = Global::trim(lines[y]);
    //Throw away coordinates if they exist
    size_t firstNonDigitIdx = 0;
    while(firstNonDigitIdx < line.length() && Global::isDigit(line[firstNonDigitIdx]))
      firstNonDigitIdx++;
    line.erase(0,firstNonDigitIdx);
    line = Global::trim(line);

    if(line.length() != xSize && line.length() != 2*xSize-1)
      throw StringError("Board::parseBoard - line length not compatible with xSize");

    for(int x = 0; x<xSize; x++) {
      char c;
      if(line.length() == xSize)
        c = line[x];
      else
        c = line[x*2];

      Loc loc = Location::getLoc(x,y,board.x_size);
      if(c == '.' || c == ' ' || c == '*' || c == ',' || c == '`')
        continue;
      else if(c == 'o' || c == 'O')
        board.setStone(loc,P_WHITE);
      else if(c == 'x' || c == 'X')
        board.setStone(loc,P_BLACK);
      else
        throw StringError(string("Board::parseBoard - could not parse board character: ") + c);
    }
  }
  return board;
}

void Board::updateState()
{
	// HAS TO BE IMPLEMENTED
	const auto& winningSets = getWinningSets();
	int p = num_stones % 2; // player on move 0 = O, 1 = X
    state.O.null();
	state.X.null();
	state.winThreat.null();
	for (int i = 0; i < 4; i++)
	{
		state.forcingMoves[i].null();
	}
	state.winningSetsLeft = 0;
	state.legal.null();
  state.OFork = false;
	
  BitBoard empty;
  empty.null();
	for(int y = 0; y<y_size; y++) {
      for(int x = 0; x<x_size; x++) {
        Loc loc = Location::getLoc(x,y,x_size);
        if(colors[loc] == C_BLACK)
          EXPAND(state.O, y * x_size + x);
        else if (colors[loc] == C_WHITE)
          EXPAND(state.X, y * x_size + x);
        else
          EXPAND(empty, y * x_size + x);
      }
    }
	for (int dir = 0; dir < 4; dir++)
	{
		for (const auto& winningSet : winningSets[dir])
		{
      
      if (!(state.X & winningSet))
      {
        state.winningSetsLeft++;
        int winningSetSize = winningSet.count();
        BitBoard intersect = (state.O & winningSet);
        int intersectSize = intersect.count();
        if (intersectSize == winningSetSize - 1)
        {
          if (p == 1)
          {
            state.winThreat |= (empty & winningSet); 
          }
        }
        if (intersectSize == winningSetSize - 2)
        {
          state.forcingMoves[dir] |= (empty & winningSet);
        }
      }
		}
	}

  if (p == 0)
  {
    for (int d1 = 0; d1 < 4; d1++)
    {
      for (int d2 = d1 + 1; d2 < 4; d2++)
      {
        auto fork = (state.forcingMoves[d1] & state.forcingMoves[d2]);
        if (!(!fork))
        {
          state.OFork = true;
        }
      }
    }
    if (state.winningSetsLeft == 0)
    {
      state.legal.null();
    }
    else
    {
      state.legal = empty;
    }
  }
  if (p == 1)
  {
    state.OFork = false;
    int w = state.winThreat.count();
    if (w == 0)
    {
      state.legal = empty;
    }
    else if (w == 1)
    {
      state.legal = state.winThreat;
    }
    else{
      state.legal.null();
    }
  }
	return;
}
#include<iostream>
using namespace std;

#include <cassert>
#include <sstream>
#include <limits>

//第一步：设计棋盘类BOARD
constexpr int BOARD_SIZE = 12;
enum Cell
{
	EMPTY,
	BLACK,
	WHITE
};
class Board
{
private:
	Cell cells[BOARD_SIZE][BOARD_SIZE];
public:
	Board()
	{
		Initialize();
	}
	void Initialize()
	{
		for (int i = 0; i < BOARD_SIZE; ++i)
		{
			for (int j = 0; j < BOARD_SIZE; ++j)
			{
				cells[i][j] = EMPTY;
			}
		}
		cells[5][5] = WHITE;
		cells[5][6] = BLACK;
		cells[6][5] = BLACK;
		cells[6][6] = WHITE;
	}
	bool inBounds(int row, int col) const
	{
		return row >= 0 && row < BOARD_SIZE &&
			col >= 0 && col < BOARD_SIZE;
	}
	bool isValid(int row, int col) const
	{
		return inBounds(row, col) &&
			cells[row][col] == EMPTY;
	}
	bool place(int row, int col, Cell color)
	{
		if (color != BLACK && color != WHITE) return false;
		if (!isValid(row, col)) return false;
		else cells[row][col] = color;
		return true;
	}

	Cell getCell(int row, int col) const
	{
		assert(inBounds(row, col));
			return cells[row][col];
	}
	bool checkWin(int row, int col) const
	{
		if (row < 0 || row >= BOARD_SIZE ||
			col < 0 || col >= BOARD_SIZE)
			return false;

		Cell color = cells[row][col];

		if (color == EMPTY)
			return false;
		auto Count = [&](int dx, int dy,int limit)->int
			{
				int count = 0;
				int nx = row + dx;
				int ny = col + dy;

				while (count < limit && nx >= 0 && nx < BOARD_SIZE &&
					ny >= 0 && ny < BOARD_SIZE && cells[nx][ny] == color)
				{
					++count;
					nx += dx;
					ny += dy;
				}
				return count;
			};

		const int directions[4][2] = {
			{ 0, 1 },
			{ 1, 0 },
			{ 1, 1 },
			{ 1, -1 }
		};

		for (const auto& dir : directions)
		{
			int count = 1;
			count += Count(dir[0], dir[1],4);
			if (count >= 5)
				return true;

			count += Count(-dir[0], -dir[1], 5 - count);

			if (count >= 5)
				return true;
		}
		return false;
	}
	void undo(int row, int col)
	{
		if (row < 0 || row >= BOARD_SIZE ||
			col < 0 || col >= BOARD_SIZE)
			return;
		cells[row][col] = EMPTY;
	}
	bool isFull() const
	{
		for (int i = 0; i < BOARD_SIZE; ++i)
		{
			for (int j = 0; j < BOARD_SIZE; ++j)
			{
				if (cells[i][j] == EMPTY) return false;
			}
		}
		return true;
	}
};
//第二步：建立交互
struct Move//表示五子棋中的一步棋
{
	int row;
	int col;
};
class GomokuAI//负责思考方式的算法模块
{
public:
	/*遍历候选位置
      ↓
计算每个位置的综合评分
      ↓
记录当前最高分
      ↓
选择最高分对应的位置*/
	Move chooseMove(const Board& board, Cell myColor) const
	{
		if (myColor != BLACK && myColor != WHITE)
		{
			return Move{ -1,-1 };
		}
		Cell opponentColor = (myColor == BLACK) ? WHITE : BLACK;
		//第一优先级：寻找自己能赢的位置
		for (int i = 0; i < BOARD_SIZE; ++i)
		{
			for (int j = 0; j < BOARD_SIZE; ++j)
			{
				if (wouldWin(board, i, j, myColor))
					return Move{ i, j };
			}
		}
		//第二优先级：防止对面获胜
		for (int i = 0; i < BOARD_SIZE; ++i)
		{
			for (int j = 0; j < BOARD_SIZE; ++j)
			{
				if (wouldWin(board, i, j, opponentColor))
					return Move{ i, j };
			}
		}
		//第三优先级：遍历候选位置进行评分
		double maxScore = numeric_limits<double>::lowest();
		Move bestMove = { -1,-1 };
		for (int i = 0; i < BOARD_SIZE; ++i)
		{
			for (int j = 0; j < BOARD_SIZE; ++j)
			{
				if (!board.isValid(i, j))
					continue;
				double score = evaluateMove(board, i, j, myColor);
				if (score > maxScore)
				{
					maxScore = score;
					bestMove = { i,j };
				}
			}
		}
		
		return bestMove;
	}
	bool wouldWin(const Board& board, const int row, const int col, Cell Color) const
	{
		if (Color != BLACK && Color != WHITE)
			return false;
		Board simulationboard = board;
		if (!simulationboard.isValid(row, col))
			return false;
		if (!simulationboard.place(row, col, Color))
		{
			return false;
		}
		return simulationboard.checkWin(row, col);
	}
	struct LineInfo
	{
		int count;
		int openEnds;
	};
	LineInfo analyzeDirection(const Board& board, int row, int col, int dx, int dy, Cell color) const
	{
		int count = 1;
		int openEnds = 0;

		for (int sign : {-1, 1})
		{
			int nx = row + sign * dx;
			int ny = col + sign * dy;

			while (board.inBounds(nx, ny)&&board.getCell(nx,ny) == color)//寻找棋盘范围内的连续棋子
			{
				++count;
				nx += sign * dx;
				ny += sign * dy;
			}
			//检查连续棋子末端是否空置
			if (board.inBounds(nx, ny) && board.getCell(nx, ny) == EMPTY)
			{
				++openEnds;
			}
		}
		return { count,openEnds };
	}
	string extractLine(const Board& board, int row, int col, int dx, int dy, Cell color) const//将选定位置的周边行列按照字符串形式表示
	{
		string line;
		Cell opponentColor = (color == BLACK) ? WHITE : BLACK;
		for (int offset = -4;offset <= 4;++offset)
		{
			int nx = row + offset * dx;
			int ny = col + offset * dy;
			//边界用#表示
			if (!board.inBounds(nx,ny)) line.push_back('#');
			//空位用_表示
			else if (board.isValid(nx, ny)) line.push_back('_');
			//判断棋子:O为对方，X为己方
			else
			{
				(board.getCell(nx, ny) == color) ? line.push_back('X') : line.push_back('O');
			}
		}
		return line;
	}
	enum class Pattern
	{
		NONE,
		SLEEP_TWO,
		OPEN_TWO,
		SLEEP_THREE,
		JUMP_THREE,
		OPEN_THREE,
		RUSH_FOUR,
		JUMP_FOUR,
		OPEN_FOUR,
		FIVE
	};
	int patternScore(Pattern pattern) const
	{
		switch (pattern)
		{
		case Pattern::FIVE:
			return 1000000;

		case Pattern::OPEN_FOUR:
			return 100000;

		case Pattern::RUSH_FOUR:
		case Pattern::JUMP_FOUR:
			return 50000;

		case Pattern::OPEN_THREE:
			return 10000;

		case Pattern::JUMP_THREE:
			return 3000;

		case Pattern::SLEEP_THREE:
			return 1000;

		case Pattern::OPEN_TWO:
			return 200;

		case Pattern::SLEEP_TWO:
			return 50;

		default:
			return 0;
		}
	}
	bool hasPatternAtCenter(const string& line, const string& pattern) const
	{
		const int center = 4;
		for (int start = 0;start + pattern.size() <= line.size();++start)
		{
			int end = start + pattern.size() - 1;
			if (start > center || end < center) continue;
			if (line.compare(start, pattern.size(), pattern)==0)
			{
				return true;
			}
		}
		return false;
	}

	Pattern classifyPattern(const string& line) const
	{
		if (line.size() != 9 || line[4] != 'X')
			return Pattern::NONE;

		// 第一优先级：已经形成五连
		if (hasPatternAtCenter(line, "XXXXX"))
			return Pattern::FIVE;

		// 第二优先级：活四
		if (hasPatternAtCenter(line, "_XXXX_"))
			return Pattern::OPEN_FOUR;

		// 第三优先级：跳四
		if (hasPatternAtCenter(line, "XX_XX") ||
			hasPatternAtCenter(line, "X_XXX") ||
			hasPatternAtCenter(line, "XXX_X"))
			return Pattern::JUMP_FOUR;

		// 第四优先级：冲四
		if (hasPatternAtCenter(line, "XXXX_") ||
			hasPatternAtCenter(line, "_XXXX"))
			return Pattern::RUSH_FOUR;

		// 第五优先级：活三
		if (hasPatternAtCenter(line, "_XXX_"))
			return Pattern::OPEN_THREE;

		// 第六优先级：跳三（初步模板）
		if (hasPatternAtCenter(line, "XX_X_") ||
			hasPatternAtCenter(line, "_X_XX") ||
			hasPatternAtCenter(line, "X_XX_") ||
			hasPatternAtCenter(line, "_XX_X"))
			return Pattern::JUMP_THREE;

		// 第七优先级：眠三
		if (hasPatternAtCenter(line, "XXX_") ||
			hasPatternAtCenter(line, "_XXX"))
			return Pattern::SLEEP_THREE;

		// 第八优先级：活二
		if (hasPatternAtCenter(line, "_XX_"))
			return Pattern::OPEN_TWO;

		// 第九优先级：眠二
		if (hasPatternAtCenter(line, "XX_") ||
			hasPatternAtCenter(line, "_XX"))
			return Pattern::SLEEP_TWO;

		return Pattern::NONE;
	}
	int evaluateAttackMove(const Board& board, int row, int col, Cell color) const//模拟 提取 识别 评分
	{
		if (color != BLACK && color != WHITE)
			return -1;

		if (!board.isValid(row, col))
			return -1;

		Board simulationBoard = board;

		if (!simulationBoard.place(row, col, color))
			return -1;
		const int directions[4][2] =
		{
			{ 1,0 },
			{ 0,1 },
			{ 1,1 },
			{ 1,-1 }
		};
		int maxscore=0;
		for (auto direction:directions)
		{
			Pattern p = classifyPattern(extractLine(simulationBoard, row, col, direction[0], direction[1], color));
			maxscore += patternScore(p);
		}
		return maxscore;
	}
	double evaluateMove(const Board& board, int row, int col, Cell myColor) const
	{
		Cell opponentColor = (myColor == BLACK) ? WHITE : BLACK;
		constexpr double e = 0.8;
		//最终评分=进攻+防守*防守系数
		return evaluateAttackMove(board, row, col, myColor) + e * evaluateAttackMove(board, row, col, opponentColor);
	}
};
class GomokuGame//交互模块
{
private:
	Board board;
	GomokuAI ai;

	Cell myColor = EMPTY;
	void handleStart(int color)
	{
		if (color != 1 && color != 2)
			return;
		myColor = static_cast<Cell>(color);
		board.Initialize();
		cout << "OK\n" << flush;
	}
	void handlePlace(int row, int col)//对手落子
	{
		Cell opponentColor = (myColor == BLACK) ? WHITE : BLACK;
		board.place(row, col, opponentColor);
	}
	void handleTurn()
	{
		Move move = ai.chooseMove(board, myColor);
		if (move.col == -1 || move.row == -1)
		{
			return;
		}
		if (!board.place(move.row, move.col, myColor))
		{
			return;
		}
		cout<< move.row << " " << move.col << '\n' << flush;
	}//负责调用ai，更新己方棋盘和输出坐标
public:
	void run()
	{
		string command;
		while (cin >> command)
		{
			if (command == "START")
			{
				int color;
				cin >> color;
				handleStart(color);
			}
			else if (command == "PLACE")
			{
				int row, col;
				cin >> row >> col;
				handlePlace(row, col);
			}
			else if (command =="TURN")
			{
				handleTurn();
			}
			else if (command == "END")
			{
				break;
			}
		}
	}
};
void testProtocol()
{
	// 模拟平台输入
	istringstream input(
		"START 1\n"
		"TURN\n"
		"PLACE 0 1\n"
		"TURN\n"
		"END\n"
	);

	// 捕获程序输出
	ostringstream output;

	// 暂时替换 cin 和 cout 的缓冲区
	streambuf* oldCin = cin.rdbuf(input.rdbuf());
	streambuf* oldCout = cout.rdbuf(output.rdbuf());

	{
		GomokuGame game;
		game.run();
	}

	// 恢复原来的输入输出
	cin.rdbuf(oldCin);
	cout.rdbuf(oldCout);

	// 检查输出是否符合预期
	assert(output.str() == "OK\n0 0\n0 2\n");

	cout << "Protocol tests passed!\n";
}
void testBoard()
{
	Board board;

	// 1. 测试初始棋盘
	assert(board.getCell(5, 5) == WHITE);
	assert(board.getCell(5, 6) == BLACK);
	assert(board.getCell(6, 5) == BLACK);
	assert(board.getCell(6, 6) == WHITE);
	assert(board.getCell(0, 0) == EMPTY);

	// 2. 测试合法性判断
	assert(board.isValid(0, 0));
	assert(!board.isValid(5, 5));
	assert(!board.isValid(-1, 0));
	assert(!board.isValid(12, 0));
	assert(!board.isValid(0, 12));

	// 3. 测试落子
	assert(board.place(0, 0, BLACK));
	assert(board.getCell(0, 0) == BLACK);

	// 已占用位置不能重复落子
	assert(!board.place(0, 0, WHITE));

	// 越界不能落子
	assert(!board.place(-1, 0, BLACK));

	// EMPTY 不能作为落子颜色
	assert(!board.place(0, 1, EMPTY));

	// 4. 测试撤销
	board.undo(0, 0);
	assert(board.getCell(0, 0) == EMPTY);

	// 5. 测试棋盘是否已满
	assert(!board.isFull());

	cout << "Board basic tests passed!\n";
}
void testIsFull()
{
	Board board;
	assert(!board.isFull());
	for (int i = 0; i < BOARD_SIZE; ++i)
	{
		for (int j = 0; j < BOARD_SIZE; ++j)
		{
			if (board.getCell(i,j) == EMPTY)
			{
				assert(board.place(i, j, BLACK));
			}
		}
	}
	assert(board.isFull());
	cout << "Board isFull test passed!\n";
}
void testDirection(int row, int col, int dx, int dy)
{
	Board board;

	for (int i = 0; i < 5; ++i)
	{
		assert(board.place(
			row + i * dx,
			col + i * dy,
			BLACK
		));
	}

	assert(board.checkWin(
		row + 4 * dx,
		col + 4 * dy
	));
}
void testWin()
{
	// 水平
	testDirection(0, 0, 0, 1);

	// 垂直
	testDirection(0, 0, 1, 0);

	// 主对角线
	testDirection(0, 0, 1, 1);

	// 副对角线
	testDirection(0, 4, 1, -1);

	cout << "Win detection tests passed!\n";
}

void testAI()
{
	GomokuAI ai;

	// 1. 测试正常落子
	{
		Board board;

		Move move = ai.chooseMove(board, BLACK);

		assert(board.inBounds(move.row, move.col));
		assert(board.isValid(move.row, move.col));

		// AI 不应该修改原棋盘
		assert(board.getCell(move.row, move.col) == EMPTY);
	}

	// 2. 测试非法颜色
	{
		Board board;

		Move invalid = ai.chooseMove(board, EMPTY);

		assert(invalid.row == -1);
		assert(invalid.col == -1);
	}

	// 3. 测试棋盘已满
	{
		Board board;

		for (int i = 0; i < BOARD_SIZE; ++i)
		{
			for (int j = 0; j < BOARD_SIZE; ++j)
			{
				if (board.getCell(i, j) == EMPTY)
					assert(board.place(i, j, WHITE));
			}
		}

		assert(board.isFull());

		Move noMove = ai.chooseMove(board, BLACK);

		assert(noMove.row == -1);
		assert(noMove.col == -1);
	}

	// 4. 测试自己能赢
	{
		Board board;

		for (int j = 0; j < 4; ++j)
			assert(board.place(0, j, BLACK));

		Move move = ai.chooseMove(board, BLACK);

		assert(move.row == 0);
		assert(move.col == 4);
	}

	// 5. 测试阻止对手获胜
	{
		Board board;

		for (int j = 0; j < 4; ++j)
			assert(board.place(0, j, WHITE));

		Move move = ai.chooseMove(board, BLACK);

		assert(move.row == 0);
		assert(move.col == 4);
	}

	// 6. 测试进攻优先于防守
	{
		Board board;

		// 黑棋有一个获胜位置
		for (int j = 0; j < 4; ++j)
			assert(board.place(0, j, BLACK));

		// 白棋也有一个获胜位置
		for (int j = 0; j < 4; ++j)
			assert(board.place(1, j, WHITE));

		Move move = ai.chooseMove(board, BLACK);

		assert(move.row == 0);
		assert(move.col == 4);
	}

	cout << "AI basic tests passed!\n";
}
void testClassifyPattern()
{
	GomokuAI ai;
	using P = GomokuAI::Pattern;
	assert(ai.classifyPattern("##_XXX_##")
		== P::OPEN_THREE);

	assert(ai.classifyPattern("###OXXX_#")
		== P::SLEEP_THREE);

	assert(ai.classifyPattern("##_XXXX_#")
		== P::OPEN_FOUR);

	assert(ai.classifyPattern("#XX_XX_##")
		== P::JUMP_FOUR);

	assert(ai.classifyPattern("##__X__##")
		== P::NONE);

	cout << "Pattern classification tests passed!\n";
}

int main()
{
	testBoard();

	testWin();

	testProtocol();

	testAI();

	testClassifyPattern();
	cout << "\nAll tests passed!\n";

	return 0;
}
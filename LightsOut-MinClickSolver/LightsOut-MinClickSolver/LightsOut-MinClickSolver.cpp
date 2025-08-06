#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <memory>
#include <windows.h>
#include <format>

using namespace std;

enum TileType { Off, On };

class Board
{
private:
    int size;
    vector<vector<TileType>> tiles;

    const string OFF = "■";
    const string ON = "□";
    const string C_OFF = "◆";
    const string C_ON = "◇";

    // 방향 벡터
    const int dx[4] = { 0, -1, 0, 1 };
    const int dy[4] = { -1, 0, 1, 0 };

    void init()
    {
        random_device rand;
        mt19937 gen(rand());
        uniform_int_distribution<int> dist(0, 1);

        // 랜덤하게 눌러보기
        for (int y = 0; y < size; y++)
        {
            for (int x = 0; x < size; x++)
            {
                if (dist(gen))
                    click(y, x);
            }
        }
    }

    TileType getOppositeType(int y, int x)
    {
        if (tiles[y][x] == TileType::Off)
            return TileType::On;
        else
            return TileType::Off;
    }

public:
    Board(int size) : size(size)
    {
        tiles.assign(size, vector<TileType>(size, TileType::Off));
        init();
    }

    string render(int cy = -1, int cx = -1)
    {
        string board;
        for (int y = 0; y < size; y++)
        {
            for (int x = 0; x < size; x++)
            {
                bool clicked = (y == cy && x == cx);
                if (clicked)
                    board += (tiles[y][x] == TileType::Off ? C_OFF : C_ON); // 클릭한 좌표 가시성
                else
                    board += (tiles[y][x] == TileType::Off ? OFF : ON);

                board += ' ';
            }

            board += '\n';
        }

        return board;
    }

    void click(int y, int x)
    {
        tiles[y][x] = getOppositeType(y, x);

        for (int dir = 0; dir < 4; dir++)
        {
            int nx = x + dx[dir];
            int ny = y + dy[dir];

            if (nx >= 0 && nx < size && ny >= 0 && ny < size)
                tiles[ny][nx] = getOppositeType(ny, nx);
        }
    }

    vector<vector<TileType>> getTiles()
    {
        return tiles;
    }

    void setTiles(vector<vector<TileType>> tiles)
    {
        this->tiles = tiles;
    }
};

unique_ptr<Board> board;
vector<string> logs;
string guide;
string nextNum;

void clearConsole()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD count;
    DWORD cellCount;
    COORD homeCoords = { 0, 0 };

    if (hConsole == INVALID_HANDLE_VALUE)
        return;

    if (GetConsoleScreenBufferInfo(hConsole, &csbi) == false)
        return;
    cellCount = csbi.dwSize.X * csbi.dwSize.Y;

    // Fill the entire buffer with spaces
    if (FillConsoleOutputCharacter(hConsole, ' ', cellCount, homeCoords, &count) == false)
        return;

    // Move the cursor home
    SetConsoleCursorPosition(hConsole, homeCoords);
}

void waitForMouseClick()
{
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD prevMode;
    GetConsoleMode(hInput, &prevMode);
    SetConsoleMode(hInput, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);

    INPUT_RECORD inputRecord;
    DWORD events;

    int range = logs.size() - 1;
    int idx = 0;

    while (true)
    {
        ReadConsoleInput(hInput, &inputRecord, 1, &events);

        if (inputRecord.EventType == MOUSE_EVENT)
        {
            MOUSE_EVENT_RECORD mer = inputRecord.Event.MouseEvent;

            if (idx > 0 && mer.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
            {
                clearConsole();
                cout << logs[--idx] << endl;
            }
            else if (idx < range && mer.dwButtonState & RIGHTMOST_BUTTON_PRESSED)
            {
                clearConsole();
                cout << logs[++idx] << endl;
            }
        }
    }

    SetConsoleMode(hInput, prevMode);
}

bool isValid(int& count)
{
    int size = board->getTiles().size();

    for (int y = 1; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            vector<vector<TileType>> tiles = board->getTiles();
            if (tiles[y - 1][x] == TileType::Off)
            {
                board->click(y, x);
                count++;

#pragma region Log
                string log = nextNum;
                log += board->render(y, x);
                log += "===========================================\n";
                log += format("click(row, col) : ({}, {})\n", y, x);
                log += format("click count : {}", count);
                logs.push_back(log);
#pragma endregion
            }
        }
    }

    // 마지막 줄 검사
    int y = size - 1;
    for (int x = 0; x < size; x++)
    {
        vector<vector<TileType>> tiles = board->getTiles();
        if (tiles[y][x] == TileType::Off)
            return false;
    }

    return true;
}

int bruteForce()
{
    vector<vector<TileType>> backupTiles = board->getTiles();
    int size = backupTiles.size();

    int minCount = 1e9;
    for (int mask = 0; mask < (1 << size); mask++)
    {
        board->setTiles(backupTiles);

#pragma region Log
        nextNum = format("{}next : {}\n", guide, mask + 1);
        string log = nextNum;
        log += board->render();
        log += "===========================================\n";
        log += "click(row, col) : Reset to the beginning.\n";
        logs.push_back(log);
#pragma endregion

        // 클릭 여부 설정
        int count = 0;
        for (int x = 0; x < size; x++)
        {
            if (mask & (1 << x))
            {
                board->click(0, x);
                count++;

#pragma region Log
                string log2 = nextNum;
                log2 += board->render(0, x);
                log2 += "===========================================\n";
                log2 += format("click(row, col) : (0, {})\n", x);
                log2 += format("click count : {}", count);
                logs.push_back(log2);
#pragma endregion
            }
        }

        if (isValid(count))
            minCount = min(minCount, count);
    }

    return (minCount == 1e9) ? -1 : minCount;
}

int main()
{
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    guide += "input board size : ";
    cout << guide;

    int size;
    cin >> size;    // 보드 크기

#pragma region Log
    guide += format("{}\n", size);
    guide += "===========================================\n";
    guide += "mouse left    =>  prev ◀\n";
    guide += "mouse right   =>  next ▶\n";
    guide += "===========================================\n";
#pragma endregion

    board = make_unique<Board>(size);
    int count = bruteForce();
    //cout << "minCount : " << count << endl;

    clearConsole();
    cout << logs[0];

    waitForMouseClick();
    return 0;
}
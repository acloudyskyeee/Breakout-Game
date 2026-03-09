#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

struct TerminalGuard {
#ifndef _WIN32
    termios oldt{};
    int oldf = 0;
    bool enabled = false;
#endif
    TerminalGuard() {
#ifndef _WIN32
        if (tcgetattr(STDIN_FILENO, &oldt) == 0) {
            termios newt = oldt;
            newt.c_lflag &= static_cast<unsigned int>(~(ICANON | ECHO));
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);
            oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
            fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
            enabled = true;
        }
#endif
    }
    ~TerminalGuard() {
#ifndef _WIN32
        if (enabled) {
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            fcntl(STDIN_FILENO, F_SETFL, oldf);
        }
#endif
    }
};

char readInput() {
#ifdef _WIN32
    if (_kbhit()) {
        return static_cast<char>(_getch());
    }
    return '\0';
#else
    char ch = '\0';
    if (read(STDIN_FILENO, &ch, 1) == 1) {
        return ch;
    }
    return '\0';
#endif
}

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    std::cout << "\033[2J\033[H";
#endif
}

struct Game {
    static constexpr int width = 44;
    static constexpr int height = 24;
    static constexpr int paddleWidth = 8;
    static constexpr int brickRows = 5;
    static constexpr int brickCols = 10;

    int paddleX = (width - paddleWidth) / 2;
    float ballX = width / 2.0f;
    float ballY = height - 4.0f;
    float dx = 0.7f;
    float dy = -0.7f;
    int score = 0;
    int lives = 3;
    bool running = true;
    bool win = false;

    std::vector<std::vector<bool>> bricks;

    Game() : bricks(brickRows, std::vector<bool>(brickCols, true)) {}

    int remainingBricks() const {
        int count = 0;
        for (const auto& row : bricks) {
            for (bool b : row) {
                if (b) ++count;
            }
        }
        return count;
    }

    void resetBall() {
        ballX = width / 2.0f;
        ballY = height - 4.0f;
        dx = (std::rand() % 2 == 0) ? 0.7f : -0.7f;
        dy = -0.7f;
    }

    void handleInput(char ch) {
        if (ch == 'a' || ch == 'A' || ch == 'h' || ch == 'H') {
            paddleX = std::max(1, paddleX - 2);
        } else if (ch == 'd' || ch == 'D' || ch == 'l' || ch == 'L') {
            paddleX = std::min(width - paddleWidth - 1, paddleX + 2);
        } else if (ch == 'q' || ch == 'Q') {
            running = false;
        }
    }

    void update() {
        float nextX = ballX + dx;
        float nextY = ballY + dy;

        if (nextX <= 1 || nextX >= width - 2) {
            dx = -dx;
            nextX = ballX + dx;
        }
        if (nextY <= 1) {
            dy = -dy;
            nextY = ballY + dy;
        }

        // Paddle collision
        if (dy > 0 && static_cast<int>(std::round(nextY)) == height - 2) {
            int bx = static_cast<int>(std::round(nextX));
            if (bx >= paddleX && bx < paddleX + paddleWidth) {
                float hit = (bx - paddleX) / static_cast<float>(paddleWidth - 1); // 0~1
                dx = (hit - 0.5f) * 1.8f;
                dy = -std::fabs(dy);
                nextY = ballY + dy;
            }
        }

        // Brick collision region
        int brickTop = 2;
        int brickHeight = 1;
        int brickWidth = (width - 2) / brickCols;
        int bx = static_cast<int>(std::round(nextX));
        int by = static_cast<int>(std::round(nextY));

        if (by >= brickTop && by < brickTop + brickRows * brickHeight) {
            int r = (by - brickTop) / brickHeight;
            int c = (bx - 1) / brickWidth;
            if (r >= 0 && r < brickRows && c >= 0 && c < brickCols && bricks[r][c]) {
                bricks[r][c] = false;
                score += 10;
                dy = -dy;
                nextY = ballY + dy;
            }
        }

        ballX = nextX;
        ballY = nextY;

        if (ballY > height - 1) {
            --lives;
            if (lives <= 0) {
                running = false;
            } else {
                resetBall();
            }
        }

        if (remainingBricks() == 0) {
            running = false;
            win = true;
        }
    }

    void draw() const {
        std::vector<std::string> buf(height, std::string(width, ' '));

        for (int y = 0; y < height; ++y) {
            buf[y][0] = '|';
            buf[y][width - 1] = '|';
        }
        for (int x = 0; x < width; ++x) {
            buf[0][x] = '-';
            buf[height - 1][x] = '-';
        }

        int brickWidth = (width - 2) / brickCols;
        for (int r = 0; r < brickRows; ++r) {
            for (int c = 0; c < brickCols; ++c) {
                if (!bricks[r][c]) continue;
                int y = 2 + r;
                int startX = 1 + c * brickWidth;
                for (int x = startX; x < startX + brickWidth - 1 && x < width - 1; ++x) {
                    buf[y][x] = '#';
                }
            }
        }

        for (int i = 0; i < paddleWidth; ++i) {
            int x = paddleX + i;
            if (x > 0 && x < width - 1) {
                buf[height - 2][x] = '=';
            }
        }

        int bx = static_cast<int>(std::round(ballX));
        int by = static_cast<int>(std::round(ballY));
        if (by > 0 && by < height - 1 && bx > 0 && bx < width - 1) {
            buf[by][bx] = 'o';
        }

        clearScreen();
        std::cout << "Breakout (A/D 移动, Q 退出)\n";
        std::cout << "Score: " << score << "  Lives: " << lives << "  Bricks: " << remainingBricks() << "\n";
        for (const auto& line : buf) {
            std::cout << line << '\n';
        }
        std::cout.flush();
    }
};

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    TerminalGuard guard;

#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hOut, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &cursorInfo);
#else
    std::cout << "\033[?25l";
#endif

    Game game;

    using clock = std::chrono::steady_clock;
    auto last = clock::now();

    while (game.running) {
        char ch = readInput();
        if (ch != '\0') game.handleInput(ch);

        auto now = clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
        if (elapsed >= 30) {
            game.update();
            game.draw();
            last = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

#ifndef _WIN32
    std::cout << "\033[?25h";
#endif

    std::cout << (game.win ? "\n你赢了！\n" : "\n游戏结束！\n");
    std::cout << "最终得分: " << game.score << "\n";
    return 0;
}

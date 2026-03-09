#include <SDL2/SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <string>
#include <vector>

struct Brick {
    SDL_Rect rect{};
    bool alive = true;
    SDL_Color color{255, 255, 255, 255};
};

struct Game {
    static constexpr int kWindowWidth = 960;
    static constexpr int kWindowHeight = 640;

    static constexpr int kBrickRows = 6;
    static constexpr int kBrickCols = 12;
    static constexpr int kBrickW = 68;
    static constexpr int kBrickH = 24;
    static constexpr int kBrickGap = 8;
    static constexpr int kTopPadding = 70;

    static constexpr int kPaddleW = 130;
    static constexpr int kPaddleH = 18;
    static constexpr int kPaddleY = kWindowHeight - 50;
    static constexpr float kPaddleSpeed = 560.0f;

    static constexpr int kBallSize = 14;
    static constexpr float kBallSpeed = 360.0f;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    std::vector<Brick> bricks;
    SDL_FRect paddle{(kWindowWidth - kPaddleW) / 2.0f, static_cast<float>(kPaddleY), static_cast<float>(kPaddleW), static_cast<float>(kPaddleH)};
    SDL_FRect ball{(kWindowWidth - kBallSize) / 2.0f, static_cast<float>(kWindowHeight - 100), static_cast<float>(kBallSize), static_cast<float>(kBallSize)};

    float vx = kBallSpeed * 0.707f;
    float vy = -kBallSpeed * 0.707f;

    int lives = 3;
    int score = 0;
    bool running = true;
    bool win = false;

    bool init() {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            SDL_Log("SDL_Init failed: %s", SDL_GetError());
            return false;
        }

        window = SDL_CreateWindow(
            "Breakout - Score: 0 Lives: 3",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            kWindowWidth,
            kWindowHeight,
            SDL_WINDOW_SHOWN
        );
        if (!window) {
            SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
            return false;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
            SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
            return false;
        }

        createBricks();
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        resetBall(false);
        return true;
    }

    void shutdown() const {
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void createBricks() {
        bricks.clear();
        bricks.reserve(kBrickRows * kBrickCols);

        constexpr std::array<SDL_Color, 6> palette{{
            {255, 99, 132, 255},
            {255, 159, 64, 255},
            {255, 205, 86, 255},
            {75, 192, 192, 255},
            {54, 162, 235, 255},
            {153, 102, 255, 255},
        }};

        const int totalWidth = kBrickCols * kBrickW + (kBrickCols - 1) * kBrickGap;
        const int startX = (kWindowWidth - totalWidth) / 2;

        for (int r = 0; r < kBrickRows; ++r) {
            for (int c = 0; c < kBrickCols; ++c) {
                Brick b;
                b.rect = {
                    startX + c * (kBrickW + kBrickGap),
                    kTopPadding + r * (kBrickH + kBrickGap),
                    kBrickW,
                    kBrickH,
                };
                b.alive = true;
                b.color = palette[r % palette.size()];
                bricks.push_back(b);
            }
        }
    }

    void resetBall(bool randomDirection) {
        ball.x = (kWindowWidth - kBallSize) / 2.0f;
        ball.y = static_cast<float>(kWindowHeight - 100);

        float dirX = randomDirection ? ((std::rand() % 2 == 0) ? -0.68f : 0.68f) : 0.68f;
        vx = kBallSpeed * dirX;
        vy = -std::sqrt(kBallSpeed * kBallSpeed - vx * vx);
    }

    void updateWindowTitle() const {
        std::ostringstream oss;
        if (running) {
            oss << "Breakout - Score: " << score << " Lives: " << lives;
        } else if (win) {
            oss << "Breakout - You Win! Score: " << score << " (Press R to restart / Esc to quit)";
        } else {
            oss << "Breakout - Game Over Score: " << score << " (Press R to restart / Esc to quit)";
        }
        SDL_SetWindowTitle(window, oss.str().c_str());
    }

    int bricksLeft() const {
        int cnt = 0;
        for (const auto& b : bricks) {
            if (b.alive) ++cnt;
        }
        return cnt;
    }

    void restart() {
        lives = 3;
        score = 0;
        win = false;
        running = true;
        paddle.x = (kWindowWidth - kPaddleW) / 2.0f;
        createBricks();
        resetBall(false);
        updateWindowTitle();
    }

    void handleEvents(bool& quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    quit = true;
                } else if ((e.key.keysym.sym == SDLK_r) && !running) {
                    restart();
                }
            }
        }
    }

    void update(float dt) {
        if (!running) return;

        const Uint8* state = SDL_GetKeyboardState(nullptr);
        if (state[SDL_SCANCODE_LEFT] || state[SDL_SCANCODE_A]) {
            paddle.x -= kPaddleSpeed * dt;
        }
        if (state[SDL_SCANCODE_RIGHT] || state[SDL_SCANCODE_D]) {
            paddle.x += kPaddleSpeed * dt;
        }
        paddle.x = std::clamp(paddle.x, 0.0f, static_cast<float>(kWindowWidth - kPaddleW));

        ball.x += vx * dt;
        ball.y += vy * dt;

        if (ball.x <= 0.0f) {
            ball.x = 0.0f;
            vx = std::fabs(vx);
        }
        if (ball.x + kBallSize >= kWindowWidth) {
            ball.x = static_cast<float>(kWindowWidth - kBallSize);
            vx = -std::fabs(vx);
        }
        if (ball.y <= 0.0f) {
            ball.y = 0.0f;
            vy = std::fabs(vy);
        }

        SDL_Rect ballRect{
            static_cast<int>(ball.x),
            static_cast<int>(ball.y),
            static_cast<int>(ball.w),
            static_cast<int>(ball.h)
        };

        SDL_Rect paddleRect{
            static_cast<int>(paddle.x),
            static_cast<int>(paddle.y),
            static_cast<int>(paddle.w),
            static_cast<int>(paddle.h)
        };

        if (SDL_HasIntersection(&ballRect, &paddleRect) && vy > 0.0f) {
            ball.y = paddle.y - kBallSize;
            float hitPos = ((ball.x + kBallSize * 0.5f) - paddle.x) / paddle.w;  // 0~1
            float angle = (-75.0f + hitPos * 150.0f) * static_cast<float>(M_PI / 180.0f);
            vx = kBallSpeed * std::sin(angle);
            vy = -kBallSpeed * std::cos(angle);
        }

        for (auto& brick : bricks) {
            if (!brick.alive) continue;
            if (SDL_HasIntersection(&ballRect, &brick.rect)) {
                brick.alive = false;
                score += 10;

                int ballCenterX = ballRect.x + ballRect.w / 2;
                int ballCenterY = ballRect.y + ballRect.h / 2;
                int brickCenterX = brick.rect.x + brick.rect.w / 2;
                int brickCenterY = brick.rect.y + brick.rect.h / 2;

                int dxCenter = std::abs(ballCenterX - brickCenterX);
                int dyCenter = std::abs(ballCenterY - brickCenterY);
                if (dxCenter > dyCenter) {
                    vx = -vx;
                } else {
                    vy = -vy;
                }

                updateWindowTitle();
                break;
            }
        }

        if (ball.y > kWindowHeight) {
            --lives;
            if (lives <= 0) {
                running = false;
                win = false;
            } else {
                resetBall(true);
            }
            updateWindowTitle();
        }

        if (bricksLeft() == 0) {
            running = false;
            win = true;
            updateWindowTitle();
        }
    }

    void render() const {
        SDL_SetRenderDrawColor(renderer, 18, 18, 26, 255);
        SDL_RenderClear(renderer);

        // Bricks
        for (const auto& brick : bricks) {
            if (!brick.alive) continue;
            SDL_SetRenderDrawColor(renderer, brick.color.r, brick.color.g, brick.color.b, 255);
            SDL_RenderFillRect(renderer, &brick.rect);
        }

        // Paddle
        SDL_Rect paddleRect{
            static_cast<int>(paddle.x),
            static_cast<int>(paddle.y),
            static_cast<int>(paddle.w),
            static_cast<int>(paddle.h)
        };
        SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
        SDL_RenderFillRect(renderer, &paddleRect);

        // Ball
        SDL_Rect ballRect{
            static_cast<int>(ball.x),
            static_cast<int>(ball.y),
            static_cast<int>(ball.w),
            static_cast<int>(ball.h)
        };
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &ballRect);

        SDL_RenderPresent(renderer);
    }
};

int main() {
    Game game;
    if (!game.init()) {
        return 1;
    }

    game.updateWindowTitle();

    bool quit = false;
    Uint64 prev = SDL_GetPerformanceCounter();

    while (!quit) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(now - prev) / static_cast<float>(SDL_GetPerformanceFrequency());
        prev = now;

        dt = std::min(dt, 0.033f);

        game.handleEvents(quit);
        game.update(dt);
        game.render();
    }

    game.shutdown();
    return 0;
}

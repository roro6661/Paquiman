#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <conio.h>
#include <fstream>
#include <algorithm>
#include <thread>

using namespace std;

constexpr int W = 20;
constexpr int H = 20;

template <typename T>
inline bool inBounds(T x, T y, int w = W, int h = H) {
    return x >= 0 && y >= 0 && x < w && y < h;
}

template <typename T>
inline int manhattan(T x1, T y1, T x2, T y2) {
    return static_cast<int>(abs(x1 - x2) + abs(y1 - y2));
}

struct GameConfig {
    int ghosts = 3;
    int pills = 10;
    int time_limit_s = 300;
};

struct Entity {
    int x = 1, y = 1;
    char glyph = '@';
    bool alive = true;
};

struct Hero : Entity {
    int score = 0;
    int lives = 3;
    Hero() { glyph = 'H'; }
};

struct GameState {
    vector<string> grid;
    Hero hero;
    vector<Entity> ghosts;
    int pills_remaining = 0;
    bool game_over = false;
    bool win = false;
};

struct Rng {
    mt19937 gen;
    Rng() : gen((uint32_t)chrono::high_resolution_clock::now().time_since_epoch().count()) {}
    int nextInt(int lo, int hi) {
        uniform_int_distribution<int> dist(lo, hi);
        return dist(gen);
    }
} RNG;

static vector<string> makeEmptyGrid() {
    vector<string> g(H, string(W, '.'));
    for (int x = 0; x < W; ++x) g[0][x] = g[H - 1][x] = '#';
    for (int y = 0; y < H; ++y) g[y][0] = g[y][W - 1] = '#';
    return g;
}

static void draw(const GameState& S) {
    system("cls");
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            if (S.hero.alive && S.hero.x == x && S.hero.y == y) {
                cout << S.hero.glyph;
            }
            else {
                bool gHere = false;
                for (const auto& g : S.ghosts) {
                    if (g.alive && g.x == x && g.y == y) { cout << g.glyph; gHere = true; break; }
                }
                if (!gHere) cout << S.grid[y][x];
            }
        }
        cout << '\n';
    }
    cout << "Vidas: " << S.hero.lives
        << " | Pontos: " << S.hero.score
        << " | Pilulas: " << S.pills_remaining
        << " | ('w','a','s','d' p/ mover, 'b' bomba, 'q' sair)\n";
}

static void placeRandom(vector<string>& grid, Entity& e, char cellFree = '.') {
    int x, y;
    do {
        x = RNG.nextInt(1, W - 2);
        y = RNG.nextInt(1, H - 2);
    } while (grid[y][x] != cellFree);
    e.x = x; e.y = y;
}

static void placeRandom(vector<string>& grid, char glyph, int count, char cellFree = '.') {
    int placed = 0;
    while (placed < count) {
        int x = RNG.nextInt(1, W - 2);
        int y = RNG.nextInt(1, H - 2);
        if (grid[y][x] == cellFree) {
            grid[y][x] = glyph;
            placed++;
        }
    }
}

static void initGame(GameState& S, const GameConfig& C) {
    S.grid = makeEmptyGrid();
    S.ghosts.clear();
    S.hero = Hero{};
    S.pills_remaining = C.pills;
    S.game_over = false; S.win = false;

    placeRandom(S.grid, 'o', C.pills);

    placeRandom(S.grid, static_cast<Entity&>(S.hero));

    S.ghosts.resize(C.ghosts);
    for (auto& g : S.ghosts) {
        g.glyph = 'G';
        g.alive = true;
        placeRandom(S.grid, g);
    }
}

static void tryMoveHero(GameState& S, int dx, int dy) {
    int nx = S.hero.x + dx;
    int ny = S.hero.y + dy;
    if (!inBounds(nx, ny) || S.grid[ny][nx] == '#') return;

    S.hero.x = nx; S.hero.y = ny;

    if (S.grid[ny][nx] == 'o') {
        S.grid[ny][nx] = '.';
        S.hero.score += 10;
        S.pills_remaining = max(0, S.pills_remaining - 1);
    }
}

static int explodeBomb(GameState& S, int cx, int cy, int radius = 1) {
    int killed = 0;
    for (int y = cy - radius; y <= cy + radius; ++y) {
        for (int x = cx - radius; x <= cx + radius; ++x) {
            if (!inBounds(x, y)) continue;
            if (S.grid[y][x] == 'o') S.grid[y][x] = '.';
            for (auto& g : S.ghosts) {
                if (g.alive && g.x == x && g.y == y) {
                    g.alive = false;
                    killed++;
                }
            }
        }
    }
    if (killed >= 1) S.hero.score += 200 * killed;
    return killed;
}

static void moveGhostsRandom(GameState& S) {
    static const int dirs[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
    for (auto& g : S.ghosts) {
        if (!g.alive) continue;
        int order[4] = { 0,1,2,3 };
        shuffle(begin(order), end(order), RNG.gen);
        for (int k = 0; k < 4; ++k) {
            int d = order[k];
            int nx = g.x + dirs[d][0];
            int ny = g.y + dirs[d][1];
            if (inBounds(nx, ny) && S.grid[ny][nx] != '#') {
                g.x = nx; g.y = ny;
                break;
            }
        }
    }
}

static bool checkCaptured(const GameState& S) {
    for (const auto& g : S.ghosts) {
        if (g.alive && g.x == S.hero.x && g.y == S.hero.y) return true;
    }
    return false;
}

static bool allGhostsDead(const GameState& S) {
    for (const auto& g : S.ghosts) if (g.alive) return false;
    return true;
}

static void showRanking() {
    ifstream in("ranking.txt");
    cout << "\n===== Ranking =====\n";
    if (!in) { cout << "(sem registros)\n"; return; }
    string line;
    vector<pair<int, string>> entries;
    while (getline(in, line)) {
        auto pos = line.find(';');
        if (pos == string::npos) continue;
        int sc = stoi(line.substr(0, pos));
        string nome = line.substr(pos + 1);
        entries.push_back({ sc, nome });
    }
    sort(entries.begin(), entries.end(), [](auto& a, auto& b) { return a.first > b.first; });
    int rank = 1;
    for (auto& e : entries) {
        cout << rank++ << ") " << e.second << " - " << e.first << " pts\n";
        if (rank > 10) break;
    }
    cout << "===================\n";
}

static void saveRanking(const string& nome, int score) {
    ofstream out("ranking.txt", ios::app);
    if (out) out << score << ';' << nome << "\n";
}

static GameConfig selectDifficulty() {
    while (true) {
        system("cls");
        cout << "===== PAC-MAN SIMPLIFICADO (M1) =====\n";
        cout << "[1] Facil    (3G, 10P, 300s)\n";
        cout << "[2] Medio    (5G, 8P, 240s)\n";
        cout << "[3] Dificil  (6G, 5P, 120s)\n";
        cout << "[4] Ranking\n";
        cout << "[0] Sair\n";
        cout << "Escolha: ";
        char c = _getch();
        cout << c << "\n";
        if (c == '1') return GameConfig{ 3,10,300 };
        if (c == '2') return GameConfig{ 5, 8,240 };
        if (c == '3') return GameConfig{ 6, 5,120 };
        if (c == '4') { showRanking(); system("pause"); }
        if (c == '0') exit(0);
    }
}

static void gameLoop(const string& playerName, const GameConfig& C) {
    GameState S;
    initGame(S, C);

    auto t0 = chrono::steady_clock::now();
    const int TL = C.time_limit_s;

    while (!S.game_over) {
        draw(S);

        auto now = chrono::steady_clock::now();
        int elapsed = (int)chrono::duration_cast<chrono::seconds>(now - t0).count();
        cout << "Tempo: " << elapsed << "s / Limite: " << TL << "s\n";

        char key = _kbhit() ? _getch() : '\0';
        if (key) {
            if (key == 'q') { S.game_over = true; break; }
            if (key == 'w') tryMoveHero(S, 0, -1);
            if (key == 's') tryMoveHero(S, 0, 1);
            if (key == 'a') tryMoveHero(S, -1, 0);
            if (key == 'd') tryMoveHero(S, 1, 0);
            if (key == 'b') {
                int k = explodeBomb(S, S.hero.x, S.hero.y);
                if (k > 0) {
                }
            }
        }

        moveGhostsRandom(S);

        if (checkCaptured(S)) {
            S.hero.lives--;
            if (S.hero.lives <= 0) { S.game_over = true; S.win = false; }
            else {
                placeRandom(S.grid, static_cast<Entity&>(S.hero));
                for (auto& g : S.ghosts) if (g.alive) placeRandom(S.grid, g);
            }
        }

        if (allGhostsDead(S)) { S.game_over = true; S.win = true; }

        if (elapsed >= TL) {  }

        this_thread::sleep_for(chrono::milliseconds(50));
    }

    int elapsed = (int)chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - t0).count();
    int bonus = max(0, (TL - elapsed)) * 50;
    if (S.win) S.hero.score += bonus;

    system("cls");
    cout << (S.win ? "VITORIA!\n" : "DERROTA.\n");
    cout << "Score final: " << S.hero.score << "\n";
    if (S.win) cout << "Bonus de rapidez: " << bonus << " pts\n";

    saveRanking(playerName, S.hero.score);
    cout << "Registro salvo no ranking. aperte qualquer tecla para continuar...\n";
    _getch();
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cout << "Digite seu nome: ";
    string playerName;
    getline(cin, playerName);
    if (playerName.empty()) playerName = "Player";

    while (true) {
        GameConfig cfg = selectDifficulty();
        gameLoop(playerName, cfg);
    }
    return 0;
}

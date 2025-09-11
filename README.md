Cabeçalhos (#include) — para que servem e onde são usados

<iostream> — I/O no console: std::cout, std::cin.
Usado em praticamente todo o programa: displayStatus, draw, menu, mensagens finais etc.

<vector> — arranjos dinâmicos: std::vector.
Usado em GameState (board, ghosts) e em várias funções utilitárias.

<string> — strings: std::string.
Usado em todo lugar (nome do jogador, leitura/gravação de ranking, direções, dificuldades).

<algorithm> — algoritmos genéricos como std::sort, std::max.
Usado em showRank (ordenar ranking) e no cálculo de bonus com std::max.

<chrono> — tempo/relógio: medir duração, timepoints.
Usado em GameState::start, em cálculos de tempo no draw e no final da loop, e para “seed” do RNG.

<random> — números aleatórios: std::mt19937, std::uniform_int_distribution.
Usado em rng, rnd, randomFree, moveGhosts.

<fstream> — arquivo texto: std::ifstream, std::ofstream.
Usado em saveRank (gravar) e showRank (ler).

<ctime> — tempo estilo C: std::time, std::localtime, std::strftime.
Usado em saveRank para datar o ranking.

<cctype> — caracteres: std::tolower.
Usado na leitura de tecla (key) na loop.

<conio.h> — entrada sem bloqueio/eco (Windows): _getch().
Usado na loop para ler w/a/s/d/b/q sem precisar de Enter.

<windows.h> — APIs do Windows: SetConsoleOutputCP, SetConsoleCP.
Usado no main para configurar o console em UTF-8 (exibir emojis e acentos).

Observações de compatibilidade:

Você usa system("cls") em draw. A declaração canônica vem de <cstdlib>. Muitos compiladores aceitam sem incluir, mas o correto é adicionar #include <cstdlib> para declarar std::system.

Você chama abs(...) com inteiros em startGame. A versão padrão está em <cstdlib> (ou <cmath>). Para evitar surpresas, inclua <cstdlib> e use std::abs.

Estruturas e estado do jogo
struct Paquiman

Representa o herói (jogador).

Campos (privados): name, posição r/c, score, lives.

Construtor: inicializa nome, score=0, lives=3.

Métodos principais

move(direction): altera r/c conforme "up", "down", "left", "right".
Bibliotecas envolvidas: só <string>.

eat(item): soma 10 (dot) ou 50 (power-pellet).

loseLife(): decrementa lives.

displayStatus() const: imprime status no console.
Bibliotecas: <iostream>.

Acesso/auxiliares (públicos): row(), col(), setPos(), scoreRef() (referência para manipular pontos), scoreVal(), livesVal().

struct Difficulty

Um “preset” de dificuldade: nome, nº de fantasmas, nº de pílulas, limite de tempo.

struct GameState

Estado global da partida.

static constexpr int N=20: tamanho do tabuleiro (20×20).

board: vector<string> com o mapa (parede #, pílula P, espaço vazio).

hero: o Paquiman.

ghosts: vetor de pares (r,c) com as posições dos fantasmas.

diff: dificuldade escolhida.

pillsInBag: quantas pílulas o herói “carrega” para detonar bomba.

start: marca de tempo inicial (para HUD e bônus).

running, victory: flags do loop.

Utilitários de aleatoriedade e validação

static mt19937 rng(...) + int rnd(int a,int b)
Gerador pseudo-aleatório Mersenne Twister e uma função para sortear inteiros uniformes.
Bibliotecas: <random>, <chrono> (seed).

bool in(int r,int c)
Testa se (r,c) está dentro da grade N×N.

bool canStep(const vector<string>& b, int r, int c)
Verifica se a célula não é parede # — usada antes de mover herói/fantasma.

Varredura em cruz e posições
template <class F> int applyInCross(...)

Percorre uma cruz (célula central + raios para 4 direções até rad), chamando um functor fn(r,c) em cada célula válida.
Usada para calcular o alcance da explosão.
Bibliotecas: genéricos do C++ (templates); não requer nada específico além de <vector>/<string>.

struct Position e moveEntity(...)

Position { int r,c; }: par de coordenadas.

moveEntity(Position&, char dir) e moveEntity(Position&, int dr,int dc): funções genéricas para mover uma entidade.
No código atual, você usa movimento do herói via Paquiman::move, mas deixa estes utilitários prontos para futura expansão (p.ex., mover fantasmas “comportamentais”).

Construção do mapa e recursos

void makeBoard(vector<string>& b)
Cria uma grade N×N preenchida com ' ' e bordas com '#'.
Bibliotecas: <vector>, <string>.

pair<int,int> randomFree(const vector<string>& b)
Sorteia uma célula interna que esteja vazia ' '.
Bibliotecas: <random>.

void dropPills(vector<string>& b,int q)
Posiciona q pílulas 'P' em posições livres aleatórias.

bool hasGhostAt(const vector<pair<int,int>>&,int r,int c)
Testa se existe fantasma naquela célula.
Bibliotecas: <vector>, <utility> (implícito via <vector>), mas está ok.

Pontuação e explosão

int scoreExplosion(int k) e int scoreExplosion(int k,int base)
Convertem “fantasmas atingidos” em pontos. Versão padrão: 200*k.

void moveGhosts(GameState& st)
Para cada fantasma, tenta até 4 direções aleatórias e move se a célula é válida.
Bibliotecas: <random> (via rnd), utilitários de movimento/validação.

bool caught(const GameState& st)
Verdadeiro se algum fantasma está na mesma célula que o herói.

int explode(GameState& st, int radius=2)
Se o herói tem pillsInBag > 0, aplica applyInCross em volta do herói, remove os fantasmas atingidos (filtra st.ghosts) e consome 1 pílula.
Retorna quantos fantasmas foram destruídos (para calcular a pontuação com scoreExplosion).
Bibliotecas: <vector>, <algorithm> (para reservar/trocar vetores).

Desenho/HUD e interação
void draw(const GameState& st, bool hud=true)

Limpa a tela com system("cls") (Windows).

Percorre o tabuleiro e imprime:

herói como 🙃

fantasmas como 👻

paredes #

pílulas 💊

HUD: mostra jogador (texto fixo “Paquiman” no print), pílulas na bolsa, pontos, tempo decorrido vs. limite, e dicas de controle.
Bibliotecas: <iostream>, <chrono>, <windows.h> (UTF-8 setado no main); ideal incluir <cstdlib> para system.

Nota: Emojis exigem fonte/console compatível; você já configurou UTF-8 em main.

Ranking (persistência simples em arquivo texto)
void saveRank(const string& name,int score,const string& file="ranking.txt")

Pega data e hora atuais (<ctime>), formata como YYYY-MM-DD HH:MM.

Abre ranking.txt em modo append e grava name;score;data.
Bibliotecas: <fstream>, <ctime>, <string>.

void showRank(const string& file="ranking.txt", int topN=10)

Lê o arquivo linha a linha, faz parse por ';'.

Constrói vetor de registros {nome, score, data}.

Ordena por score desc com std::sort.

Exibe top N.
Bibliotecas: <fstream>, <string>, <vector>, <algorithm>.

Menu e seleção de dificuldade
Difficulty chooseDiff()

Mostra opções e retorna um Difficulty com (nome, ghosts, pills, timeLimitSec).

GameState startGame(const string& name,const Difficulty& d)

Cria GameState.

Gera o tabuleiro com bordas (makeBoard).

Coloca o herói no centro (ou em (1,1) se houver parede).

Solta pílulas (dropPills) e espalha fantasmas aleatórios, evitando spawn muito perto do herói.

Marca start = now.
Bibliotecas: <chrono>, <random>, <vector>.

Loop principal do jogo
void loop(GameState& st)

Fluxo por “tick”:

Coleta de pílula
Se a célula do herói tem 'P':

incrementa pillsInBag

limpa a célula

soma pontos de “dot” via hero.eat("dot") (+10)

Desenho
Chama draw(st).

Entrada do usuário
Lê uma tecla com _getch() e converte para minúscula (std::tolower) — sem Enter.

q: encerra a partida.

b: bomba — chama explode(st, 2); converte retorno em pontos com scoreExplosion(k) somando via scoreRef().

w/a/s/d: traduz para direção "up/down/left/right", checa colisão com parede com canStep, e então chama hero.move(dir).

Atualização dos fantasmas
moveGhosts(st) (passeio aleatório).

Condições de término

se caught(st): derrota.

se st.ghosts.empty(): vitória.

Fim da partida
Calcula secs = now - start;
bonus = max(0, (timeLimitSec - secs) * 50);
adiciona bonus à pontuação; imprime resumo;
grava ranking com saveRank("Paquiman", score).

Bibliotecas: <conio.h>, <cctype>, <chrono>, <algorithm>, <iostream>.

Observação: o limite de tempo hoje não encerra a partida — ele só influencia o bônus final. Se você quiser que o jogo termine por tempo, bastaria checar durante o loop e setar running=false quando secs >= timeLimitSec.

Menu principal e main()
void menu()

Loop com três opções:

Jogar: lê nome, escolhe dificuldade, cria estado startGame, roda loop.

Ranking: chama showRank.

Sair: encerra.

int main()

SetConsoleOutputCP(CP_UTF8); SetConsoleCP(CP_UTF8); — configura entrada/saída do console para UTF-8 (Windows).
Biblioteca: <windows.h>.

ios::sync_with_stdio(false); cin.tie(nullptr); — acelera I/O do C++.

Chama menu() e retorna.

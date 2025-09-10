#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <random>
#include <fstream>
#include <ctime>
#include <cctype>
#include <conio.h>
#include <windows.h>

using namespace std;

struct Paquiman {
public:
    explicit Paquiman(const string& name) : name(name), score(0), lives(3) {}
    void move(const string& direction) {
        if (direction == "up") r--;
        else if (direction == "down") r++;
        else if (direction == "left") c--;
        else if (direction == "right") c++;
    }
    void eat(const string& item) {
        if (item == "dot") score += 10;
        else if (item == "power-pellet") score += 50;
    }
    void loseLife() {
        lives--;
        if (lives <= 0) {}
    }
    void displayStatus() const {
        cout << "Status of " << name << ": Position(" << c << ", " << r
             << "), Score: " << score << ", Lives: " << lives << "\n";
    }
private:
    string name;
    int r = 0, c = 0;
    int score;
    int lives;
public:
    int row() const { return r; }
    int col() const { return c; }
    void setPos(int R, int C){ r = R; c = C; }
    int& scoreRef(){ return score; }
    int  scoreVal() const { return score; }
    int  livesVal() const { return lives; }
};

struct Difficulty { string name; int ghosts; int pills; int timeLimitSec; };

struct GameState {
    static constexpr int N = 20;
    vector<string> board;
    Paquiman hero;
    vector<pair<int,int>> ghosts;
    Difficulty diff;
    int pillsInBag{0};
    chrono::steady_clock::time_point start;
    bool running{true}, victory{false};
    GameState(const string& playerName, const Difficulty& d)
        : board(), hero(playerName), ghosts(), diff(d) {}
};

static mt19937 rng( (uint32_t)chrono::steady_clock::now().time_since_epoch().count() );
int rnd(int a,int b){ return uniform_int_distribution<int>(a,b)(rng); }
bool in(int r,int c){ return 0<=r && r<GameState::N && 0<=c && c<GameState::N; }
bool canStep(const vector<string>& b, int r, int c){ return in(r,c) && b[r][c] != '#'; }

template <class F>
int applyInCross(vector<string>& g, int r0, int c0, int rad, F&& fn){
    if (!in(r0,c0) || g[r0][c0]=='#') return 0;
    int ct=0;
    auto poke=[&](int r,int c){ if(in(r,c)&&g[r][c]!='#'){ fn(r,c); ++ct; } };
    poke(r0,c0);
    const int dr[4]={-1,1,0,0}, dc[4]={0,0,-1,1};
    for(int k=0;k<4;k++){
        int r=r0,c=c0;
        for(int s=1;s<=rad;s++){
            r+=dr[k]; c+=dc[k];
            if(!in(r,c) || g[r][c]=='#') break;
            fn(r,c); ++ct;
        }
    }
    return ct;
}

struct Position { int r{}, c{}; };
bool moveEntity(Position& p, char dir){
    int dr=0,dc=0;
    if(dir=='w') dr=-1; else if(dir=='s') dr=1; else if(dir=='a') dc=-1; else if(dir=='d') dc=1; else return false;
    p.r+=dr; p.c+=dc; return true;
}
bool moveEntity(Position& p, int dr, int dc){ p.r+=dr; p.c+=dc; return true; }

void makeBoard(vector<string>& b){
    b.assign(GameState::N, string(GameState::N,' '));
    for(int i=0;i<GameState::N;i++){
        b[0][i]='#'; b[GameState::N-1][i]='#';
        b[i][0]='#'; b[i][GameState::N-1]='#';
    }
}
pair<int,int> randomFree(const vector<string>& b){
    while(true){ int r=rnd(1,GameState::N-2), c=rnd(1,GameState::N-2); if(b[r][c]==' ') return {r,c}; }
}
void dropPills(vector<string>& b,int q){ for(int i=0;i<q;i++){ auto p=randomFree(b); b[p.first][p.second]='P'; } }
bool hasGhostAt(const vector<pair<int,int>>& gs,int r,int c){ for(auto& g:gs) if(g.first==r&&g.second==c) return true; return false; }

int scoreExplosion(int k){ return k>0? 200*k : 0; }
int scoreExplosion(int k,int base){ return k>0? base*k : 0; }

void moveGhosts(GameState& st){
    static const int DR[4]={-1,1,0,0}, DC[4]={0,0,-1,1};
    for(auto& g: st.ghosts){
        for(int t=0;t<4;t++){
            int k=rnd(0,3), nr=g.first+DR[k], nc=g.second+DC[k];
            if(!canStep(st.board,nr,nc)) continue;
            g.first=nr; g.second=nc; break;
        }
    }
}
bool caught(const GameState& st){ return hasGhostAt(st.ghosts, st.hero.row(), st.hero.col()); }

int explode(GameState& st, int radius=2){
    if(st.pillsInBag<=0) return 0;
    vector<Position> hit;
    applyInCross(st.board, st.hero.row(), st.hero.col(), radius, [&](int r,int c){ hit.push_back({r,c}); });
    int before = (int)st.ghosts.size();
    vector<pair<int,int>> keep; keep.reserve(before);
    for(auto g: st.ghosts){
        bool dead=false; for(auto& h:hit) if(g.first==h.r && g.second==h.c){ dead=true; break; }
        if(!dead) keep.push_back(g);
    }
    st.ghosts.swap(keep);
    st.pillsInBag--;
    return before - (int)st.ghosts.size();
}

void draw(const GameState& st, bool hud=true){
    system("cls");
    for(int r=0;r<GameState::N;r++){
        for(int c=0;c<GameState::N;c++){
            if(st.hero.row()==r && st.hero.col()==c) { cout<<"🙃"; continue; }
            if(hasGhostAt(st.ghosts,r,c)) { cout<<"👻"; continue; }
            char ch=st.board[r][c];
            if(ch=='#') cout<<"#";
            else if(ch=='P') cout<<"💊";
            else cout<<" ";
        }
        cout<<"\n";
    }
    if(hud){
        auto sec = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now()-st.start).count();
        cout<<"\nJogador: "<</*oculto*/"Paquiman"
            <<" | Pílulas: "<<st.pillsInBag
            <<" | Pontos: "<<st.hero.scoreVal()
            <<" | Tempo: "<<sec<<"s / T_limite="<<st.diff.timeLimitSec<<"s\n";
        cout<<"w/a/s/d mover | b bomba | q sair\n";
        cout<<"Sua jogada: ";
    }
}

void saveRank(const string& name,int score,const string& file="ranking.txt"){
    time_t t=time(nullptr); tm* lt=localtime(&t); char buf[32]; strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M",lt);
    ofstream out(file, ios::app); if(out) out<<name<<';'<<score<<';'<<buf<<"\n";
}
void showRank(const string& file="ranking.txt", int topN=10){
    struct R{string n; int s; string d;};
    ifstream in(file); vector<R> v; string line;
    while(getline(in,line)){
        auto p1=line.find(';'); auto p2=line.find(';',p1==string::npos?0:p1+1);
        if(p1==string::npos || p2==string::npos) continue;
        R r{ line.substr(0,p1), stoi(line.substr(p1+1,p2-p1-1)), line.substr(p2+1) };
        v.push_back(r);
    }
    sort(v.begin(),v.end(),[](auto&a,auto&b){return a.s>b.s;});
    cout<<"\n===== RANKING =====\n";
    if(v.empty()) cout<<"Sem registros.\n";
    for(int i=0;i<(int)v.size() && i<topN;i++)
        cout<<i+1<<") "<<v[i].n<<" - "<<v[i].s<<" ("<<v[i].d<<")\n";
    cout<<"===================\n\n";
}

Difficulty chooseDiff(){
    cout<<"Dificuldade:\n1) Facil (3G,10P,T=300s)\n2) Medio (5G,8P,T=240s)\n3) Dificil (6G,5P,T=120s)\nOpcao: ";
    int o; cin>>o;
    if(o==2) return {"Médio",5,8,240};
    if(o==3) return {"Difícil",6,5,120};
    return {"Fácil",3,10,300};
}

GameState startGame(const string& name,const Difficulty& d){
    GameState st(name,d);
    makeBoard(st.board);
    int hr = GameState::N/2, hc = GameState::N/2;
    if(st.board[hr][hc]=='#'){ hr=1; hc=1; }
    st.hero.setPos(hr,hc);
    dropPills(st.board,d.pills);
    for(int i=0;i<d.ghosts;i++){
        auto p=randomFree(st.board);
        if(abs(p.first-hr)+abs(p.second-hc)<=2){ i--; continue; }
        st.ghosts.push_back(p);
    }
    st.start=chrono::steady_clock::now();
    return st;
}

void loop(GameState& st){
    while(st.running){
        if(st.board[st.hero.row()][st.hero.col()]=='P'){
            st.pillsInBag++;
            st.board[st.hero.row()][st.hero.col()]=' ';
            st.hero.eat("dot");
        }
        draw(st);
        char key = (char)tolower(_getch());
        if(key=='q'){ st.running=false; break; }
        if(key=='b'){
            int k=explode(st,2);
            st.hero.scoreRef() += scoreExplosion(k);
        }else{
            string dir;
            if(key=='w') dir="up"; else if(key=='s') dir="down"; else if(key=='a') dir="left"; else if(key=='d') dir="right";
            if(!dir.empty()){
                int nr = st.hero.row() + (dir=="up"?-1:dir=="down"?1:0);
                int nc = st.hero.col() + (dir=="left"?-1:dir=="right"?1:0);
                if(canStep(st.board,nr,nc)){
                    st.hero.move(dir);
                }
            }
        }
        moveGhosts(st);
        if(caught(st)){ st.running=false; st.victory=false; }
        if(st.ghosts.empty()){ st.running=false; st.victory=true; }
    }
    auto secs = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now()-st.start).count();
    int bonus = max(0, (st.diff.timeLimitSec - (int)secs) * 50);
    st.hero.scoreRef() += bonus;
    cout<<"\n=== FIM === "<<(st.victory?"VITORIA":"DERROTA")
        <<" | Pontos: "<<st.hero.scoreVal()<<" | Tempo: "<<secs<<"s"<<" | Bônus: "<<bonus<<"\n";
    saveRank("Paquiman", st.hero.scoreVal());
}

void menu(){
    while(true){
        cout<<"===== PAC-MAN (Integrado ao Paquiman) =====\n1) Jogar\n2) Ranking\n3) Sair\nOpcao: ";
        int op; if(!(cin>>op)) return;
        if(op==1){
            cout<<"Seu nome: "; string nm; cin>>ws; getline(cin,nm);
            auto d=chooseDiff();
            auto st=startGame(nm,d);
            loop(st);
        }else if(op==2) showRank();
        else if(op==3){ cout<<"Saindo...\n"; return; }
        else cout<<"Opcao invalida.\n";
    }
}

int main(){
    SetConsoleOutputCP(CP_UTF8); SetConsoleCP(CP_UTF8);
    ios::sync_with_stdio(false); cin.tie(nullptr);
    menu();
    return 0;
}

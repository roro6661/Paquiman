#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <chronos>
#include <thread>

using namespace std;

class Paquiman {
    public:
        Paquiman(const string& name) : name(name), score(0), lives(3) {}

        void move(const string& direction) {
            if (direction == "up") y++;
            else if (direction == "down") y--;
            else if (direction == "left") x--;
            else if (direction == "right") x++;
            cout << name << " moved " << direction << " to (" << x << ", " << y << ")\n";
        }

        void eat(const string& item) {
            if (item == "dot") score += 10;
            else if (item == "power-pellet") score += 50;
            cout << name << " ate a " << item << ". Score: " << score << "\n";
        }

        void loseLife() {
            lives--;
            cout << name << " lost a life. Lives left: " << lives << "\n";
            if (lives <= 0) {
                cout << name << " has no more lives. Game Over!\n";
            }
        }

        void displayStatus() const {
            cout << "Status of " << name << ": Position(" << x << ", " << y 
                 << "), Score: " << score << ", Lives: " << lives << "\n";
        }
    private:
        string name;
        int x = 0, y = 0;
        int score;
        int lives;
};
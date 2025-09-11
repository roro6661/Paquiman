#ifdef _WIN32
#include <windows.h>
extern "C" int main();
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return main();
}
#endif
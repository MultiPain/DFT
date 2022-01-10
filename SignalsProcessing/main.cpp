#include "SPU.h"

int main(int argc, char const* argv[]) {
    SPU app("Signals processing", 1280, 720);
    app.Run();
    return 0;
}
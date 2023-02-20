#include "avio.h"

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cout << "Usage: sample-cmd filename" << std::endl;
        return 1;
    }

    std::cout << "playing file: " << argv[1] << std::endl;

    avio::Player player;
    player.uri = argv[1];
    player.width = []() { return 1280; };
    player.height = []() { return 720; };

    player.run();

    return 0;
}
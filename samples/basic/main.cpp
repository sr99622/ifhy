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

    /*
    avio::Reader reader(argv[1]);
    avio::Decoder videoDecoder(reader, AVMEDIA_TYPE_VIDEO, AV_HWDEVICE_TYPE_NONE);
    avio::Filter videoFilter(videoDecoder, "null");
    avio::Decoder audioDecoder(reader, AVMEDIA_TYPE_AUDIO);
    avio::Filter audioFilter(audioDecoder, "anull");
    avio::Display display(reader);
    player.add_reader(reader);
    player.add_decoder(videoDecoder);
    player.add_filter(videoFilter);
    player.add_decoder(audioDecoder);
    player.add_filter(audioFilter);
    player.add_display(display);
    */

    player.run();

    return 0;
}
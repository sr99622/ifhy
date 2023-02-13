#include "avio.h"

int main(int argc, char** argv)
{
    avio::Player player;

    std::string filename;
#ifdef _WIN32 
    filename = "C:/Users/sr996/Videos/news.mp4";
#else
    filename = "/home/stephen/Videos/news.mp4";
#endif

    avio::Reader reader(filename.c_str());
    reader.set_video_out("vpq_reader");
    reader.set_audio_out("apq_reader");

    avio::Decoder videoDecoder(reader, AVMEDIA_TYPE_VIDEO, AV_HWDEVICE_TYPE_NONE);
    videoDecoder.set_video_in(reader.video_out());
    videoDecoder.set_video_out("vfq_decoder");

    //avio::Filter videoFilter(videoDecoder, "scale=1280x720,format=rgb24");
    avio::Filter videoFilter(videoDecoder, "null");
    videoFilter.set_video_in(videoDecoder.video_out());
    videoFilter.set_video_out("vfq_filter");
    //videoFilter.show_frames = true;

    avio::Decoder audioDecoder(reader, AVMEDIA_TYPE_AUDIO);
    audioDecoder.set_audio_in(reader.audio_out());
    audioDecoder.set_audio_out("afq_decoder");

    avio::Display display(reader);
    display.set_video_in(videoFilter.video_out());
    //display.set_video_in(videoDecoder.video_out());
    display.set_audio_in(audioDecoder.audio_out());

    player.add_reader(reader);
    player.add_decoder(videoDecoder);
    player.add_filter(videoFilter);
    player.add_decoder(audioDecoder);
    player.add_display(display);

    player.run();

    return 0;
}
#include "Player.h"
#include "avio.h"

namespace avio
{

bool Player::isPaused()
{
    bool result = false;
    if (display) result = display->paused;
    return result;
}

bool Player::isPiping()
{
    bool result = false;
    if (reader) result = reader->request_pipe_write;
    return result;
}

void Player::setMute(bool arg)
{
    if (display) display->mute = arg;
}

void Player::setVolume(int arg)
{
    if (display) display->volume = (float)arg / 100.0f;
}

void Player::togglePaused()
{
    if (display) display->togglePause();
}

void Player::seek(float arg)
{
    if (reader) reader->request_seek(arg);
}

void Player::toggle_pipe_out(const std::string& filename)
{
    if (reader) {
        reader->pipe_out_filename = filename;
        reader->request_pipe_write = !reader->request_pipe_write;
    }
}

void Player::clear_queues()
{
    if (reader->vpq)  reader->vpq->clear();
    if (reader->apq)  reader->apq->clear();
    if (videoDecoder) videoDecoder->frame_q->clear();
    if (videoFilter)  videoFilter->frame_out_q->clear();
    if (audioDecoder) audioDecoder->frame_q->clear();
    if (audioFilter)  audioFilter->frame_out_q->clear();
}

void Player::clear_decoders()
{
    if (videoDecoder) videoDecoder->flush();
    if (audioDecoder) audioDecoder->flush();
}

void Player::key_event(int keyCode)
{
    SDL_Event event;
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = keyCode;
    SDL_PushEvent(&event);
}

bool Player::checkForStreamHeader(const char* name)
{
    /*
    QString str = QString(name).toLower();
    if (str.startsWith("rtsp://"))
        return true;
    if (str.startsWith("http://"))
        return true;
    if (str.startsWith("https://"))
        return true;
    */
    return false;
}

void Player::twink(void* caller)
{
    Player* player = (Player*)caller;
    player->run();
}

void Player::start()
{
    std::thread process_thread([&]() { run(); });
    process_thread.detach();
}

void Player::run()
{
    try {
        running = true;

        vpq_reader  = new Queue<Packet>;
        vfq_decoder = new Queue<Frame>;
        vfq_filter  = new Queue<Frame>;
        apq_reader  = new Queue<Packet>;
        afq_decoder = new Queue<Frame>;
        afq_filter  = new Queue<Frame>;

        reader = new Reader(uri.c_str());
        reader->showStreamParameters();
        const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(reader->pix_fmt());
        if (!desc) throw avio::Exception("No pixel format in video stream");

        if (reader->has_video()) {
            reader->vpq = vpq_reader;

            videoDecoder = new Decoder(*reader, AVMEDIA_TYPE_VIDEO, hw_device_type);
            videoDecoder->pkt_q = vpq_reader;
            videoDecoder->frame_q = vfq_decoder;

            videoFilter = new Filter(*videoDecoder, video_filter.c_str());
            videoFilter->frame_in_q = vfq_decoder;
            videoFilter->frame_out_q = vfq_filter;
        }

        if (reader->has_audio()) {
            reader->apq = apq_reader;

            audioDecoder = new Decoder(*reader, AVMEDIA_TYPE_AUDIO);
            audioDecoder->pkt_q = apq_reader;
            audioDecoder->frame_q = afq_decoder;

            audioFilter = new Filter(*audioDecoder, audio_filter.c_str());
            audioFilter->frame_in_q = afq_decoder;
            audioFilter->frame_out_q = afq_filter;
        }

        if(checkForStreamHeader(uri.c_str())) {
            if (vpq_size) reader->apq_max_size = vpq_size;
            if (apq_size) reader->vpq_max_size = vpq_size;
        }

        ops.push_back(new std::thread(read, reader, this));
        if (videoDecoder) ops.push_back(new std::thread(decode, videoDecoder));
        if (videoFilter)  ops.push_back(new std::thread(filter, videoFilter));
        if (audioDecoder) ops.push_back(new std::thread(decode, audioDecoder));
        if (audioFilter)  ops.push_back(new std::thread(filter, audioFilter));

        display = new Display(*reader);
        if (videoFilter) display->vfq_in = videoFilter->frame_out_q;
        if (audioFilter) {
            display->afq_in = audioFilter->frame_out_q;
            display->initAudio(audioFilter);
        }
        display->player = this;
        display->hWnd = hWnd;

        if (cbMediaPlayingStarted) cbMediaPlayingStarted(reader->duration());
        while (display->display()) {}
        running = false;

        std::cout << "display done" << std::endl;
    }
    catch (const Exception e) {
        if (errorCallback) {
            std::stringstream str;
            str << "avio player error: " << e.what();
            errorCallback(str.str());
        }
    }

    if (reader) {
        if (reader->vpq) reader->vpq->close();
        if (reader->apq) reader->apq->close();
    }

    for (int i = 0; i < ops.size(); i++) {
        ops[i]->join();
        delete ops[i];
    }

    if (reader)       delete reader;
    if (videoFilter)  delete videoFilter;
    if (videoDecoder) delete videoDecoder;
    if (audioFilter)  delete audioFilter;
    if (audioDecoder) delete audioDecoder;
    if (display)      delete display;

    delete  vpq_reader;
    delete  vfq_decoder;
    delete  vfq_filter;
    delete  apq_reader;
    delete  afq_decoder;
    delete  afq_filter;

    if (cbMediaPlayingStopped) cbMediaPlayingStopped();


}

}
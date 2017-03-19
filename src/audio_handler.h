#ifndef audio_handler_h_INCLUDED
#define audio_handler_h_INCLUDED

#include <pulse/pulseaudio.h>

namespace peerspeak {

class AudioHandler {
public:
    AudioHandler();
    ~AudioHandler();

    // Function for receiving audio from connections
    void recv_audio(/*TODO parameters*/);

    // Function for playing sound effects
    void play_sound(/*TODO parameters*/);

private:
    pa_mainloop *loop;
    pa_mainloop_api *loop_api;
    pa_stream *out_stream;
};

} // namespace peerspeak

#endif // audio_handler_h_INCLUDED

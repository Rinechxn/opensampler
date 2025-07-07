#include <iostream>
#include <alsa/asoundlib.h>
#include <sfizz.h>
#include <unistd.h> // for getcwd

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <sfz_file> [midi_note]" << std::endl;
        return 1;
    }

    const char* sfzFile = argv[1];
    int midiNote = argc > 2 ? std::stoi(argv[2]) : 60;

    // ALSA PCM init
    snd_pcm_t* pcmHandle;
    int err = snd_pcm_open(&pcmHandle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        std::cerr << "ALSA open error: " << snd_strerror(err) << std::endl;
        return 1;
    }

    err = snd_pcm_set_params(pcmHandle,
        SND_PCM_FORMAT_FLOAT_LE,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        2, // stereo
        44100, // sample rate
        1,     // allow resample
        500000 // latency (us)
    );
    if (err < 0) {
        std::cerr << "ALSA set_params error: " << snd_strerror(err) << std::endl;
        snd_pcm_close(pcmHandle);
        return 1;
    }

    // sfizz init
    sfizz_synth_t* synth = sfizz_create_synth();
    if (!synth) {
        std::cerr << "Failed to create sfizz synth!" << std::endl;
        snd_pcm_close(pcmHandle);
        return 1;
    }

    sfizz_set_sample_rate(synth, 44100.0f);
    sfizz_set_samples_per_block(synth, 512);

    std::cout << "Loading SFZ: " << sfzFile << std::endl;

    // Print current working directory for debugging
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::cout << "Current working directory: " << cwd << std::endl;
    }

    // Check if file exists
    FILE* testFile = fopen(sfzFile, "r");
    if (!testFile) {
        std::cerr << "Error: SFZ file does not exist or cannot be opened: " << sfzFile << std::endl;
        sfizz_free(synth);
        snd_pcm_close(pcmHandle);
        return 1;
    }
    fclose(testFile);

    int loadResult = sfizz_load_file(synth, sfzFile);
    if (loadResult != 0) {
        std::cerr << "Failed to load SFZ file: " << sfzFile << std::endl;
        std::cerr << "Possible reasons: missing samples, bad paths, or unsupported SFZ format." << std::endl;

        // Try to give more hints about missing samples
        std::cerr << "Hint: Check if the sample paths in the SFZ file are relative to the SFZ file location." << std::endl;
        std::cerr << "      If not, try moving the SFZ file to the same directory as its samples, or adjust the sample paths." << std::endl;

        sfizz_free(synth);
        snd_pcm_close(pcmHandle);
        return 1;
    }

    // Assume stereo output (2 channels) as required by the C API
    sfizz_send_note_on(synth, 0, midiNote, 127); // channel 0, note, velocity

    constexpr int blockSize = 512;
    float* outputs[2];
    float* left = new float[blockSize]();
    float* right = new float[blockSize]();
    outputs[0] = left;
    outputs[1] = right;

    for (int i = 0; i < 100; ++i) {
        std::fill_n(left, blockSize, 0.0f);
        std::fill_n(right, blockSize, 0.0f);

        sfizz_render_block(synth, outputs, 0, blockSize);

        float interleaved[blockSize * 2];
        for (int f = 0; f < blockSize; ++f) {
            interleaved[f * 2] = left[f];
            interleaved[f * 2 + 1] = right[f];
        }

        err = snd_pcm_writei(pcmHandle, interleaved, blockSize);
        if (err < 0) {
            std::cerr << "ALSA write error: " << snd_strerror(err) << std::endl;
            snd_pcm_prepare(pcmHandle);
        }
    }

    sfizz_send_note_off(synth, 0, midiNote, 0);
    delete[] left;
    delete[] right;
    sfizz_free(synth);
    snd_pcm_drain(pcmHandle);
    snd_pcm_close(pcmHandle);

    std::cout << "✅ Done playing: " << sfzFile << std::endl;
    return 0;
}

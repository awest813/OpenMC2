#include "doctest/doctest.h"

#include "gameos.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <vector>

namespace {

// 16 bit mono PCM WAV with a short sine tone
void write_test_wav(const char* path)
{
    const uint32_t rate = 22050;
    std::vector<int16_t> samples(rate / 10);
    for (size_t i = 0; i < samples.size(); ++i)
        samples[i] = (int16_t)(8000.0 * std::sin(i * 2.0 * 3.14159265 * 440.0 / rate));

    const uint32_t data_size = (uint32_t)(samples.size() * sizeof(int16_t));
    std::ofstream f(path, std::ios::binary);
    auto u32 = [&](uint32_t v) { f.write((const char*)&v, 4); };
    auto u16 = [&](uint16_t v) { f.write((const char*)&v, 2); };
    f.write("RIFF", 4); u32(36 + data_size); f.write("WAVE", 4);
    f.write("fmt ", 4); u32(16); u16(1); u16(1); u32(rate); u32(rate * 2); u16(2); u16(16);
    f.write("data", 4); u32(data_size);
    f.write((const char*)samples.data(), data_size);
}

struct AudioFixture {
    bool ok;
    AudioFixture()
    {
        // no sound card needed
#ifdef _WIN32
        _putenv_s("SDL_AUDIODRIVER", "dummy");
#else
        setenv("SDL_AUDIODRIVER", "dummy", 1);
#endif
        ok = gos_CreateAudio();
        write_test_wav("beep.wav");
    }
    ~AudioFixture() { gos_DestroyAudio(); }
};

} // namespace

TEST_CASE("Audio: volume uses the full SDL_mixer range without saturating")
{
    AudioFixture audio;
    REQUIRE(audio.ok);

    HGOSAUDIO res = NULL;
    gosAudio_CreateResource(&res, gosAudio_StreamedFile, "beep.wav");
    REQUIRE(res != NULL);
    gosAudio_AssignResourceToChannel(0, res);

    gosAudio_SetChannelSlider(0, gosAudio_Volume, 1.0f);
    CHECK(Mix_Volume(0, -1) == MIX_MAX_VOLUME);
    gosAudio_SetChannelSlider(0, gosAudio_Volume, 0.5f);
    CHECK(Mix_Volume(0, -1) == MIX_MAX_VOLUME / 2);
    gosAudio_SetChannelSlider(0, gosAudio_Volume, 0.0f);
    CHECK(Mix_Volume(0, -1) == 0);

    float volume = -1.0f;
    gosAudio_GetChannelSlider(0, gosAudio_Volume, &volume);
    CHECK(volume == 0.0f);

    gosAudio_DestroyResource(&res);
    CHECK(res == NULL);
}

TEST_CASE("Audio: panning can be read back")
{
    AudioFixture audio;
    REQUIRE(audio.ok);

    gosAudio_SetChannelSlider(1, gosAudio_Panning, 0.25f);
    float pan = 0.0f;
    gosAudio_GetChannelSlider(1, gosAudio_Panning, &pan);
    CHECK(pan == 0.25f);
}

TEST_CASE("Audio: resources that were never assigned to a channel can be destroyed")
{
    // they used to stay in the engine's list until shutdown; this checks the destroy path is clean
    // (no double free / use after free under ASan)
    AudioFixture audio;
    REQUIRE(audio.ok);

    for (int i = 0; i < 3; ++i) {
        HGOSAUDIO res = NULL;
        gosAudio_CreateResource(&res, gosAudio_StreamedFile, "beep.wav");
        REQUIRE(res != NULL);
        gosAudio_DestroyResource(&res);
        CHECK(res == NULL);
    }
}

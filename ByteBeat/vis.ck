// TODO:
// add waveform history
// add spectrum history
// for bytebeat: visualize the bits!!

public class Vis {
    512 => int WINDOW_SIZE;
    8 => int WATERFALL_DEPTH;
    PoleZero dcbloke => FFT fft => blackhole;
    .95 => dcbloke.blockZero;

    Windowing.hann(WINDOW_SIZE) => fft.window;
    WINDOW_SIZE*2 => fft.size;

    complex response[0];  // spectrum
    float spectrum_hist_positions[WINDOW_SIZE]; // remapped spectrum

    int sample_accum_w_idx;
    UI_Int waveform_w_pixels(800);
    UI_Float window_dur_sec(.125);
    float sample_accum[waveform_w_pixels.val()];

    UGen@ u;

    int paused;

    // graphics ==========================

    ShaderDesc desc;
    me.dir() + "./bit_shader.wgsl" => desc.vertexPath;
    me.dir() + "./bit_shader.wgsl" => desc.fragmentPath;
    null => desc.vertexLayout;

    Shader shader(desc);
    shader.name("bytebeat screen shader");

    // audio texture
    TextureDesc audio_texture_desc;
    1024 => audio_texture_desc.width;
    WATERFALL_DEPTH => audio_texture_desc.height; // eventually make this WATERFALL_DEPTH
    // single channel float (to hold spectrum data)
    Texture.FORMAT_R32FLOAT => audio_texture_desc.format; // TODO test with r32uint format
    // no mipmaps
    0 => audio_texture_desc.mips;
    // create the texture
    Texture audio_texture(audio_texture_desc);
    float bytebeat_sample_accum[audio_texture.width()];
    int bytebeat_sample_accum_idx;
    int bytebeat_waterfall_idx;

    TextureWriteDesc write_desc;
    audio_texture.width() => write_desc.width;

    // render graph
    GG.rootPass() --> ScreenPass screen_pass(shader);
    screen_pass.material().texture(0, audio_texture);
    // screen_pass.material().sampler(1, TextureSampler.nearest());

    // =============================

    fun @construct( UGen u ) { 
        u => dcbloke;
        u => dac;
        u @=> this.u;
    }

    // map FFT output
    fun void map2spectrum( complex in[], float out[] )
    {
        for(int i; i < in.size(); i++)
            5 * Math.sqrt( (in[i]$polar).mag * 25 ) => out[i];
    }

    fun void waveformShred() {
        while (1) {
            if (!paused) {
                this.u.last() => sample_accum[sample_accum_w_idx++ % sample_accum.size()];
            }
            (window_dur_sec.val() / waveform_w_pixels.val())::second => now;
        }
    } spork ~ waveformShred();

    fun void audioAnalyzerShred()
    {
        while( true ) {
            if (!paused) {
                fft.upchuck();
                fft.spectrum( response );
            }
            WINDOW_SIZE::samp => now;
        }
    } spork ~ audioAnalyzerShred();

    fun void bitShred() {
        while (1) {
            if (!paused) {
                this.u.last() => bytebeat_sample_accum[bytebeat_sample_accum_idx++ % bytebeat_sample_accum.size()];
            }
            (1.0/8000)::second => now;
            // write to tex
            if (bytebeat_sample_accum_idx == bytebeat_sample_accum.size()) {
                bytebeat_waterfall_idx => write_desc.y;
                audio_texture.write(bytebeat_sample_accum, write_desc);
                0 => bytebeat_sample_accum_idx;

                bytebeat_waterfall_idx++;
                if (bytebeat_waterfall_idx == WATERFALL_DEPTH) {
                    0 => bytebeat_waterfall_idx;
                }
            }
        }
    } 
    spork ~ bitShred();

    fun void run() {
        while (1) {
            GG.nextFrame() => now;

            UI.begin("");
            // UI.plotLines("##Waveform", samples);
            UI.plotLines(
                "##Waveform", sample_accum, 0, 
                "Waveform", -1, 1, 
                @(waveform_w_pixels.val(), 200)
                // @(1000 * window_dur_sec, 200)
            );

            UI.sameLine();
            UI.vslider("Time##Time", @(50, 200), window_dur_sec, .01, 2);
            UI.sameLine();
            if (UI.vslider("Size##Size", @(50, 200), waveform_w_pixels, 50, 1000)) {
                sample_accum.clear();
                waveform_w_pixels.val() => sample_accum.size;
            }


            UI.dummy(@(0.0f, 20.0f)); // vertical spacing

            // plot spectrum as histogram
            map2spectrum( response, spectrum_hist_positions );
            UI.plotHistogram(
                "##Spectrum", spectrum_hist_positions, 0, "512 bins", 0, 8, @(0, 200)
            );

            if (UI.button(paused ? "play" : "pause")) {
                !paused => paused;
            }

            UI.end();
        }
    }
}


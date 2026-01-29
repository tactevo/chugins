@import "Binaural.chug"

CNoise noise => Binaural binaural => dac;
"pink" => noise.mode;
0.25 => noise.gain;

UI_Float azimuth, elevation;

while (true) {
    GG.nextFrame() => now;

    if (UI.begin("Window")) {
        if (UI.slider("azimuth", azimuth, 0, 360)) azimuth.val() => binaural.azimuth;
        if (UI.slider("elevation", elevation, -90, 90)) elevation.val() => binaural.elevation;
    } UI.end();
}
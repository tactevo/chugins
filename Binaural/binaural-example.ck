@import "Binaural.chug"

fun void waves(dur length)
{
    CNoise noise => Binaural binaural => dac;
    0.25 => noise.gain;
    "pink" => noise.mode;
    0 => binaural.azimuth;
    0 => binaural.elevation;

    0 => float ele;
    now + length => time sc1;
    while( now < sc1 )
    {
        (binaural.azimuth() + 1) % 360 => binaural.azimuth;
        <<< "Azi, Ele:", binaural.azimuth(), binaural.elevation() >>>;
        20::ms => now;
    }
}


fun void tones(int nBeeps)
{
    SqrOsc s => Binaural binaural => dac;
    0.75 => s.gain;
    for( 0 => int foo; foo < nBeeps ; foo++ )
    {
        if (foo % 2)
            0 => s.gain;
        else
        {
            Math.random2f(0, 360) => binaural.azimuth;
            Math.random2f(-90, 90) => binaural.elevation;
            0.5 => s.gain;
            Math.random2f(50, 200) => s.freq;
        }
        0.5::second => now;
        
    }
}

fun void sweeps(int nSweeps)
{
    SinOsc s => Binaural binaural => dac;
    0.5 => s.gain;
    for (0 => int i; i < nSweeps; i++)
    {
        (binaural.azimuth() + 45) % 360 => binaural.azimuth;
        for (0 => int j; j < 90; j++)
        {
            -45 + j => binaural.elevation;
            10*j + 1000 => s.freq;
            2::ms => now;
        }
        0.5 - (0.5/nSweeps)*i => s.gain;
        
    }
}

spork ~waves(75::second);
10::second => now;
spork ~tones(200);
30::second => now;
spork ~sweeps(50);
30::second => now;

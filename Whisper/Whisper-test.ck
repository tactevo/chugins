@import "build/Whisper"

adc => Whisper w(me.dir() + "./models/ggml-base.en.bin") => DelayL delay => dac;
.75::second => delay.max => delay.delay;

<<< "whisper internal srate: ", w.SRATE >>>;

500::ms => dur step;
step => now;
while (1) {
    now + step => time later;
    w.transcribe() => now; 
    <<< w.text() >>>;
    if (now < later) later - now => now; // wait for next step
}

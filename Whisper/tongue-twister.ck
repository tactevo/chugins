@import "build/Whisper"
@import "../../chugl/games/lib/g2d/ChuGL.chug"
@import "../../chugl/games/lib/T.ck"

GG.camera().orthographic();
GG.camera().viewSize(10);
GG.rootPass() --> ScenePass sp(GG.scene());

/*
Bugs:
- punctuation. strip it. (except for things like [humming], [crying] etc)

*/


GText last_recognized --> GG.scene();
last_recognized.text("yo what");
last_recognized.color(Color.GREEN);
last_recognized.size(.5);
last_recognized.align(1);
last_recognized.maxWidth(GG.camera().viewSize() * GG.camera().aspect());
last_recognized.posY(.33 * GG.camera().viewSize());

GText target --> GG.scene();
target.size(.5);
target.align(1);
target.maxWidth(GG.camera().viewSize() * GG.camera().aspect());
target.posY(-.25 * GG.camera().viewSize());
"Peter Piper picked a peck of pickled peppers.  A peck of pickled peppers Peter Piper picked.  If Peter Piper picked a peck of pickled peppers, Where’s the peck of pickled peppers Peter Piper picked?" => string target_text;
target.text(target_text);


GText transcribed_text --> GG.scene();
transcribed_text.size(.6);
transcribed_text.align(1);
transcribed_text.maxWidth(GG.camera().viewSize() * GG.camera().aspect());
transcribed_text.text("");

adc => Whisper w(me.dir() + "./models/ggml-base.en.bin") => blackhole;

fun void transcriber() {
    500::ms => dur step;
    step => now;
    while (1) {
        now + step => time later;
        w.transcribe() => now; 

        transcribed_text.text(w.text());
        match();

        if (now < later) later - now => now; // wait for next step
    }
} spork ~ transcriber();

fun void match() {
    if (target_text == null || target_text.length() == 0) return;

    // get next word
    T.assert(target_text.ltrim().length() == target_text.length(), "no leading whitespace allowed!");
    target_text.find(' ') => int first_space;
    string target_word;
    if (first_space >= 0) {
        target_text.substring(0, first_space) => target_word;
    } else {
        target_text => target_word;
    }
    // sanitize
    target_word.lower() => target_word;

    <<< "=======looking for target world:", target_word >>>;

    // check if word has been said
    transcribed_text.text().trim().lower() => string curr_transcription;
    if (curr_transcription != null && curr_transcription.length() > 0) {
        curr_transcription.find(target_word) => int word_idx;
        <<< "=======found target world<", target_word, "> within transcription: ", curr_transcription, "----at idx", word_idx >>>;
        if (word_idx >= 0) {
            // found, update last_recognized and pop off front of target text
            target_text.substring(0, target_word.length()) => last_recognized.text;
            target_text.erase(0, target_word.length());
            target_text.ltrim() => target_text;
            target_text => target.text;
        }
    }
}

while (1) {
    GG.nextFrame() => now;

    // check if anything that was said matches the target text, if so remove it



}

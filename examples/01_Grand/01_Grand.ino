// upright example 01: grand
// one piano, the grand preset, struck at middle c every two seconds and let
// go after one. teensy 4.x and the audio shield (sgtl5000): the piano on
// both channels.

#include <Audio.h>
#include <Upright.h>

AudioSynthUpright    piano;
AudioOutputI2S       i2s;
AudioConnection      cordLeft(piano, 0, i2s, 0), cordRight(piano, 0, i2s, 1);
AudioControlSGTL5000 codec;

void setup() {
    AudioMemory(8);
    codec.enable();
    codec.volume(0.5f);
    up::Tuning t;
    t.outputGain = 4.0f;            // the piano to full scale: this note peaks near 0.44
    piano.setTuning(t);
    piano.setShape(up::grand());    // the instrument
}

void loop() {
    piano.noteOn(60.0f, 0.8f);      // middle c, velocity 0.8
    delay(1000);
    piano.noteOff();                // the key let go: its damper lands
    delay(1000);
}

// ----------------------------------------------------------------------------
// upright example 03 — Keyboard  (plan: module layout)
//
// the pool on usb midi: eight voices and one soundboard, one piano on a
// keyboard. a key strikes the string at that note; struck again while it
// sounds, a fresh hammer meets its moving string on the same voice; a ninth
// key steals the quietest voice, which fades out (about 80 ms) before it
// sounds the new note. every voice's bridge drives the one board, each where
// its string crosses the bridge.
//
//   note on / off       a strike at the key's velocity (velocity 0 is a note
//                       off); a note off lands the key's damper
//   cc 64, the pedal    pressed (64 and up) pedal(1): every damper held off,
//                       a released key rings on; released pedal(0): the
//                       dampers land on every key already let go
//   program change      a preset: 0 grand, 1 concertGrand, 2 upright,
//                       3 spinet, 4 honkyTonk, 5 fortepiano, 6 rhodes,
//                       7 rhodesBark, 8 wurlitzer (and on, modulo nine)
//   cc 123              all notes off
// a new preset's shape is written whole (setShape) to every voice and to
// every note after it, and the board's plate to its body; the voices ease
// there under the ringing keys.
//
// teensy 4.1 and the audio shield (sgtl5000), and usb: the teensy is a usb
// midi device, and a usb audio one too. its usb type must carry midi: in the
// arduino ide, tools > usb type > "serial + midi + audio"; under platformio,
// build_flags = -D USB_MIDI_AUDIO_SERIAL (make board reads the line below).
// the pool's one output goes to both channels of the shield and of usb
// audio. once a second the serial monitor shows the piano, the voices
// sounding and the cpu.
//
// the output gain is 0.4, holding the loudest preset's worst chord under
// full scale (the pool's sum is clamped): measured on the host, eight keys
// struck in one block evenly across a preset's range at velocity 1 (the worst
// case: their onsets add) peak at 2.27 of full scale for rhodesBark at a gain
// of 1, 0.97 for rhodes, 0.59 for grand and 0.22 for wurlitzer; so here at
// most 0.91. the presets differ in level (wurlitzer's chord 20 db under
// rhodesBark's): the gain is one for all. a chord played by hand lands over
// several blocks and peaks lower.
//
// make board: -DUSB_MIDI_AUDIO_SERIAL
// ----------------------------------------------------------------------------

#include <Audio.h>
#include <Upright.h>

AudioSynthUprightPool<8> keyboard(1);
AudioOutputI2S           i2s;
AudioOutputUSB           usb;
AudioConnection          cordLeft(keyboard, 0, i2s, 0);
AudioConnection          cordRight(keyboard, 0, i2s, 1);
AudioConnection          cordUsbLeft(keyboard, 0, usb, 0);
AudioConnection          cordUsbRight(keyboard, 0, usb, 1);
AudioControlSGTL5000     codec;

const float kGain = 0.4f;                   // the pool's output gain

int       preset = 0;
uint32_t  lastPrintMs = 0;

void onNoteOn(byte channel, byte note, byte velocity) {
    (void)channel;
    if (velocity == 0) keyboard.noteOff((float)note);
    else keyboard.noteOn((float)note, (float)velocity / 127.0f);
}

void onNoteOff(byte channel, byte note, byte velocity) {
    (void)channel;
    (void)velocity;
    keyboard.noteOff((float)note);
}

void onControlChange(byte channel, byte control, byte value) {
    (void)channel;
    if (control == 64) {
        keyboard.pedal(value >= 64 ? 1.0f : 0.0f);           // the dampers held off, or let down
    } else if (control == 123) {
        keyboard.allNotesOff();
    }
}

void onProgramChange(byte channel, byte program) {
    (void)channel;
    preset = program % up::presetCount();
    keyboard.setShape(up::presetAt(preset).shape());          // every voice, every later note and the board
}

void setup() {
    Serial.begin(115200);

    AudioMemory(16);
    codec.enable();
    codec.volume(0.5f);

    up::Tuning t;
    t.outputGain = kGain;
    keyboard.setTuning(t);
    keyboard.setShape(up::presetAt(preset).shape());

    usbMIDI.setHandleNoteOn(onNoteOn);
    usbMIDI.setHandleNoteOff(onNoteOff);
    usbMIDI.setHandleControlChange(onControlChange);
    usbMIDI.setHandleProgramChange(onProgramChange);
}

void loop() {
    while (usbMIDI.read()) {}

    const uint32_t now = millis();
    if (now - lastPrintMs >= 1000u) {
        lastPrintMs = now;
        Serial.print(up::presetAt(preset).name);
        Serial.print("   voices ");
        Serial.print(keyboard.sounding());
        Serial.print(" of ");
        Serial.print(keyboard.voices());
        Serial.print("   cpu max ");
        Serial.print(AudioProcessorUsageMax(), 2);
        Serial.print(" %");
        if (keyboard.dropped()) {
            Serial.print("   dropped ");
            Serial.print(keyboard.dropped());
        }
        Serial.println();
        AudioProcessorUsageMaxReset();
    }
}

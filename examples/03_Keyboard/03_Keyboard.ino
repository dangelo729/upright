// upright example 03: keyboard
// eight voices and one soundboard played from usb midi. a key struck again
// while it sounds meets its moving string; a ninth key steals the quietest
// voice, faded out first. once a second the serial monitor shows the preset,
// the voices sounding and the cpu.
//
//   note on / off    a strike at the key's velocity; the key let go
//   cc 64            the sustain pedal: 64 and up down, under 64 up
//   program change   a preset: 0 grand, 1 concertGrand, 2 upright, 3 spinet,
//                    4 honkyTonk, 5 fortepiano, 6 rhodes, 7 rhodesBark,
//                    8 wurlitzer; higher programs wrap around
//   cc 123           all notes off
//
// teensy 4.x and the audio shield (sgtl5000); the pool on both channels of
// the shield and of usb audio. the usb type must carry midi and audio: in the
// arduino ide, tools > usb type > "serial + midi + audio"; under platformio,
// build_flags = -D USB_MIDI_AUDIO_SERIAL.
//
// make board: -DUSB_MIDI_AUDIO_SERIAL

#include <Audio.h>
#include <Upright.h>

AudioSynthUprightPool<8> keyboard;
AudioOutputI2S           i2s;
AudioOutputUSB           usb;
AudioConnection          cordLeft(keyboard, 0, i2s, 0);
AudioConnection          cordRight(keyboard, 0, i2s, 1);
AudioConnection          cordUsbLeft(keyboard, 0, usb, 0);
AudioConnection          cordUsbRight(keyboard, 0, usb, 1);
AudioControlSGTL5000     codec;

const float kGain = 0.4f;                   // the pool's output gain: eight keys struck at once stay under full scale

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
        keyboard.setPedal(value >= 64 ? 1.0f : 0.0f);        // the dampers held off, or let down
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

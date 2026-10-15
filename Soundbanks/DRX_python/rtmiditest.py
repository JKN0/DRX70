import time
import rtmidi

#Create midi out object
midiout = rtmidi.MidiOut()
available_ports = midiout.get_ports()

if available_ports:
    print("Available MIDI out ports:")
    for portnr, name in enumerate(available_ports):
        print("[%i] %s" % (portnr, name))
        if name.startswith('DRX-70'):
            drx_port = portnr
    print('')
else:
    print("No available out ports")

midi_in = rtmidi.MidiIn()
available_ports = midi_in.get_ports()

if available_ports:
    print("Available MIDI in ports:")
    for portnr, name in enumerate(available_ports):
        print("[%i] %s" % (portnr, name))
    print('')
else:
    print("No available in ports")

midiout.open_port(2)
#with midiout:
note_on = [0x90, 60, 112] # channel 1, middle C, velocity 112
note_off = [0x80, 60, 0]
print("Sending note on/off")
midiout.send_message(note_on)
time.sleep(0.5)
midiout.send_message(note_off)
time.sleep(0.1)

del midiout
#del midi_in

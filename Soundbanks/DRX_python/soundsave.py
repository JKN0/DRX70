import time
import rtmidi

channel = 1

first_patch = 'A25'
last_patch = 'A29'

print_port_list = False

OK = 0
FAIL = 1

# =====================================================================
# Open midi ports
# --------------------------------------------------------------------- 
    
def open_midi_ports():
    global midiout
    global midi_in

    #Create midi objects
    midiout = rtmidi.MidiOut()
    midi_in = rtmidi.MidiIn()

    # Open midi out port
    drx_port = None
    available_ports = midiout.get_ports()
    if available_ports:
        if print_port_list:
            print()
            print("Available MIDI out ports:")

        for portnr, name in enumerate(available_ports):
            if print_port_list:
                print("[%i] %s" % (portnr, name))

            #if name.startswith('loopMIDI'):
            if name.startswith('DRX-70'):
                drx_port = portnr

        if print_port_list:
            print('')
    else:
        print("No available out ports")

    if drx_port == None:
        print('DRX-70 out port not found')
        return FAIL
    else:
        print("Using out port:",drx_port)
        midiout.open_port(drx_port)

    # Open midi in port
    drx_port = None
    available_ports = midi_in.get_ports()
    if available_ports:

        if print_port_list:
            print()
            print("Available MIDI in ports:")
        
        for portnr, name in enumerate(available_ports):
            if print_port_list:
                print("[%i] %s" % (portnr, name))

            #if name.startswith('loopM2'):
            if name.startswith('DRX-70'):
                drx_port = portnr

        if print_port_list:
            print('')
    else:
        print("No available in ports")

    if drx_port == None:
        print('DRX-70 in port not found')
        return FAIL
    else:
        print("Using in port: ",drx_port)
        midi_in.open_port(drx_port)

    return OK

# =====================================================================
# Get one patch from DRX70:
# - send midi patch change
# - send sysex "get patch"
# - wait for sysex "patch upload"
# --------------------------------------------------------------------- 
    
def get_patch_from_drx(pn):
    #print('Getting patch',pn)

    print(patch_nr_to_id(pn),end=' ')

    send_midi_patch_change(pn)
    time.sleep(0.2)
    send_midi_get_current_patch()
    time.sleep(0.2)

# =====================================================================
# Send midi patch change to DRX
# --------------------------------------------------------------------- 
    
def send_midi_patch_change(pn):
    bank = int(pn/128)
    pnr = pn % 128

    # Set bank: ctrl change 32 (bank select LSB)
    midi_msg = [ 0xB0 + channel - 1, 32, bank ]
    #print('Sending:',midi_msg)
    midiout.send_message(midi_msg)

    # Program change 
    midi_msg = [ 0xC0 + channel - 1, pnr ]
    #print('Sending:',midi_msg)
    midiout.send_message(midi_msg)

# =====================================================================
# Send midi sysex "request current patch"
# --------------------------------------------------------------------- 
    
def send_midi_get_current_patch():

    midi_msg = [ 0xF0,0x7D,0x06,0xF7 ]
    #print('Sending:',midi_msg)
    midiout.send_message(midi_msg)

# =====================================================================
# Callback for midi in messages (only expecting "upload current patch")
# --------------------------------------------------------------------- 
    
def midi_in_callback(msg_and_time,data):
    msg,mtime = msg_and_time
    
    #print('Got midi in:',end='')

    if msg[0:3] != [0xF0,0x7D,0x11]:
        print("Got unexpected sysex")
        return

    pname = ''.join(chr(c) for c in msg[5:15])
    print(pname,end=':')

    print(' F0 7D 01',end='')

    for b in msg[3:]:
        print(' %02X' % b,end='')
    print()

# =====================================================================
# Return patch id (A01...G64) as patch number (0...447)
# --------------------------------------------------------------------- 
    
def patch_id_to_nr(id):
    bank = ord(id[0]) - ord('A')
    nr_in_bank = int(id[1:3])

    return 64*bank + nr_in_bank - 1

# =====================================================================
# Return patch number (0...447) as patch id (A01...G64)
# --------------------------------------------------------------------- 
    
def patch_nr_to_id(nr):
    bank = chr(int(nr/64) + ord('A'))
    nr = nr % 64 + 1
    nr_in_bank = ''.join('{:02d}'.format(nr))

    return bank + nr_in_bank

# =====================================================================
# Main program
# --------------------------------------------------------------------- 

if open_midi_ports() == FAIL:
    print('Midi ports open fail')
    quit()

midi_in.set_callback(midi_in_callback)
midi_in.ignore_types(sysex=False)

first_patch_nr = patch_id_to_nr(first_patch)
last_patch_nr = patch_id_to_nr(last_patch)

print('first:',first_patch_nr,'    last:',last_patch_nr)

for pn in range(first_patch_nr,last_patch_nr+1):
    get_patch_from_drx(pn)

del midiout
del midi_in


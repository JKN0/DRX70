# =====================================================================
#
# Parse DX21 sysex files structures and convert to DRX-70
#
# Print out contents as human readable/SysEx

import time
import rtmidi

filename = 'DX21_4.SYX'
pn = -1
bank_offs = 224

# A01 = 0
# A33 = 32
# B01 = 64
# B33 = 96
# C01 = 128
# C33 = 160
# D01 = 192
# D33 = 224
# E01 = 256
# E33 = 288
# F01 = 320
# F33 = 352
# G01 = 384
# G33 = 416

swap_always = False

opmap = [4,2,3,1]
opmap4 = [4,3,2,1]
factormap = [
0,0|16,1|16,1|16,1,1|16,2|16,2|16,2,3|16,3,3|16,3|16,4,4|16,5|16,5,5|16,6|16,6,6|16,7|16,7,7|16,8|16,8,8|16,
9|16,9,9|16,10|16,10,10|16,10|16,11|16,11|16,11,11|16,11|16,11|16,11|16,12|16,12|16,12|16,12|16,12,13|16,13|16,
13|16,14|16,14|16,14|16,15|16,15|16,15|16,15|16,15|16,15|16,15|16,15|16,15|16,15|16,15|16,15|16
]

drx_port = 0

# =====================================================================
# Read sysex file. Return binary data as bytes.
# --------------------------------------------------------------------- 

def read_tx_sysex_file(fname):
    bindata = b''

    file = open(fname,'rb')
    
    for line in file:
        bindata += line

    file.close()

    return bindata

# =====================================================================
# Print prompt as text and hdata as hexdump
# --------------------------------------------------------------------- 
    
def printhex(prompt,hdata):
    print(prompt,end=' ')
    print(' '.join('{:02X}'.format(c) for c in hdata))

# =====================================================================
# --------------------------------------------------------------------- 
    
def get_pointer(data):
    ptr = int.from_bytes(data[0:2], byteorder='little')
    return ptr & 0x1FFF

# =====================================================================
# --------------------------------------------------------------------- 
    
def parse_bank(bindata):
    if bindata[0:6] != b'\xF0\x43\x00\x04\x20\x00':
        print('Not valid bank')
        return

    if pn >= 0:
        rs = pn
        re = pn+1
    else:
        rs = 0
        re = 32
        
    for n in range(rs,re):
        psi = n*128 + 6
        tx = parse_patch(bindata[psi:psi+128])
        #print_patch_params(tx)
        #print_patch_name(n,tx)
        drx = convert_patch(tx)
        #print_converted_params(drx)
        print_sysex(n+bank_offs,drx)
        send_sysex(n+bank_offs,drx)

# =====================================================================
# --------------------------------------------------------------------- 
    
def parse_patch(bindata):
    #printhex(' patch:',bindata)

    tx = {}
    tx['OPS'] = []

    tx['NAME'] = bindata[57:67].decode('utf-8')
    tx['Algorithm'] = int(bindata[40] & 0x07)
    
    for i in range(4):
        op = {}
        idx =  10*i

        # in OPQ, alg 4 has ops 2 and 3 swapped!
        if swap_always:
            op['OPNR'] = opmap4[i]
        else:            
            if tx['Algorithm'] == 4:
                op['OPNR'] = opmap4[i]
            else:
                op['OPNR'] = opmap[i]
            
        op['EG attack'] = int(bindata[idx+0])
        op['EG decay'] = int(bindata[idx+1])
        op['EG sus level'] = int(bindata[idx+4])
        op['EG sus rate'] = int(bindata[idx+2])
        op['EG release'] = int(bindata[idx+3])
        op['Level scaling'] = int(bindata[idx+5])
        op['AM enable'] = int(bindata[idx+6] >> 6)
        op['EG bias sens'] = int((bindata[idx+6] >> 3) & 0x07)
        op['Key vel sens'] = int(bindata[idx+6] & 0x07)
        op['Level'] = int(bindata[idx+7])
        op['Freq'] = int(bindata[idx+8])
        op['Rate scaling'] = int((bindata[idx+9] >> 3) & 0x03)
        op['Detune'] = int(bindata[idx+9] & 0x07)

        idx = 2*i
        op['EG shift'] = int((bindata[idx+73] >> 4) & 0x03)
        op['Fixed freq'] = int((bindata[idx+73] >> 3) & 0x01)
        op['Fixed freq range'] = int(bindata[idx+73] & 0x07)
        op['Waveform'] = int((bindata[idx+74] >> 4) & 0x07)
        op['Freq fine'] = int(bindata[idx+74] & 0x0F)

        tx['OPS'].append(op)

    tx['Feedback'] = int((bindata[40] >> 3) & 0x07)
    tx['LFO sync'] = int(bindata[40] >> 6)
    tx['LFO speed'] = int(bindata[41])
    tx['LFO delay'] = int(bindata[42])
    tx['LFO waveform'] = int(bindata[45] & 0x03)
    tx['FM depth'] = int(bindata[43])
    tx['FM sens'] = int(bindata[45] >> 4)
    tx['AM depth'] = int(bindata[44])
    tx['AM sens'] = int((bindata[45] >> 2) & 0x03)
    tx['Transpose'] = int(bindata[46])
    tx['Pitch bend range'] = int(bindata[47])
    tx['Mod wheel pitch'] = int(bindata[51])
    tx['Mod wheel ampl'] = int(bindata[52])
    tx['Chorus on'] = int((bindata[48] >> 4) & 0x01)
    tx['Poly mode'] = int((bindata[48] >> 3) & 0x01)
    tx['Sustain on'] = int((bindata[48] >> 2) & 0x01)
    tx['Portamento on'] = int((bindata[48] >> 1) & 0x01)
    tx['Portamento mode'] = int(bindata[48] & 0x01)
    tx['Portamento time'] = int(bindata[49])
    tx['Foot contr vol'] = int(bindata[50])
    tx['Foot contr pitch'] = int(bindata[82])
    tx['Foot contr ampl'] = int(bindata[83])
    tx['Breath contr pitch'] = int(bindata[53])
    tx['Breath contr ampl'] = int(bindata[54])
    tx['Breath contr p bias'] = int(bindata[55])
    tx['Breath contr EG bias'] = int(bindata[56])
    tx['Reverb rate'] = int(bindata[81])

    return tx
    
# =====================================================================
# --------------------------------------------------------------------- 
    
def print_patch_params(tx):

    print('== Patch',tx['NAME'],'====================================')
    
    for par_name,par_value in tx.items():
        if par_name != 'OPS' and par_name != 'NAME':
            print('%-20s: %s' % (par_name,par_value))

    ops = tx['OPS']

    for opnr in range(1,5):
        for i in range(4):
            if ops[i]['OPNR'] == opnr:
                print('-- Op',opnr,'-------------------------')
                for par_name,par_value in ops[i].items():
                    if par_name != 'OPNR':
                        print('%-20s: %s' % (par_name,par_value))


# =====================================================================
# --------------------------------------------------------------------- 
    
def print_converted_params(drx):

    print()
    print('===== Converted parameters =====')
    print('Name:',drx['name'])
    print('  algorithm:   ',drx['alg'])
    print('  feedback:    ',drx['feedb'])
    print('  echo:        ',drx['echo'])
    print('  lfo_control: ',drx['lfo_control'])
    print('  lfo_fm_delay:',drx['lfo_fm_delay'])
    print('  lfo_fm_rate: ',drx['lfo_fm_rate'])
    print('  lfo_fm_depth:',drx['lfo_fm_depth'])
    print('  lfo_am_depth:',drx['lfo_am_depth'])
    print()
    print('               _4___3___2___1_')
    print_op_param('lfo_am_en: ',drx['lfo_am_en'])
    print_op_param('factor:    ',drx['factor'])
    print_op_param('waveform:  ',drx['waveform'])
    print_op_param('vsl_sel:   ',drx['vsl_sel'])
    print_op_param('kse:       ',drx['kse'])
    print_op_param('ksl_hi_sel:',drx['ksl_hi_sel'])
    print_op_param('ksl_lo_sel:',drx['ksl_lo_sel'])
    print_op_detune('detune:    ',drx['detune'])
    print_op_param('attack:    ',drx['attack'])
    print_op_param('decay:     ',drx['decay'])
    print_op_param('sustain:   ',drx['sustain'])
    print_op_param('sus_level: ',drx['sus_level'])
    print_op_param('release:   ',drx['release'])
    print_op_param('level:     ',drx['level'])    

# =====================================================================
# --------------------------------------------------------------------- 
    
def print_patch_name(nr,tx):
    print(nr,tx['NAME'])
    
# =====================================================================
# --------------------------------------------------------------------- 
    
def convert_patch(tx):

    drx = {}

    drx['name'] = tx['NAME']
    drx['alg'] = tx['Algorithm']
    if tx['Algorithm'] == 7:
        print('Warning: algorithm 8 differs')
    drx['feedb'] = tx['Feedback']
    drx['outchan'] = 2
    drx['echo'] = scale_revrate(tx['Reverb rate'])

    drx['lfo_control'] = scale99(tx['LFO speed'],3)
    drx['lfo_fm_delay'] = scale99(tx['LFO delay'],3)
    drx['lfo_fm_rate'] = 0
    drx['lfo_fm_depth'] = scale99(tx['FM depth'],3)
    drx['lfo_am_depth'] = scale99(tx['AM depth'],2)

    drx['op_en'] = [0] * 4
    drx['lfo_am_en'] = [0] * 4
    drx['factor'] = [0] * 4
    drx['waveform'] = [0] * 4
    drx['vsl_sel'] = [0] * 4
    drx['kse'] = [0] * 4
    drx['ksl_hi_sel'] = [0] * 4
    drx['ksl_lo_sel'] = [0] * 4
    drx['detune'] = [0] * 4
    drx['attack'] = [0] * 4
    drx['decay'] = [0] * 4
    drx['sustain'] = [0] * 4
    drx['sus_level'] = [0] * 4
    drx['release'] = [0] * 4
    drx['level'] = [0] * 4
    
    ops = tx['OPS']
    for i in range(4):
        opidx = 4 - ops[i]['OPNR']
        op = ops[i]

        drx['op_en'][opidx] = 1
        drx['lfo_am_en'][opidx] = op['AM enable']
        
        drx['factor'][opidx] = convert_factor(op['Freq'])
        drx['waveform'][opidx] = convert_waveform(op['Waveform'])
        drx['vsl_sel'][opidx] = 0  # op['Key vel sens'] 
        drx['kse'][opidx] = op['Rate scaling']
        drx['ksl_hi_sel'][opidx] = 0   # op['Level scaling']
        drx['ksl_lo_sel'][opidx] = 0   # op['Level scaling']
        
        drx['detune'][opidx] = scale_detune(op['Detune'])
        drx['attack'][opidx] = op['EG attack']
        drx['decay'][opidx] = op['EG decay']
        drx['sustain'][opidx] = op['EG sus rate']
        drx['sus_level'][opidx] = op['EG sus level']
        drx['release'][opidx] = op['EG release']

        drx['level'][opidx] = op['Level']

    return drx

# =====================================================================
# --------------------------------------------------------------------- 
    
def print_sysex(pnr,drx):

    name = drx['name']
    print(name,'--------------------')
    print('F0 7D 01',end=' ')
    print_patch_nr(pnr)
    print(' '.join('{:02X}'.format(ord(c)) for c in name),end=' ')
    print('00 01')

    print('%02X' % drx['alg'],    end=' ')
    print('%02X' % drx['feedb'],  end=' ')
    print('%02X' % drx['outchan'],end=' ')
    print('%02X' % drx['echo'],   end=' ')
    print_op_sysex_param(drx['op_en'])
    print()

    print('%02X' % drx['lfo_control'], end=' ')
    print('%02X' % drx['lfo_fm_delay'],end=' ')
    print('%02X' % drx['lfo_fm_rate'], end=' ')
    print('%02X' % drx['lfo_fm_depth'],end=' ')
    print('%02X' % drx['lfo_am_depth'],end=' ')
    print_op_sysex_param(drx['lfo_am_en'])
    print()

    print_op_sysex_param(drx['factor'])
    print_op_sysex_param(drx['waveform'])
    print_op_sysex_param(drx['vsl_sel'])
    print_op_sysex_param(drx['kse'])
    print_op_sysex_param(drx['ksl_hi_sel'])
    print_op_sysex_param(drx['ksl_lo_sel'])
    print()
   
    print_op_sysex_param(drx['level'])
    print_op_sysex_param(drx['detune'])
    print_op_sysex_param(drx['attack'])
    print_op_sysex_param(drx['decay'])
    print_op_sysex_param(drx['sustain'])
    print_op_sysex_param(drx['sus_level'])
    print_op_sysex_param(drx['release'])
    print('F7')

# =====================================================================
# --------------------------------------------------------------------- 
    
def send_sysex(pnr,drx):

    print('Sending sysex...',end='')

    sysex = [0xF0,0x7D,0x01]    # SOX, manuf, message type

    sysex.append(pnr >> 4)      # patch nr, encoded 11-bits
    sysex.append(pnr & 0x0F)    

    for c in drx['name']:       # name
        sysex.append(ord(c))
    sysex.append(0x00)

    sysex.append(0x01)          # SCMD_NEW_PATCH

    # --- Patch data
    sysex.append(drx['alg'])
    sysex.append(drx['feedb'])
    sysex.append(drx['outchan'])
    sysex.append(drx['echo'])
    append_op_params(sysex,drx['op_en'])

    sysex.append(drx['lfo_control'])
    sysex.append(drx['lfo_fm_delay'])
    sysex.append(drx['lfo_fm_rate'])
    sysex.append(drx['lfo_fm_depth'])
    sysex.append(drx['lfo_am_depth'])
    append_op_params(sysex,drx['lfo_am_en'])
    
    append_op_params(sysex,drx['factor'])
    append_op_params(sysex,drx['waveform'])
    append_op_params(sysex,drx['vsl_sel'])
    append_op_params(sysex,drx['kse'])
    append_op_params(sysex,drx['ksl_hi_sel'])
    append_op_params(sysex,drx['ksl_lo_sel'])
    
    append_op_params(sysex,drx['level'])
    append_op_params(sysex,drx['detune'])
    append_op_params(sysex,drx['attack'])
    append_op_params(sysex,drx['decay'])
    append_op_params(sysex,drx['sustain'])
    append_op_params(sysex,drx['sus_level'])
    append_op_params(sysex,drx['release'])

    sysex.append(0xF7)          # EOX
    
    #print(sysex)

    midiout.send_message(sysex)
    time.sleep(0.2)

    print('sent')
    
# =====================================================================
# --------------------------------------------------------------------- 
    
def scale99(txval,bits):
    uplim = 2**bits
    drxval = int((txval/99)*uplim)

    return drxval
    
# =====================================================================
# --------------------------------------------------------------------- 
    
def convert_factor(txf):
    drxf = factormap[txf]
    drxfc = drxf & 0x0F

    if drxf >= 16:
        print('Warning: factor conversion %d->%d not exact' % (txf,drxfc))

    return drxfc
    
# =====================================================================
# --------------------------------------------------------------------- 
    
def convert_waveform(txw):
    if txw == 0:
        return 0
    elif txw == 2:
        return 1
    else:
        print('Warning: cannot convert waveform %d, using sine' % txw)
        return 0

# =====================================================================
# --------------------------------------------------------------------- 
    
def scale_detune(txval):
    return 2*txval + 26

# =====================================================================
# --------------------------------------------------------------------- 
    
def scale_revrate(txval):
    if txval == 0:
        return 0
    elif txval < 5:
        return 1
    else:
        return 0

# =====================================================================
# --------------------------------------------------------------------- 
    
def print_op_sysex_param(par_tbl):
    for par in par_tbl:
        print('%02X' % par,end=' ')
  
# =====================================================================
# --------------------------------------------------------------------- 
    
def append_op_params(sysex,par_tbl):
    for par in par_tbl:
        sysex.append(par)
  
# =====================================================================
# --------------------------------------------------------------------- 
    
def print_op_detune(par_name,par_tbl):
    print(' ',par_name,end=' ')
    for par in par_tbl:
        print('%3d' % (int(par)-32),end=' ')
    print()
  
# =====================================================================
# --------------------------------------------------------------------- 
    
def print_op_param(par_name,par_tbl):
    print(' ',par_name,end=' ')
    for par in par_tbl:
        print('%3d' % par,end=' ')
    print()
          
# =====================================================================
# --------------------------------------------------------------------- 
    
def print_patch_nr(pnr):
    pn_hi = pnr >> 4
    pn_lo = pnr & 0x0F
    print('%02X %02X' % (pn_hi,pn_lo),end=' ')

# =====================================================================
# Main program
# --------------------------------------------------------------------- 

#Create midi out object
midiout = rtmidi.MidiOut()
available_ports = midiout.get_ports()

if available_ports:
    print("Available MIDI out ports:")
    for portnr, name in enumerate(available_ports):
        print("[%i] %s" % (portnr, name))
        #if name.startswith('DRX-70'):
        if name.startswith('loopMIDI'):
            drx_port = portnr
    print('')
else:
    print("No available out ports")

print("DRX-port:",drx_port)
print()

midiout.open_port(drx_port)

# Read input files
bindata = read_tx_sysex_file(filename)
#printhex('bindata:',bindata)
parse_bank(bindata)

##for name in syx_names:
##    print(name,'--------------')
##    bindata = read_tx_sysex_file(name)
##    parse_bank(bindata)

del midiout


# DRX70 - FM synthesizer module

DRX70 is an FM midi synthesizer module with 8 voice polyphony and 4 operators per voice. It includes also a sample based drum machine and built-in chorus effect. Sounds can be edited using local display and buttons, or via midi sysex. It has both traditional midi and USB midi interfaces. 

Synthesizer is modified from a Yamaha PSR-70 entry-level home keyboard from 1985, with small number of fixed sound presets and auto accompaniment. 

Modifications done:
- Keyboard has been modified to midi sound module by removing the main circuit boards and installing them to a new case
- New firmware for Yamaha boards has been written from scratch (the Z80 part)
- Another CPU has been added to handle new user interface and USB (the 8051 part)
- Sounds have been converted from similar synths, like DX100

This is a continuation project for https://github.com/JKN0/PSR70-reverse

Files:

- Boards: 
  - CPUinterface: adapter board for 8051 breakout
  - Pcomadapter: adapter board for communication between the two CPUs

- DRX_synth: synthesizer firmware for the Z80 part 

- DRX_ui: UI firmware for the 8051 part 

- Soundbanks: current soundbanks and conversion scripts


More details of the project: https://hackaday.io/project/207035-drx70-fm-synthesizer-module

Short sound demo: https://www.youtube.com/watch?v=fnx1b2eMlik


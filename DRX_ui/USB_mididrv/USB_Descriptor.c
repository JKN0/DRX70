//-----------------------------------------------------------------------------
// USB_Descriptor.c
//-----------------------------------------------------------------------------
// Copyright 2005 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// Source file for USB firmware. Includes descriptor data.
//
//
// How To Test:     See Readme.txt
//
//
// FID:             32X000021
// Target:          C8051F32x
// Tool chain:      Keil C51 7.50 / Keil EVAL C51
//                  Silicon Laboratories IDE version 2.6
// Command Line:    See Readme.txt
// Project Name:    F32x_USB_Interrupt
//
//
// Release 1.3
//    -All changes by GP
//    -22 NOV 2005
//    -Changed revision number to match project revision
//      No content changes to this file
//    -Modified file to fit new formatting guidelines
//    -Changed file name from USB_DESCRIPTOR.c
//
// Release 1.0
//    -Initial Revision (DM)
//    -22 NOV 2002
//
// --- Modified for midi by JK 3.4.2021

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "USB_Types.h"
#include "USB_Descriptor.h"

//-----------------------------------------------------------------------------
// Macro for two byte field
//-----------------------------------------------------------------------------

#if defined BIG_ENDIAN
#define LE(x)   ((((x)&0x00FF)<<8)|(((x)&0xFF00)>>8))   // convert to little endian
#else
#define LE(x)   (x)                                     // no conversion
#endif

// Total size of class specific descriptors for Audio Control Interface
#define SIZE_OF_AC_DESCS    (sizeof(Tif_CS_AudioControl_descriptor))

// Total size of class specific descriptors for Midistreaming Interface
#define SIZE_OF_MS_DESCS    (sizeof(Tif_CS_MidiStreaming_descriptor) + \
                                sizeof(Tif_in_jack_descriptor) + \
                                sizeof(Tif_in_jack_descriptor) + \
                                sizeof(Tif_out_jack_descriptor) + \
                                sizeof(Tif_out_jack_descriptor) + \
                                sizeof(Tendpoint_descriptor) + \
                                    sizeof(Tep_MS_bulk_descriptor) + \
                                sizeof(Tendpoint_descriptor) + \
                                    sizeof(Tep_MS_bulk_descriptor))

                                    
//-----------------------------------------------------------------------------
// Descriptor Declarations
//-----------------------------------------------------------------------------

// Descriptors are according to USB Midi 1.0 and taken almost directly from 
//   https://www.usb.org/document-library/usb-midi-devices-10

// Device descriptor
Tdevice_descriptor __code DeviceDesc =
{
    sizeof(Tdevice_descriptor), // bLength
    DSC_TYPE_DEVICE,    // bDescriptorType
    LE( VER_USB ),      // bcdUSB
    0x00,               // bDeviceClass (specified in interface descriptor)
    0x00,               // bDeviceSubClass (unused)
    0x00,               // bDeviceProtocol (unused)
    EP0_PACKET_SIZE,    // bMaxPacketSize: 64 instead of 8
    LE( VID ),          // idVendor
    LE( PID ),          // idProduct
    LE( DEV_REV ),      // bcdDevice
    0x01,               // iManufacturer
    0x02,               // iProduct
    0x03,               // iSerialNumber
    0x01                // bNumConfigurations
}; //end of DeviceDesc

// Configuration
Tconfiguration_desc_set __code ConfigDescSet =
{
  // Configuration descriptor
  {
    sizeof(Tconfiguration_descriptor),  // bLength
    DSC_TYPE_CONFIG,    // bDescriptorType
    LE( sizeof(Tconfiguration_desc_set) ),// bTotalLength
    DSC_NUM_INTERFACE,  // bNumInterfaces (2)
    0x01,               // bConfigurationValue
    0x00,               // iConfiguration: no config string
    0x80,               // bmAttributes: b7:must be 1, b6=0: bus powered, b5=0: no wake-up support
    0x32                // bMaxPower: 100 mA
  },
  
  // --- Audio Control Interface
  // Standard Audio Control Interface Descriptor
  {
    sizeof(Tinterface_descriptor),  // bLength
    DSC_TYPE_INTERFACE, // bDescriptorType
    0x00,               // bInterfaceNumber: 0
    0x00,               // bAlternateSetting: this is the default setting 
    0x00,               // bNumEndpoints: 0 (only EP0 exists) 
    0x01,               // bInterfaceClass: AUDIO
    0x01,               // bInterfaceSubClass: AUDIO_CONTROL
    0x00,               // bInterfaceProcotol: unused
    0x00                // iInterface: no string
  },
  // Class-specific Audio Control Interface Descriptor
  { 
    sizeof(Tif_CS_AudioControl_descriptor),    // bLength
    DSC_TYPE_CS_INTERFACE,  // bDescriptorType: CS_INTERFACE
    0x01,               // bDescriptorSubtype: HEADER
    LE(0x0100),         // bcdADC: Revision of class specification = 1.0
    LE(SIZE_OF_AC_DESCS),  // wTotalLength: Total size of class specific descriptors
    0x01,               // bInCollection Number of streaming interfaces: 1 
    0x01                // baInterfaceNr: MIDIStreaming interface 1 belongs to this AudioControl interface
  },
  
  // --- MIDIStreaming Interface
  // Standard MIDIStreaming Interface Descriptor   
  {
    sizeof(Tinterface_descriptor),  // bLength
    DSC_TYPE_INTERFACE, // bDescriptorType
    0x01,               // bInterfaceNumber: 1
    0x00,               // bAlternateSetting: this is the default setting 
    0x02,               // bNumEndpoints: 2
    0x01,               // bInterfaceClass: AUDIO
    0x03,               // bInterfaceSubClass: MIDISTREAMING
    0x00,               // bInterfaceProcotol: unused
    0x00                // iInterface: no string
  },
  // Class-specific MIDIStreaming Interface Descriptor
  { 
    sizeof(Tif_CS_MidiStreaming_descriptor),    // bLength
    DSC_TYPE_CS_INTERFACE,      // bDescriptorType: CS_INTERFACE
    0x01,               // bDescriptorSubtype: MS_HEADER
    LE(0x0100),         // bcdADC: Revision of class specification = 1.0
    LE(SIZE_OF_MS_DESCS) // wTotalLength: Total size of class specific descriptors
  },

  // --- MIDI Jacks
  {
    sizeof(Tif_in_jack_descriptor),  // bLength
    DSC_TYPE_CS_INTERFACE,      // bDescriptorType: CS_INTERFACE
    MIDI_IN_JACK,               // bDescriptorSubtype: MIDI_IN_JACK
    MIDI_JACK_EMBEDDED,         // bJackType: EMBEDDED    
    0x01,                       // bJackID: 1
    0x00                        // iJack: unused
  },
  {
    sizeof(Tif_in_jack_descriptor),  // bLength
    DSC_TYPE_CS_INTERFACE,      // bDescriptorType: CS_INTERFACE
    MIDI_IN_JACK,               // bDescriptorSubtype: MIDI_IN_JACK
    MIDI_JACK_EXTERNAL,         // bJackType: EXTERNAL    
    0x02,                       // bJackID: 2
    0x00                        // iJack: unused
  },
  {
    sizeof(Tif_out_jack_descriptor),  // bLength
    DSC_TYPE_CS_INTERFACE,      // bDescriptorType: CS_INTERFACE
    MIDI_OUT_JACK,              // bDescriptorSubtype: MIDI_OUT_JACK
    MIDI_JACK_EMBEDDED,         // bJackType: EMBEDDED    
    0x03,                       // bJackID: 3
    0x01,                       // bNrInputPins: 1 
    0x02,                       // BaSourceID: ID of the Entity to which this Pin is connected
    0x01,                       // BaSourcePin: Output Pin number of the Entity to which this Input Pin is connected
    0x00                        // iJack: unused
  },
  {
    sizeof(Tif_out_jack_descriptor),  // bLength
    DSC_TYPE_CS_INTERFACE,      // bDescriptorType: CS_INTERFACE
    MIDI_OUT_JACK,              // bDescriptorSubtype: MIDI_OUT_JACK
    MIDI_JACK_EXTERNAL,         // bJackType: EXTERNAL    
    0x04,                       // bJackID: 4
    0x01,                       // bNrInputPins: 1 
    0x01,                       // BaSourceID: ID of the Entity to which this Pin is connected
    0x01,                       // BaSourcePin: Output Pin number of the Entity to which this Input Pin is connected
    0x00                        // iJack: unused
  },

  // --- Endpoints
  // Standard Bulk OUT Endpoint Descriptor    
  {
    sizeof(Tendpoint_descriptor),   // bLength
    DSC_TYPE_ENDPOINT,          // bDescriptorType
    OUT_EP2,                    // bEndpointAddress
    DSC_EP_BULK,                // bmAttributes: bulk
    LE(EP2_PACKET_SIZE),        // MaxPacketSize
    0x00,                       // bInterval: 0 ignored
    0x00,                       // bRefresh: unused
    0x00                        // bSynchAddress: unused
  },
  // MIDIStreaming Bulk OUT Endpoint Descriptor
  {
    sizeof(Tep_MS_bulk_descriptor),  // bLength
    DSC_TYPE_CS_ENDPOINT,       // bDescriptorType: CS_ENDPOINT
    0x01,                       // bDescriptorSubtype: MS_GENERAL
    0x01,                       // bNumEmbMIDIJack: 1
    0x01                        // BaAssocJackID(1): ID of the first Embedded MIDI IN Jack
  },
  // Standard Bulk IN Endpoint Descriptor    
  {
    sizeof(Tendpoint_descriptor),   // bLength
    DSC_TYPE_ENDPOINT,          // bDescriptorType
    IN_EP2,                     // bEndpointAddress
    DSC_EP_BULK,                // bmAttributes: bulk
    LE(EP2_PACKET_SIZE),        // MaxPacketSize
    0x00,                       // bInterval: 0 ignored
    0x00,                       // bRefresh: unused
    0x00                        // bSynchAddress: unused
  },
  // MIDIStreaming Bulk IN Endpoint Descriptor
  {
    sizeof(Tep_MS_bulk_descriptor),  // bLength
    DSC_TYPE_CS_ENDPOINT,       // bDescriptorType: CS_ENDPOINT
    0x01,                       // bDescriptorSubtype: MS_GENERAL
    0x01,                       // bNumEmbMIDIJack: 1
    0x03                        // BaAssocJackID(1): ID of the first Embedded MIDI IN Jack
  }
}; //end of Configuration

#define STR0LEN 4

BYTE __code String0Desc[STR0LEN] =
{
    STR0LEN, DSC_TYPE_STRING, 0x09, 0x04
}; //end of String0Desc

#define STR1LEN sizeof("SILICON LABORATORIES INC.")*2

BYTE __code String1Desc[STR1LEN] =
{
    STR1LEN, DSC_TYPE_STRING,
    'S', 0,
    'I', 0,
    'L', 0,
    'I', 0,
    'C', 0,
    'O', 0,
    'N', 0,
    ' ', 0,
    'L', 0,
    'A', 0,
    'B', 0,
    'O', 0,
    'R', 0,
    'A', 0,
    'T', 0,
    'O', 0,
    'R', 0,
    'I', 0,
    'E', 0,
    'S', 0,
    ' ', 0,
    'I', 0,
    'N', 0,
    'C', 0,
    '.', 0
}; //end of String1Desc

#define STR2LEN sizeof("DRX-70")*2

BYTE __code String2Desc[STR2LEN] =
{
    STR2LEN, DSC_TYPE_STRING,
    'D', 0,
    'R', 0,
    'X', 0,
    '-', 0,
    '7', 0,
    '0', 0
}; //end of String2Desc

// serial number string

#define STR3LEN sizeof("0001")*2

BYTE __code String3Desc[STR3LEN] =
{
    STR3LEN, DSC_TYPE_STRING,
    '0', 0,
    '0', 0,
    '0', 0,
    '1', 0
}; //end of String3Desc

BYTE __code * __code StringDescTable[ DSC_NUM_STRING ] =
{
    String0Desc,
    String1Desc,
    String2Desc,
    String3Desc
};

//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------

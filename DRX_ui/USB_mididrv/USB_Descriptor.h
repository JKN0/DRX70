//-----------------------------------------------------------------------------
// USB_Descriptor.h
//-----------------------------------------------------------------------------
// Copyright 2005 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// Header file for USB firmware. Defines standard descriptor structures.
//
//
// How To Test:     See Readme.txt
//
//
// FID:             32X000022
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
//    -Changed file name from USB_DESCRIPTOR.h
//
//
// Release 1.0
//    -Initial Revision (DM)
//    -22 NOV 2002
//
// --- Modified for midi by JK 3.4.2021
//

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

//-----------------------------------------------------------------------------
// Definition of descriptors
//-----------------------------------------------------------------------------

#define VER_USB                 0x0200  // USB specification revision
#define VID                     0x1209  // Vendor ID = Generic
#define PID                     0x0001  // Product ID = Test PID
#define DEV_REV                 0x0000  // Device Release number

#define DSC_NUM_INTERFACE       2       // Number of Interfaces
#define DSC_NUM_STRING          4       // Number of String desc

                                        // Define Endpoint Packet Sizes
#define EP0_PACKET_SIZE         0x40    // Endpoint 0

                                        // Endpoint 1-3
                                        //  For full-speed,
                                        //      bulk:       8, 16, 32, 64
                                        //      interrupt:    0 - 64
                                        //      isoc:       0 - 1023 (512 for 'F32x/34x)
#define EP1_PACKET_SIZE         0x0010
#define EP2_PACKET_SIZE         0x0040


//------------------------------------------
// Standard Device Descriptor Type Defintion
//------------------------------------------
typedef struct {
   BYTE bLength;                       // Size of this Descriptor in Bytes
   BYTE bDescriptorType;               // Descriptor Type (=1)
   UINT bcdUSB;                        // USB Spec Release Number in BCD
   BYTE bDeviceClass;                  // Device Class Code
   BYTE bDeviceSubClass;               // Device Subclass Code
   BYTE bDeviceProtocol;               // Device Protocol Code
   BYTE bMaxPacketSize0;               // Maximum Packet Size for EP0
   UINT idVendor;                      // Vendor ID
   UINT idProduct;                     // Product ID
   UINT bcdDevice;                     // Device Release Number in BCD
   BYTE iManufacturer;                 // Index of String Desc for Manufacturer
   BYTE iProduct;                      // Index of String Desc for Product
   BYTE iSerialNumber;                 // Index of String Desc for SerNo
   BYTE bNumConfigurations;            // Number of possible Configurations
} Tdevice_descriptor;                   // End of Device Descriptor Type

//--------------------------------------------------
// Standard Configuration Descriptor Type Definition
//--------------------------------------------------
typedef struct {
   BYTE bLength;                       // Size of this Descriptor in Bytes
   BYTE bDescriptorType;               // Descriptor Type (=2)
   UINT wTotalLength;                  // Total Length of Data for this Conf
   BYTE bNumInterfaces;                // # of Interfaces supported by Conf
   BYTE bConfigurationValue;           // Designator Value for *this* Conf
   BYTE iConfiguration;                // Index of String Desc for this Conf
   BYTE bmAttributes;                  // Configuration Characteristics
   BYTE bMaxPower;                     // Max. Power Consumption in Conf (*2mA)
} Tconfiguration_descriptor;            // End of Configuration Descriptor Type

//----------------------------------------------
// Standard Interface Descriptor Type Definition
//----------------------------------------------
typedef struct {
   BYTE bLength;                       // Size of this Descriptor in Bytes
   BYTE bDescriptorType;               // Descriptor Type (=4)
   BYTE bInterfaceNumber;              // Number of *this* Interface (0..)
   BYTE bAlternateSetting;             // Alternative for this Interface
   BYTE bNumEndpoints;                 // No of EPs used by this IF (excl. EP0)
   BYTE bInterfaceClass;               // Interface Class Code
   BYTE bInterfaceSubClass;            // Interface Subclass Code
   BYTE bInterfaceProtocol;            // Interface Protocol Code
   BYTE iInterface;                    // Index of String Desc for Interface
} Tinterface_descriptor;                // End of Interface Descriptor Type

//---------------------------------------------
// Standard Endpoint Descriptor Type Definition
//---------------------------------------------
typedef struct {
   BYTE bLength;                       // Size of this Descriptor in Bytes
   BYTE bDescriptorType;               // Descriptor Type (=5)
   BYTE bEndpointAddress;              // Endpoint Address (Number + Direction)
   BYTE bmAttributes;                  // Endpoint Attributes (Transfer Type)
   UINT wMaxPacketSize;                // Max. Endpoint Packet Size
   BYTE bInterval;                     // Polling Interval (Interrupt) ms
   BYTE bRefresh;                      // added for midi bulk out endpoint
   BYTE bSynchAddress;                 // added for midi bulk out endpoint
} Tendpoint_descriptor;                 // End of Endpoint Descriptor Type

//---------------------------------------------
// Class specific descriptors
//---------------------------------------------

//---------------------------------------------
// Class-specific Audio Control Interface Descriptor
//---------------------------------------------
typedef struct {
   BYTE bLength;
   BYTE bDescriptorType;
   BYTE bDescriptorSubtype;
   UINT bcdADC;
   UINT wTotalLength;
   BYTE bInCollection;
   BYTE baInterfaceNr;
} Tif_CS_AudioControl_descriptor;

//---------------------------------------------
// Class-specific MIDIStreaming Interface Descriptor
//---------------------------------------------
typedef struct {
    BYTE bLength;
    BYTE bDescriptorType;
    BYTE bDescriptorSubtype;
    UINT bcdADC;
    UINT wTotalLength;
} Tif_CS_MidiStreaming_descriptor;

//---------------------------------------------
// MIDI IN Jack Descriptor
//---------------------------------------------
typedef struct {
    BYTE bLength;
    BYTE bDescriptorType;
    BYTE bDescriptorSubtype;
    BYTE bJackType;
    BYTE bJackID;
    BYTE iJack;
} Tif_in_jack_descriptor;

//---------------------------------------------
// MIDI OUT Jack Descriptor
//---------------------------------------------
typedef struct {
    BYTE bLength;
    BYTE bDescriptorType;
    BYTE bDescriptorSubtype;
    BYTE bJackType;
    BYTE bJackID;
    BYTE bNrInputPins;
    BYTE BaSourceID;
    BYTE BaSourcePin;
    BYTE iJack;
} Tif_out_jack_descriptor;

//---------------------------------------------
// MIDIStreaming Bulk Endpoint Descriptor
//---------------------------------------------
typedef struct {
    BYTE bLength;
    BYTE bDescriptorType;
    BYTE bDescriptorSubtype;
    BYTE bNumEmbMIDIJack;
    BYTE BaAssocJackID; 
} Tep_MS_bulk_descriptor;

//---------------------------------------------
// Configuration Descriptor Set
//---------------------------------------------
typedef struct {
    Tconfiguration_descriptor                   m_config_desc;          
        Tinterface_descriptor                   m_if_ac_desc;
            Tif_CS_AudioControl_descriptor      m_if_cs_ac_desc;
        Tinterface_descriptor                   m_if_ms_desc;
            Tif_CS_MidiStreaming_descriptor     m_if_cs_ms_desc;
                Tif_in_jack_descriptor          m_if_ij_desc_1;
                Tif_in_jack_descriptor          m_if_ij_desc_2;
                Tif_out_jack_descriptor         m_if_oj_desc_3;
                Tif_out_jack_descriptor         m_if_oj_desc_4;
                Tendpoint_descriptor            m_ep_desc_OUT2;
                    Tep_MS_bulk_descriptor      m_ep_ms_desc_OUT2;
                Tendpoint_descriptor            m_ep_desc_IN2;
                    Tep_MS_bulk_descriptor      m_ep_ms_desc_IN2;
} Tconfiguration_desc_set;


// Descriptor type
#define DSC_TYPE_DEVICE         0x01
#define DSC_TYPE_CONFIG         0x02
#define DSC_TYPE_STRING         0x03
#define DSC_TYPE_INTERFACE      0x04
#define DSC_TYPE_ENDPOINT       0x05

// Endpoint transfer type
#define DSC_EP_CONTROL          0x00
#define DSC_EP_ISOC             0x01
#define DSC_EP_BULK             0x02
#define DSC_EP_INTERRUPT        0x03

// Endpoint address
#define IN_EP1                  0x81
#define OUT_EP1                 0x01
#define IN_EP2                  0x82
#define OUT_EP2                 0x02
#define IN_EP3                  0x83
#define OUT_EP3                 0x03

// Descriptor type (Class specific)
#define DSC_TYPE_CS_INTERFACE       0x24
#define DSC_SUBTYPE_CS_HEADER_FUNC  0x00
#define DSC_SUBTYPE_CS_CALL_MAN     0x01
#define DSC_SUBTYPE_CS_ABST_CNTRL   0x02
#define DSC_SUBTYPE_CS_UNION_FUNC   0x06
#define DSC_TYPE_CS_ENDPOINT        0x25

// Midi jack types
#define MIDI_IN_JACK                0x02
#define MIDI_OUT_JACK               0x03

#define MIDI_JACK_EMBEDDED          0x01
#define MIDI_JACK_EXTERNAL          0x02


#endif  // USB_DESCRIPTORS_H

//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------

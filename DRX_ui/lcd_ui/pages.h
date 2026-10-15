/***************************************************************************

 pages.h

 LCD/encoder UI driver for DRX-70. Display page definitions.

 16.5.2021  - First version

 ***************************************************************************/

#ifndef PAGES_H
#define PAGES_H

typedef struct fvalue_t FVALUE_T;

typedef void (*FHANDLER)( FVALUE_T *fval, BYTE oper, INT8 vdelta ) __reentrant;

typedef struct flabel_t {
    char *caption;
    BYTE r;
    BYTE c;
} FLABEL_T;

typedef struct fvalue_t {
    BYTE r;
    BYTE c;
    BYTE w;
    BYTE min;
    BYTE max;
    FHANDLER handler;
    void *target;
    BYTE scmd;
    char *clist[];
} FVALUE_T;

typedef struct field_t {
    FLABEL_T *label;
    FVALUE_T *value;
    BYTE op_inst;        // operator or instrument nr
} FIELD_T;

typedef struct page_t {
    char *caption;
    BYTE field_cnt;
    FIELD_T fields[];
} PAGE_T;

#endif  // PAGES_H

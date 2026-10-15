CC=sdcc
CFLAGS=-mz80 --opt-code-speed --max-allocs-per-node 9000
LFLAGS=-mz80 --code-loc 0x0110 --data-loc 0xE000 

MAIN=DRX_synth.c
SRC=synth.c drum.c parcom_slave.c uicom.c

SRC:=$(MAIN) $(SRC)
REL=$(SRC:.c=.rel)
IHX=$(MAIN:.c=.ihx)
HEX=$(IHX:%.ihx=_%.hex)

all: $(HEX)

%.rel: %.c
	$(CC) $(CFLAGS) -c $<

$(IHX): $(REL)
	$(CC) $(LFLAGS) $^

$(HEX): $(IHX)
	packihx $< > $@

clean:
	del $(HEX)
	del $(REL)

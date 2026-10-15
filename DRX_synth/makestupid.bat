sdcc -mz80 -c synth.c
sdcc -mz80 -c drum.c
sdcc -mz80 -c uicom.c
sdcc -mz80 -c parcom_slave.c
sdcc -mz80 --code-loc 0x0110 --data-loc 0xE000 DRX_synth.c synth.rel drum.rel uicom.rel parcom_slave.rel
packihx DRX_synth.ihx > _DRX_synth.hex

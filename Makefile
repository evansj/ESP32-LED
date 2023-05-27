#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := LED
SUFFIX := $(shell components/ESP32-RevK/buildsuffix)

all:
	@echo Make: build/$(PROJECT_NAME)$(SUFFIX).bin
	@touch $(IDF_PATH)/components/esp_app_format/esp_app_desc.c
	@idf.py build
	@cp --remove-destination build/$(PROJECT_NAME).bin $(PROJECT_NAME)$(SUFFIX).bin
	@echo Done: build/$(PROJECT_NAME)$(SUFFIX).bin

makering: makering.c
	gcc -I/usr/local/include -L/usr/local/lib -O -o $@ $< -lpopt -lm -g

issue:  set
	cp --remove-destination LED*.bin release

set:	solo wroom pico

pico:
	components/ESP32-RevK/setbuildsuffix -S1-PICO
	@make

wroom:
	components/ESP32-RevK/setbuildsuffix -S1
	@make

solo:
	components/ESP32-RevK/setbuildsuffix -S1-SOLO
	@make

flash:
	idf.py flash

monitor:
	idf.py monitor

clean:
	idf.py clean

menuconfig:
	idf.py menuconfig

pull:
	git pull
	git submodule update --recursive

update:
	git submodule update --init --recursive --remote
	git commit -a -m "Library update"

# Set GPIO low (whichever CBUS is set to mode 8/GPIO)
bootmode: ftdizap/ftdizap
	./ftdizap/ftdizap --cbus=0

# Flash with GPIO control using CBUS0 (FT230X design)
zap:    bootmode flash
	./ftdizap/ftdizap --cbus=1 --reset

# Program the FTDI
ftdi: ftdizap/ftdizap
	./ftdizap/ftdizap --serial="RevK" --description="LED" --cbus0-mode=7 --cbus1-mode=13 --invert-rts=1 --invert-dtr=1

ftdizap/ftdizap: ftdizap/ftdizap.c
	make -C ftdizap

PCBCase/case: PCBCase/case.c
	make -C PCBCase

%.stl: %.scad
	echo "Making $@"
	/Applications/OpenSCAD.app/Contents/MacOS/OpenSCAD $< -o $@
	echo "Made $@"

stl:    PCB/LED/LED.stl

PCB/LED/LED.scad: PCB/LED/LED.kicad_pcb PCBCase/case Makefile
	PCBCase/case -o $@ $< --edge=2 --base=5.6 --top=3


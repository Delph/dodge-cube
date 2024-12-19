DIR:=$(abspath $(lastword $(MAKEFILE_LIST)))
PROJECT_NAME:=$(notdir $(patsubst %/,%,$(dir $(DIR))))


MAIN_FILE:=$(PROJECT_NAME).ino
FQBN:=esp8266:esp8266:generic:eesz=1M128
ADDRESS:=192.168.0.10
ESPOTA:=~/.arduino15/packages/esp8266/hardware/esp8266/3.1.2/tools/espota.py
ESPTOOL:=~/.arduino15/packages/esp8266/hardware/esp8266/3.1.2/tools/esptool/esptool.py

DEFINES:=-DARDUINOJSON_SLOT_ID_SIZE=1 -DARDUINOJSON_USE_DOUBLE=0


compile:
	arduino-cli compile --fqbn $(FQBN) --build-path build --build-property build.extra_flags="$(DEFINES)"
	@cp build/compile_commands.json compile_commands.json


upload:
	arduino-cli upload --fqbn $(FQBN) --port $(ADDRESS) --input-dir build


filesystem:
	rm -rf web/build
	cd web && bun install && bun run build


core:
	-arduino-cli config init || true
	arduino-cli config add board_manager.additional_urls https://arduino.esp8266.com/stable/package_esp8266com_index.json
	arduino-cli core update-index
	arduino-cli core install esp8266:esp8266


LIBRARIES:=FastLED ArduinoJson
libraries: $(LIBRARIES)
	arduino-cli lib install $^


clean:
	@$(RM) -rf build compile_commands.json


.PHONY: compile upload port filesystem core clean $(LIBRARIES)

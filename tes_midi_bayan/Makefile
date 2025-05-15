#
#
#

BUILD_ID_FILE=BUILD_ID.hpp

PORT=/dev/ttyACM0

MAKEPRG=arduino-cli compile --no-color

MK_BUILD_ID=md5sum *h *cpp *ino|cksum|awk '{print("\#define BUILD_ID " $$1)}' >$(BUILD_ID_FILE)

all: *.cpp *.h *.ino
	$(MK_BUILD_ID)
	$(MAKEPRG)

upload:
	$(MK_BUILD_ID)
	$(MAKEPRG) -u -p $(PORT)


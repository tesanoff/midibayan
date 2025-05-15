#
#
#

BUILD_ID_FILE=BUILD_ID.hpp
ALL_SOURCE_FILES=*.cpp *.h *.ino
PORT=/dev/ttyACM0

MAKEPRG=arduino-cli compile --no-color

MK_BUILD_ID=md5sum *h *cpp *ino|cksum|awk '{print("\#define BUILD_ID " $$1)}' >$(BUILD_ID_FILE)

all: $(ALL_SOURCE_FILES) $(BUILD_ID_FILE)
	$(MAKEPRG)

$(BUILD_ID_FILE): $(ALL_SOURCE_FILES)
	$(MK_BUILD_ID)

upload: $(ALL_SOURCE_FILES) $(BUILD_ID_FILE)
	$(MAKEPRG) -u -p $(PORT)


TARGET=grs_iq_rx

ifndef BUILD_DIR
	BUILD_DIR=$(CURDIR)
endif

CC=gcc
FLAGS=-fpic -std=c99 -Wall -pedantic -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes
LIBS=-pthread -lm -lrtlsdr -lzmq

all: $(BUILD_DIR)/main.o
	$(CC) $(FLAGS) $(BUILD_DIR)/*.o -o $(BUILD_DIR)/$(TARGET) $(LIBS)

$(BUILD_DIR)/%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@ $(LIBS)

install:
	@echo "Installing GRS IQ Receiver application..."
	install -m 0755 $(BUILD_DIR)/$(TARGET) /usr/local/bin/$(TARGET)

uninstall:
	@echo "Uninstalling GRS IQ Receiver application..."
	rm /usr/local/bin/$(TARGET)

clean:
	rm $(BUILD_DIR)/*.o $(BUILD_DIR)/$(TARGET)

# Specify the build directory
BUILD_DIR := build

all: clean $(BUILD_DIR)/simulador $(BUILD_DIR)/monitor

$(BUILD_DIR)/simulador: $(BUILD_DIR)/simulador.o
	gcc -g -o $@ $^ -lpthread

$(BUILD_DIR)/monitor: $(BUILD_DIR)/monitor.o
	gcc -g -o $@ $^ -lpthread

$(BUILD_DIR)/%.o: %.c config.h | $(BUILD_DIR)
	gcc -c -g $< -o $@

$(BUILD_DIR):
	mkdir -p $@

clean:
	if [ -d "$(BUILD_DIR)" ]; then rm -r "$(BUILD_DIR)"; fi

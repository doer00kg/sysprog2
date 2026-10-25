# Хостові тести: збираються звичайним gcc, плата не потрібна.
#   make -C tests test       - зібрати й запустити
#   make -C tests coverage   - те саме + покриття Devices/ та App/
CC       ?= gcc
SRC_DEV  := ../firmware/Devices
SRC_APP  := ../firmware/App
CFLAGS   := -std=c11 -Wall -Wextra -Werror -O0 -g --coverage \
            -I$(SRC_DEV) -I$(SRC_APP) -Ifakes -I.
BUILD    := build

TESTS := $(BUILD)/test_mpu6050 $(BUILD)/test_motion

.PHONY: test coverage clean
test: $(TESTS)
	@set -e; for t in $(TESTS); do echo "== $$t"; ./$$t; done

$(BUILD)/test_mpu6050: test_mpu6050.c fakes/fake_i2c.c $(SRC_DEV)/mpu6050.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@ -lm

$(BUILD)/test_motion: test_motion.c $(SRC_APP)/motion.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@ -lm

$(BUILD):
	mkdir -p $@

coverage: clean test
	@echo "== Покриття рядків (шари Devices та App)"
	@for f in $(BUILD)/*.gcda; do gcov -n $$f 2>/dev/null; done | \
	  awk '/^File/ {name=$$2} /^Lines executed/ && name ~ /firmware\/(Devices|App)\// {print name, $$0}' | sort -u

clean:
	rm -rf $(BUILD)

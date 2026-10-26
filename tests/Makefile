# Хостові тести: збираються звичайним gcc, плата не потрібна.
#   make -C tests test       - зібрати й запустити
#   make -C tests coverage   - те саме + покриття Devices/ та App/
CC       ?= gcc
SRC_DEV  := ../firmware/Devices
SRC_APP  := ../firmware/App
SRC_PLAT := ../firmware/Platform
CFLAGS   := -std=c11 -Wall -Wextra -Werror -O0 -g --coverage \
            -I$(SRC_DEV) -I$(SRC_APP) -I$(SRC_PLAT) -Ifakes -I.
BUILD    := build

TESTS := $(BUILD)/test_mpu6050 $(BUILD)/test_motion $(BUILD)/test_debounce $(BUILD)/test_app

.PHONY: test coverage clean
test: $(TESTS)
	@set -e; for t in $(TESTS); do echo "== $$t"; ./$$t; done

$(BUILD)/test_mpu6050: test_mpu6050.c fakes/fake_i2c.c $(SRC_DEV)/mpu6050.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@ -lm

$(BUILD)/test_motion: test_motion.c $(SRC_APP)/motion.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@ -lm

$(BUILD)/test_debounce: test_debounce.c $(SRC_APP)/debounce.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@ -lm

$(BUILD)/test_app: test_app.c fakes/fake_i2c.c fakes/fake_platform.c \
                   $(SRC_APP)/app.c $(SRC_APP)/motion.c $(SRC_APP)/debounce.c $(SRC_DEV)/mpu6050.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@ -lm

$(BUILD):
	mkdir -p $@

coverage: clean test
	@echo "== Покриття рядків (шари Devices та App; максимум за всіма тестами)"
	@for f in $(BUILD)/*.gcda; do gcov -n $$f 2>/dev/null; done | \
	  awk '/^File/ {gsub(/\x27/, "", $$2); name=$$2} \
	       /^Lines executed/ && name ~ /firmware\/(Devices|App)\// { \
	         split($$2, a, ":"); pct=substr(a[2], 1, length(a[2])-1) + 0; \
	         if (pct > best[name]) best[name] = pct } \
	       END { for (n in best) printf "%s %.1f%%\n", n, best[n] }' | sort

clean:
	rm -rf $(BUILD)

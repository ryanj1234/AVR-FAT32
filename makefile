CC=gcc
TARGET=main

SRC_DIR=src
TEST_DIR=tests

TEST_TARGET=test

all: program rest test run_test

program: $(SRC_DIR)/main.c $(SRC_DIR)/f32.c $(SRC_DIR)/f32_print.c $(SRC_DIR)/f32_disk_mgmt.c $(SRC_DIR)/sd.c
	$(CC) $(SRC_DIR)/main.c $(SRC_DIR)/f32.c $(SRC_DIR)/f32_print.c $(SRC_DIR)/f32_disk_mgmt.c $(SRC_DIR)/sd.c -o $(TARGET)

test: $(TEST_DIR)/main.c $(TEST_DIR)/munit/munit.c $(SRC_DIR)/f32.c
	$(CC) -I$(SRC_DIR) $(TEST_DIR)/main.c $(SRC_DIR)/f32.c $(SRC_DIR)/f32_print.c $(SRC_DIR)/f32_disk_mgmt.c $(SRC_DIR)/sd.c $(TEST_DIR)/munit/munit.c -o $(TEST_TARGET)

rest: test_mmc.img
	cp base_mmc.img test_mmc.img

run_test: test
	./$(TEST_TARGET)

clean:
	rm -f $(TARGET) $(TEST_TARGET)

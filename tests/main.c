#include "munit/munit.h"
#include "f32.h"
#include <stdio.h>

static MunitResult
test_not_found_txt(const MunitParameter params[], void* data) {
    (void) params;
    (void) data;

    f32_sector sec;
    munit_assert(f32_mount(&sec) == 0);

    f32_file * fd = f32_open("asdf.txt", "r");
    munit_assert_null(fd);

    munit_assert(f32_close(fd) == 0);

    munit_assert(f32_umount() == 0);
    return MUNIT_OK;
}

static MunitResult
test_test_txt(const MunitParameter params[], void* data) {
    (void) params;
    (void) data;

    f32_sector sec;
    munit_assert(f32_mount(&sec) == 0);

    f32_file * fd = f32_open("asdf.txt", "r");
    munit_assert_null(fd);

    fd = f32_open("TEST.TXT", "r");

    munit_logf(MUNIT_LOG_INFO, "Start cluster: 0x%08X\n", fd->start_cluster);
    munit_assert(fd->start_cluster == 3);

    munit_logf(MUNIT_LOG_INFO, "File size in bytes: %lu\n", fd->size);
    munit_assert(fd->size == 20);

    char buf[20];
    char * expected = "This is a test file\n";
    munit_assert(f32_read(fd) == 20);
    for(int i = 0; i < 20; i++) {
        munit_assert(sec.data[i] == expected[i]);
    }

    munit_assert(f32_read(fd) == F32_EOF);

    munit_assert(f32_close(fd) == 0);

    munit_assert(f32_umount() == 0);
    return MUNIT_OK;
}

static MunitResult
test_hamlet_txt(const MunitParameter params[], void* data) {
    (void) params;
    (void) data;

    f32_sector sec;
    munit_assert(f32_mount(&sec) == 0);

    f32_file * fd = f32_open("HAMLET.TXT", "r");

    munit_logf(MUNIT_LOG_INFO, "Start cluster: 0x%08X\n", fd->start_cluster);
    munit_assert(fd->start_cluster == 4);

    munit_logf(MUNIT_LOG_INFO, "File size in bytes: %lu\n", fd->size);
    munit_assert(fd->size == 185977);

    FILE * act = fopen("tests/hamlet.txt", "r");
    uint8_t buf[SEC_SIZE];

    int i = 0;
    uint32_t br;
    while((br = f32_read(fd)) == SEC_SIZE) {
        // munit_logf(MUNIT_LOG_INFO, "Iteration %u, read: %lu\n", i, br);
        munit_assert(fread(buf, SEC_SIZE, 1, act) == 1);
        munit_assert_memory_equal(SEC_SIZE, buf, sec.data);
        i++;
    }

    // munit_logf(MUNIT_LOG_INFO, "Iteration %u, read: %lu\n", i, br);
    munit_assert(fread(buf, br, 1, act) == 1);
    munit_assert_memory_equal(br, buf, sec.data);
    fclose(act);

    munit_assert(f32_read(fd) == F32_EOF);

    munit_assert(f32_close(fd) == 0);

    munit_assert(f32_umount() == 0);
    return MUNIT_OK;
}

static MunitResult
test_dir_path_txt(const MunitParameter params[], void* data) {
    (void) params;
    (void) data;

    f32_sector sec;
    munit_assert(f32_mount(&sec) == 0);

    f32_file * fd = f32_open("/MYDIR~1/HAMLET.TXT", "r");

    munit_logf(MUNIT_LOG_INFO, "Start cluster: 0x%08X\n", fd->start_cluster);
    munit_assert(fd->start_cluster == 0x1C);

    munit_logf(MUNIT_LOG_INFO, "File size in bytes: %lu\n", fd->size);
    munit_assert(fd->size == 185977);

    FILE * act = fopen("tests/hamlet.txt", "r");
    uint8_t buf[SEC_SIZE];

    int i = 0;
    uint32_t br;
    while((br = f32_read(fd)) == SEC_SIZE) {
        // munit_logf(MUNIT_LOG_INFO, "Iteration %u, read: %lu\n", i, br);
        munit_assert(fread(buf, SEC_SIZE, 1, act) == 1);
        munit_assert_memory_equal(SEC_SIZE, buf, sec.data);
        i++;
    }

    // munit_logf(MUNIT_LOG_INFO, "Iteration %u, read: %lu\n", i, br);
    munit_assert(fread(buf, br, 1, act) == 1);
    munit_assert_memory_equal(br, buf, sec.data);
    fclose(act);

    munit_assert(f32_read(fd) == F32_EOF);

    munit_assert(f32_close(fd) == 0);

    munit_assert(f32_umount() == 0);
    return MUNIT_OK;
}

static MunitResult
test_dir_no_slash_path_txt(const MunitParameter params[], void* data) {
    (void) params;
    (void) data;

    f32_sector sec;
    munit_assert(f32_mount(&sec) == 0);

    f32_file * fd = f32_open("MYDIR~1/HAMLET.TXT", "r");

    munit_logf(MUNIT_LOG_INFO, "Start cluster: 0x%08X\n", fd->start_cluster);
    munit_assert(fd->start_cluster == 0x1C);

    munit_logf(MUNIT_LOG_INFO, "File size in bytes: %lu\n", fd->size);
    munit_assert(fd->size == 185977);

    FILE * act = fopen("tests/hamlet.txt", "r");
    uint8_t buf[SEC_SIZE];

    int i = 0;
    uint32_t br;
    while((br = f32_read(fd)) == SEC_SIZE) {
        // munit_logf(MUNIT_LOG_INFO, "Iteration %u, read: %lu\n", i, br);
        munit_assert(fread(buf, SEC_SIZE, 1, act) == 1);
        munit_assert_memory_equal(SEC_SIZE, buf, sec.data);
        i++;
    }

    // munit_logf(MUNIT_LOG_INFO, "Iteration %u, read: %lu\n", i, br);
    munit_assert(fread(buf, br, 1, act) == 1);
    munit_assert_memory_equal(br, buf, sec.data);
    fclose(act);

    munit_assert(f32_read(fd) == F32_EOF);

    munit_assert(f32_close(fd) == 0);

    munit_assert(f32_umount() == 0);
    return MUNIT_OK;
}

static MunitResult
test_dir_not_found_txt(const MunitParameter params[], void* data) {
    (void) params;
    (void) data;

    f32_sector sec;
    munit_assert(f32_mount(&sec) == 0);

    f32_file * fd = f32_open("/MYDIR~1/asdf.TXT", "r");

    munit_assert_null(fd);

    munit_assert(f32_umount() == 0);
    return MUNIT_OK;
}

static MunitResult
test_seek_hamlet_txt(const MunitParameter params[], void* data) {
    (void) params;
    (void) data;

    f32_sector sec;
    munit_assert(f32_mount(&sec) == 0);

    f32_file * fd = f32_open("HAMLET.TXT", "r");

    munit_assert_ptr_not_null(fd);

    munit_logf(MUNIT_LOG_INFO, "Start cluster: 0x%08X\n", fd->start_cluster);
    munit_assert(fd->start_cluster == 4);

    munit_logf(MUNIT_LOG_INFO, "File size in bytes: %lu\n", fd->size);
    munit_assert(fd->size == 185977);

    FILE * act = fopen("tests/hamlet.txt", "r");
    uint8_t buf[SEC_SIZE];

    munit_assert(f32_seek(fd, SEC_SIZE) == 0);
    fseek(act, SEC_SIZE, SEEK_SET);
    munit_assert(fread(buf, SEC_SIZE, 1, act) == 1);
    munit_assert(f32_read(fd) == SEC_SIZE);
    munit_assert_memory_equal(SEC_SIZE, buf, sec.data);

    munit_assert(f32_seek(fd, SEC_SIZE*100) == 0);
    fseek(act, SEC_SIZE*100, SEEK_SET);
    munit_assert(fread(buf, SEC_SIZE, 1, act) == 1);
    munit_assert(f32_read(fd) == SEC_SIZE);
    munit_assert_memory_equal(SEC_SIZE, buf, sec.data);

    munit_assert(f32_seek(fd, fd->size + 1) == 1);

    fclose(act);

    munit_assert(f32_close(fd) == 0);

    munit_assert(f32_umount() == 0);
    return MUNIT_OK;
}

static MunitResult
test_seek_hamlet_in_dir_txt(const MunitParameter params[], void* data) {
    (void) params;
    (void) data;

    f32_sector sec;
    munit_assert(f32_mount(&sec) == 0);

    f32_file * fd = f32_open("/MYDIR~1/HAMLET.TXT", "r");

    munit_assert_ptr_not_null(fd);

    munit_logf(MUNIT_LOG_INFO, "Start cluster: 0x%08X\n", fd->start_cluster);
    munit_assert(fd->start_cluster == 0x1C);

    munit_logf(MUNIT_LOG_INFO, "File size in bytes: %lu\n", fd->size);
    munit_assert(fd->size == 185977);

    FILE * act = fopen("tests/hamlet.txt", "r");
    uint8_t buf[SEC_SIZE];

    munit_assert(f32_seek(fd, SEC_SIZE) == 0);
    fseek(act, SEC_SIZE, SEEK_SET);
    munit_assert(fread(buf, SEC_SIZE, 1, act) == 1);
    munit_assert(f32_read(fd) == SEC_SIZE);
    munit_assert_memory_equal(SEC_SIZE, buf, sec.data);

    munit_assert(f32_seek(fd, SEC_SIZE*100) == 0);
    fseek(act, SEC_SIZE*100, SEEK_SET);
    munit_assert(fread(buf, SEC_SIZE, 1, act) == 1);
    munit_assert(f32_read(fd) == SEC_SIZE);
    munit_assert_memory_equal(SEC_SIZE, buf, sec.data);

    munit_assert(f32_seek(fd, fd->size + 1) == 1);

    fclose(act);

    munit_assert(f32_close(fd) == 0);

    munit_assert(f32_umount() == 0);
    return MUNIT_OK;
}

static MunitResult
test_write_hamlet(const MunitParameter params[], void* data) {
    (void) params;
    (void) data;

    f32_sector sec;
    munit_assert(f32_mount(&sec) == 0);

    f32_file * fd = f32_open("/MYDIR~1/SECRETS/ASDF.TXT", "w");

    munit_assert_ptr_not_null(fd);
    munit_assert(fd->size == 0);

    FILE * act = fopen("tests/hamlet.txt", "r");

    munit_assert_ptr_not_null(act);

    uint8_t buf[SEC_SIZE];
    while(fread(buf, SEC_SIZE, 1, act) == 1) {
        memcpy(sec.data, buf, SEC_SIZE);
        munit_assert(f32_write_sec(fd) == 0);
    }

    int i = 0;
    uint32_t br;
    munit_assert(f32_seek(fd, 0) == 0);
    fseek(act, 0, SEEK_SET);
    char tmp[513];
    while((br = f32_read(fd)) == SEC_SIZE) {
        munit_assert(fread(buf, SEC_SIZE, 1, act) == 1);
        munit_assert_memory_equal(SEC_SIZE, buf, sec.data);
        i++;
    }

    fclose(act);

    munit_assert(f32_close(fd) == 0);

    munit_assert(f32_umount() == 0);
    return MUNIT_OK;
}

static MunitResult
test_write_hamlet_root(const MunitParameter params[], void* data) {
    (void) params;
    (void) data;

    f32_sector sec;
    munit_assert(f32_mount(&sec) == 0);

    f32_file * fd = f32_open("ASDF.TXT", "w");

    munit_assert_ptr_not_null(fd);
    munit_assert(fd->size == 0);

    FILE * act = fopen("tests/hamlet.txt", "r");

    munit_assert_ptr_not_null(act);

    uint8_t buf[SEC_SIZE];
    while(fread(buf, SEC_SIZE, 1, act) == 1) {
        memcpy(sec.data, buf, SEC_SIZE);
        munit_assert(f32_write_sec(fd) == 0);
    }

    int i = 0;
    uint32_t br;
    munit_assert(f32_seek(fd, 0) == 0);
    fseek(act, 0, SEEK_SET);
    char tmp[513];
    while((br = f32_read(fd)) == SEC_SIZE) {
        munit_assert(fread(buf, SEC_SIZE, 1, act) == 1);
        munit_assert_memory_equal(SEC_SIZE, buf, sec.data);
        i++;
    }

    fclose(act);

    munit_assert(f32_close(fd) == 0);

    munit_assert(f32_umount() == 0);
    return MUNIT_OK;
}

/** Test file that is exactly aligned with cluster boundary */

static MunitTest test_suite_tests[] = {
    { (char*) "File not found", test_not_found_txt, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "Small file", test_test_txt, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "Large file", test_hamlet_txt, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "Directory", test_dir_path_txt, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "Directory with no slash", test_dir_no_slash_path_txt, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "Directory not found", test_dir_not_found_txt, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "Seek Hamlet", test_seek_hamlet_txt, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "Seek Hamlet in directory", test_seek_hamlet_in_dir_txt, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "Write Hamlet", test_write_hamlet, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "Write Hamlet in root", test_write_hamlet_root, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

static const MunitSuite test_suite = {
    (char*) "", test_suite_tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};

#include <stdlib.h>

int main(int argc, char* argv[]) {
  return munit_suite_main(&test_suite, (void*) "µnit", argc, argv);
}
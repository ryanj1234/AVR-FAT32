#ifndef _sd_card_h
#define _sd_card_h

#define SD_SUCCESS              0
#define SD_ERROR                1
#define SD_BLOCK_LEN            512

/**
 * Initialize sd card
 */
extern uint8_t sd_init(void);

/**
 * Read single 512 byte block
 * 
 * @param addr  Block address to read
 * @param buf   Pointer to buffer where data read will be stored
 * @param token Token response from read command. Values are
 *                  0xFE - Successful read
 *                  0x0X - Data error
 *                  0xFF - timeout
 * @return Response 1 from card
 */
extern uint8_t sd_read_block(uint32_t addr, uint8_t *buf);

/**
 * Write single 512 byte block
 * 
 * @param addr  Block address to write
 * @param buf   Data buffer that will be written
 * @param token Token response from write command. Values are
 *                  0x00 - busy timeout
 *                  0x05 - data accepted
 *                  0xFF - response timeout
 * 
 * @return Response 1 from card
 */
extern uint8_t sd_write_block(uint32_t addr, const uint8_t *buf);

#endif

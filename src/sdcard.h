#ifndef _sd_card_h
#define _sd_card_h

#define SD_CS_DDR   DDRB
#define SD_CS_PORT  PORTB
#define SD_CS_PIN   PINB2

/**
 * Initialize sd card
 */
uint8_t sd_init(void);

/**
 * Read single 512 byte block
 * 
 * @param addr  Block address to read
 * @param buf   Pointer to buffer where data read will be stored
 *
 * @return Response 1 from card
 */
uint8_t sd_read_block(uint32_t addr, uint8_t *buf);

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
uint8_t sd_write_block(uint32_t addr, const uint8_t *buf);

#endif

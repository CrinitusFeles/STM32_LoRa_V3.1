#define PREF_REC_ADDR_START       0x801FC00   /* ресурс - 1024 */
#define PREF_REC_ADDR_END         0x8020000  // page 128
#define MAIN_FW_ADDR              0x8000000
#define RESERVE_FW_ADDR           0x8020000
#define CONFIG_ADDR               0x801F800
#define CONFIG_PAGE               63
#define FW_PAGES_AMOUNT           64

#define M64(adr) (*((volatile uint64_t *) (adr)))

int8_t CheckBlock(register uint32_t *addr);
uint8_t HaveRunFlashBlockNum();
uint8_t HavePrefFlashBlockNum();

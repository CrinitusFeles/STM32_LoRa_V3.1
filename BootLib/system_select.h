#define PREF_REC_ADDR_START       0x800FF80   /* ресурс - 1024 */
#define PREF_REC_ADDR_END         0x8010000  // page 64
#define M64(adr) (*((volatile uint64_t *) (adr)))

int8_t CheckBlock(register uint32_t *addr);
uint8_t HaveRunFlashBlockNum();
uint8_t HavePrefFlashBlockNum();

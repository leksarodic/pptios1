/****************************************************************************
* Univerzitet u Novom Sadu, Fakultet tehnickih nauka
* Katedra za Računarsku Tehniku i Računarske Komunikacije
* -----------------------------------------------------
* Ispitni zadatak iz predmeta:
*
* PROGRAMSKA PODRSKA U TELEVIZIJI I OBRADI SLIKE 1
* -----------------------------------------------------
* Aplikacija za TV prijemnik
* -----------------------------------------------------
*
* Kreirano marta 2019.
*
* @Author Aleksa Rodić
*****************************************************************************/

#ifndef __TABLES_H__
#define __TABLES_H__

#include <stdint.h>

#define TABLES_MAX_NUMBER_OF_PIDS_IN_PAT 20    /* Max number of PMT pids in one PAT table */
#define TABLES_MAX_NUMBER_OF_ELEMENTARY_PID 20 /* Max number of elementary pids in one PMT table */

/**
 * @brief Enumeration of possible tables parser error codes
 */
typedef enum _ParseErrorCode
{
    TABLES_PARSE_ERROR = 0, /* TABLES_PARSE_ERROR */
    TABLES_PARSE_OK = 1     /* TABLES_PARSE_OK */
} ParseErrorCode;

/**
 * @brief Structure that defines PAT Table Header
 */
typedef struct _PatHeader
{
    uint8_t tableId;                /* The type of table */
    uint8_t sectionSyntaxIndicator; /* The format of the table section to follow */
    uint16_t sectionLength;         /* The length of the table section beyond this field */
    uint16_t transportStreamId;     /* Transport stream identifier */
    uint8_t versionNumber;          /* The version number the private table section */
    uint8_t currentNextIndicator;   /* Signals what a particular table will look like when it next changes */
    uint8_t sectionNumber;          /* Section number */
    uint8_t lastSectionNumber;      /* Signals the last section that is valid for a particular MPEG-2 private table */
} PatHeader;

/**
 * @brief Structure that defines PAT service info
 */
typedef struct _PatServiceInfo
{
    uint16_t programNumber; /* Identifies each service present in a transport stream */
    uint16_t pid;           /* Pid of Program Map table section or pid of Network Information Table  */
} PatServiceInfo;

/**
 * @brief Structure that defines PAT table
 */
typedef struct _PatTable
{
    PatHeader patHeader;                                                  /* PAT Table Header */
    PatServiceInfo patServiceInfoArray[TABLES_MAX_NUMBER_OF_PIDS_IN_PAT]; /* Services info presented in PAT table */
    uint8_t serviceInfoCount;                                             /* Number of services info presented in PAT table */
} PatTable;

/**
 * @brief Structure that defines PMT table header
 */
typedef struct _PmtTableHeader
{
    uint8_t tableId;
    uint8_t sectionSyntaxIndicator;
    uint16_t sectionLength;
    uint16_t programNumber;
    uint8_t versionNumber;
    uint8_t currentNextIndicator;
    uint8_t sectionNumber;
    uint8_t lastSectionNumber;
    uint16_t pcrPid;
    uint16_t programInfoLength;
} PmtTableHeader;

/**
 * @brief Structure that defines PMT elementary info
 */
typedef struct _PmtElementaryInfo
{
    uint8_t streamType;
    uint16_t elementaryPid;
    uint16_t esInfoLength;
} PmtElementaryInfo;

/**
 * @brief Structure that defines PMT table
 */
typedef struct _PmtTable
{
    PmtTableHeader pmtHeader;
    PmtElementaryInfo pmtElementaryInfoArray[TABLES_MAX_NUMBER_OF_ELEMENTARY_PID];
    uint8_t elementaryInfoCount;
} PmtTable;

/**
 * @brief Struktura zaglavlja EIT tabele
 */
typedef struct _EitTableHeader
{
    uint8_t table_id;
    uint16_t section_length;
    uint16_t servise_id;
    uint8_t section_number;
    uint8_t last_table_id;
} EitTableHeader;

/**
 * @brief Struktura dekriptora događaja
 */
typedef struct _ShortEventDescriptor
{
    uint8_t descriptor_tag;
    uint8_t descriptor_length;
    uint8_t event_name_length;
    char event_name_char[50];
} ShortEventDescriptor;

/**
 * @brief Struktura tela EIT tabele
 */
typedef struct _EitTableInfo
{
    uint8_t running_status;
    uint32_t duration;
    uint16_t descriptor_loop_length;
    ShortEventDescriptor event;
} EitTableInfo;

/**
 * @brief Struktura koja definiše celu EIT tabelu
 */
typedef struct _EitTable
{
    EitTableHeader eitHeader;
    EitTableInfo eitInfoArray[TABLES_MAX_NUMBER_OF_ELEMENTARY_PID];
    uint8_t numEventsInSection;
} EitTable;

/**
 * @brief  Parse PAT header.
 * 
 * @param  [in]   patHeaderBuffer Buffer that contains PAT header
 * @param  [out]  patHeader PAT header
 * @return tables error code
 */
ParseErrorCode parsePatHeader(const uint8_t *patHeaderBuffer, PatHeader *patHeader);

/**
 * @brief  Parse PAT Service information.
 * 
 * @param  [in]   patServiceInfoBuffer Buffer that contains PAT Service info
 * @param  [out]  descriptor PAT Service info
 * @return tables error code
 */
ParseErrorCode parsePatServiceInfo(const uint8_t *patServiceInfoBuffer, PatServiceInfo *patServiceInfo);

/**
 * @brief  Parse PAT Table.
 * 
 * @param  [in]   patSectionBuffer Buffer that contains PAT table section
 * @param  [out]  patTable PAT Table
 * @return tables error code
 */
ParseErrorCode parsePatTable(const uint8_t *patSectionBuffer, PatTable *patTable);

/**
 * @brief  Print PAT Table
 * 
 * @param  [in]   patTable PAT table to be printed
 * @return tables error code
 */
ParseErrorCode printPatTable(PatTable *patTable);

/**
 * @brief Parse pmt table
 *
 * @param [in]  pmtHeaderBuffer Buffer that contains PMT header
 * @param [out] pmtHeader PMT table header
 * @return tables error code
 */
ParseErrorCode parsePmtHeader(const uint8_t *pmtHeaderBuffer, PmtTableHeader *pmtHeader);

/**
 * @brief Parse PMT elementary info
 *
 * @param [in]  pmtElementaryInfoBuffer Buffer that contains pmt elementary info
 * @param [out] PMT elementary info
 * @return tables error code
 */
ParseErrorCode parsePmtElementaryInfo(const uint8_t *pmtElementaryInfoBuffer, PmtElementaryInfo *pmtElementaryInfo);

/**
 * @brief Parse PMT table
 *
 * @param [in]  pmtSectionBuffer Buffer that contains pmt table section
 * @param [out] pmtTable PMT table
 * @return tables error code
 */
ParseErrorCode parsePmtTable(const uint8_t *pmtSectionBuffer, PmtTable *pmtTable);

/**
 * @brief Print PMT table
 *
 * @param [in] pmtTable PMT table
 * @return tables error code
 */
ParseErrorCode printPmtTable(PmtTable *pmtTable);

/* Funkcije za obradu EIT tabele i short event deskriptora */

/**
 * @brief Parsiranje zaglavlja EIT tabele
 * 
 * @param eitHeaderBuffer
 * @param eitHeader 
 * @return ParseErrorCode 
 */
ParseErrorCode parseEitHeader(const uint8_t *eitHeaderBuffer, EitTableHeader *eitHeader);

/**
 * @brief Parsiranje tela EIT tabele
 * 
 * @param eitInfoBuffer 
 * @param EitInfo 
 * @return ParseErrorCode 
 */
ParseErrorCode parseEitInfo(const uint8_t *eitInfoBuffer, EitTableInfo *EitInfo);

/**
 * @brief Parsiranje EIT tabele
 * 
 * @param eitSectionBuffer 
 * @param eitTable 
 * @return ParseErrorCode 
 */
ParseErrorCode parseEitTable(const uint8_t *eitSectionBuffer, EitTable *eitTable);

/**
 * @brief Ispis sadržaja EIT tabele
 * 
 * @param eitTable 
 * @return ParseErrorCode 
 */
ParseErrorCode printEitTable(EitTable *eitTable);

#endif /* __TABLES_H__ */

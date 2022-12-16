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

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "tables.h"

ParseErrorCode parsePatHeader(const uint8_t *patHeaderBuffer, PatHeader *patHeader)
{
    if (patHeaderBuffer == NULL || patHeader == NULL)
    {
        printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    patHeader->tableId = (uint8_t)*patHeaderBuffer;
    if (patHeader->tableId != 0x00)
    {
        printf("\n%s : ERROR it is not a PAT Table\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    uint8_t lower8Bits = 0;
    uint8_t higher8Bits = 0;
    uint16_t all16Bits = 0;

    lower8Bits = (uint8_t)(*(patHeaderBuffer + 1));
    lower8Bits = lower8Bits >> 7;
    patHeader->sectionSyntaxIndicator = lower8Bits & 0x01;

    higher8Bits = (uint8_t)(*(patHeaderBuffer + 1));
    lower8Bits = (uint8_t)(*(patHeaderBuffer + 2));
    all16Bits = (uint16_t)((higher8Bits << 8) + lower8Bits);
    patHeader->sectionLength = all16Bits & 0x0FFF;

    higher8Bits = (uint8_t)(*(patHeaderBuffer + 3));
    lower8Bits = (uint8_t)(*(patHeaderBuffer + 4));
    all16Bits = (uint16_t)((higher8Bits << 8) + lower8Bits);
    patHeader->transportStreamId = all16Bits & 0xFFFF;

    lower8Bits = (uint8_t)(*(patHeaderBuffer + 5));
    lower8Bits = lower8Bits >> 1;
    patHeader->versionNumber = lower8Bits & 0x1F;

    lower8Bits = (uint8_t)(*(patHeaderBuffer + 5));
    patHeader->currentNextIndicator = lower8Bits & 0x01;

    lower8Bits = (uint8_t)(*(patHeaderBuffer + 6));
    patHeader->sectionNumber = lower8Bits & 0xFF;

    lower8Bits = (uint8_t)(*(patHeaderBuffer + 7));
    patHeader->lastSectionNumber = lower8Bits & 0xFF;

    return TABLES_PARSE_OK;
}

ParseErrorCode parsePatServiceInfo(const uint8_t *patServiceInfoBuffer, PatServiceInfo *patServiceInfo)
{
    if (patServiceInfoBuffer == NULL || patServiceInfo == NULL)
    {
        printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    uint8_t lower8Bits = 0;
    uint8_t higher8Bits = 0;
    uint16_t all16Bits = 0;

    higher8Bits = (uint8_t)(*(patServiceInfoBuffer));
    lower8Bits = (uint8_t)(*(patServiceInfoBuffer + 1));
    all16Bits = (uint16_t)((higher8Bits << 8) + lower8Bits);
    patServiceInfo->programNumber = all16Bits & 0xFFFF;

    higher8Bits = (uint8_t)(*(patServiceInfoBuffer + 2));
    lower8Bits = (uint8_t)(*(patServiceInfoBuffer + 3));
    all16Bits = (uint16_t)((higher8Bits << 8) + lower8Bits);
    patServiceInfo->pid = all16Bits & 0x1FFF;

    return TABLES_PARSE_OK;
}

ParseErrorCode parsePatTable(const uint8_t *patSectionBuffer, PatTable *patTable)
{
    uint8_t *currentBufferPosition = NULL;
    uint32_t parsedLength = 0;

    if (patSectionBuffer == NULL || patTable == NULL)
    {
        printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    if (parsePatHeader(patSectionBuffer, &(patTable->patHeader)) != TABLES_PARSE_OK)
    {
        printf("\n%s : ERROR parsing PAT header\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    parsedLength = 12 /*PAT header size*/ - 3 /*Not in section length*/;
    currentBufferPosition = (uint8_t *)(patSectionBuffer + 8); /* Position after last_section_number */
    patTable->serviceInfoCount = 0;                            /* Number of services info presented in PAT table */

    while (parsedLength < patTable->patHeader.sectionLength)
    {
        if (patTable->serviceInfoCount > TABLES_MAX_NUMBER_OF_PIDS_IN_PAT - 1)
        {
            printf("\n%s : ERROR there is not enough space in PAT structure for Service info\n", __FUNCTION__);
            return TABLES_PARSE_ERROR;
        }

        if (parsePatServiceInfo(currentBufferPosition, &(patTable->patServiceInfoArray[patTable->serviceInfoCount])) == TABLES_PARSE_OK)
        {
            currentBufferPosition += 4; /* Size from program_number to pid */
            parsedLength += 4;          /* Size from program_number to pid */
            patTable->serviceInfoCount++;
        }
    }

    return TABLES_PARSE_OK;
}

ParseErrorCode printPatTable(PatTable *patTable)
{
    uint8_t i = 0;

    if (patTable == NULL)
    {
        printf("\n%s : ERROR received parameter is not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    printf("\n********************PAT TABLE SECTION********************\n");
    printf("table_id                 |      %d\n", patTable->patHeader.tableId);
    printf("section_length           |      %d\n", patTable->patHeader.sectionLength);
    printf("transport_stream_id      |      %d\n", patTable->patHeader.transportStreamId);
    printf("section_number           |      %d\n", patTable->patHeader.sectionNumber);
    printf("last_section_number      |      %d\n", patTable->patHeader.lastSectionNumber);

    for (i = 0; i < patTable->serviceInfoCount; i++)
    {
        printf("-----------------------------------------\n");
        printf("program_number           |      %d\n", patTable->patServiceInfoArray[i].programNumber);
        printf("pid                      |      %d\n", patTable->patServiceInfoArray[i].pid);
    }
    printf("\n********************PAT TABLE SECTION********************\n");

    return TABLES_PARSE_OK;
}

ParseErrorCode parsePmtHeader(const uint8_t *pmtHeaderBuffer, PmtTableHeader *pmtHeader)
{

    if (pmtHeaderBuffer == NULL || pmtHeader == NULL)
    {
        printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    pmtHeader->tableId = (uint8_t)*pmtHeaderBuffer;
    if (pmtHeader->tableId != 0x02)
    {
        printf("\n%s : ERROR it is not a PMT Table\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    uint8_t lower8Bits = 0;
    uint8_t higher8Bits = 0;
    uint16_t all16Bits = 0;

    lower8Bits = (uint8_t)(*(pmtHeaderBuffer + 1));
    lower8Bits = lower8Bits >> 7;
    pmtHeader->sectionSyntaxIndicator = lower8Bits & 0x01;

    higher8Bits = (uint8_t)(*(pmtHeaderBuffer + 1));
    lower8Bits = (uint8_t)(*(pmtHeaderBuffer + 2));
    all16Bits = (uint16_t)((higher8Bits << 8) + lower8Bits);
    pmtHeader->sectionLength = all16Bits & 0x0FFF;

    higher8Bits = (uint8_t)(*(pmtHeaderBuffer + 3));
    lower8Bits = (uint8_t)(*(pmtHeaderBuffer + 4));
    all16Bits = (uint16_t)((higher8Bits << 8) + lower8Bits);
    pmtHeader->programNumber = all16Bits & 0xFFFF;

    lower8Bits = (uint8_t)(*(pmtHeaderBuffer + 5));
    lower8Bits = lower8Bits >> 1;
    pmtHeader->versionNumber = lower8Bits & 0x1F;

    lower8Bits = (uint8_t)(*(pmtHeaderBuffer + 5));
    pmtHeader->currentNextIndicator = lower8Bits & 0x01;

    lower8Bits = (uint8_t)(*(pmtHeaderBuffer + 6));
    pmtHeader->sectionNumber = lower8Bits & 0xFF;

    lower8Bits = (uint8_t)(*(pmtHeaderBuffer + 7));
    pmtHeader->lastSectionNumber = lower8Bits & 0xFF;

    higher8Bits = (uint8_t)(*(pmtHeaderBuffer + 8));
    lower8Bits = (uint8_t)(*(pmtHeaderBuffer + 9));
    all16Bits = (uint16_t)((higher8Bits << 8) + lower8Bits);
    pmtHeader->pcrPid = all16Bits & 0xFFFF;

    higher8Bits = (uint8_t)(*(pmtHeaderBuffer + 10));
    lower8Bits = (uint8_t)(*(pmtHeaderBuffer + 11));
    all16Bits = (uint16_t)((higher8Bits << 8) + lower8Bits);
    pmtHeader->programInfoLength = all16Bits & 0x0FFF;

    return TABLES_PARSE_OK;
}

ParseErrorCode parsePmtElementaryInfo(const uint8_t *pmtElementaryInfoBuffer, PmtElementaryInfo *pmtElementaryInfo)
{
    if (pmtElementaryInfoBuffer == NULL || pmtElementaryInfo == NULL)
    {
        printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    uint8_t lower8Bits = 0;
    uint8_t higher8Bits = 0;
    uint16_t all16Bits = 0;

    lower8Bits = (uint8_t)(*(pmtElementaryInfoBuffer));
    pmtElementaryInfo->streamType = lower8Bits;

    higher8Bits = (uint8_t)(*(pmtElementaryInfoBuffer + 1));
    lower8Bits = (uint8_t)(*(pmtElementaryInfoBuffer + 2));
    all16Bits = (uint16_t)((higher8Bits << 8) + lower8Bits);
    pmtElementaryInfo->elementaryPid = all16Bits & 0x1FFF;

    higher8Bits = (uint8_t)(*(pmtElementaryInfoBuffer + 3));
    lower8Bits = (uint8_t)(*(pmtElementaryInfoBuffer + 4));
    all16Bits = (uint16_t)((higher8Bits << 8) + lower8Bits);
    pmtElementaryInfo->esInfoLength = all16Bits & 0x0FFF;

    return TABLES_PARSE_OK;
}

ParseErrorCode parsePmtTable(const uint8_t *pmtSectionBuffer, PmtTable *pmtTable)
{
    uint8_t *currentBufferPosition = NULL;
    uint32_t parsedLength = 0;

    if (pmtSectionBuffer == NULL || pmtTable == NULL)
    {
        printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    if (parsePmtHeader(pmtSectionBuffer, &(pmtTable->pmtHeader)) != TABLES_PARSE_OK)
    {
        printf("\n%s : ERROR parsing PMT header\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    parsedLength = 12 + pmtTable->pmtHeader.programInfoLength /*PMT header size*/ + 4 /*CRC size*/ - 3 /*Not in section length*/;
    currentBufferPosition = (uint8_t *)(pmtSectionBuffer + 12 + pmtTable->pmtHeader.programInfoLength); /* Position after last descriptor */
    pmtTable->elementaryInfoCount = 0;                                                                  /* Number of elementary info presented in PMT table */

    while (parsedLength < pmtTable->pmtHeader.sectionLength)
    {
        if (pmtTable->elementaryInfoCount > TABLES_MAX_NUMBER_OF_ELEMENTARY_PID - 1)
        {
            printf("\n%s : ERROR there is not enough space in PMT structure for elementary info\n", __FUNCTION__);
            return TABLES_PARSE_ERROR;
        }

        if (parsePmtElementaryInfo(currentBufferPosition, &(pmtTable->pmtElementaryInfoArray[pmtTable->elementaryInfoCount])) == TABLES_PARSE_OK)
        {
            currentBufferPosition += 5 + pmtTable->pmtElementaryInfoArray[pmtTable->elementaryInfoCount].esInfoLength; /* Size from stream type to elemntary info descriptor*/
            parsedLength += 5 + pmtTable->pmtElementaryInfoArray[pmtTable->elementaryInfoCount].esInfoLength;          /* Size from stream type to elementary info descriptor */
            pmtTable->elementaryInfoCount++;
        }
    }

    return TABLES_PARSE_OK;
}

ParseErrorCode printPmtTable(PmtTable *pmtTable)
{
    uint8_t i = 0;

    if (pmtTable == NULL)
    {
        printf("\n%s : ERROR received parameter is not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    printf("\n********************PMT TABLE SECTION********************\n");
    printf("table_id                 |      %d\n", pmtTable->pmtHeader.tableId);
    printf("section_length           |      %d\n", pmtTable->pmtHeader.sectionLength);
    printf("program_number           |      %d\n", pmtTable->pmtHeader.programNumber);
    printf("section_number           |      %d\n", pmtTable->pmtHeader.sectionNumber);
    printf("last_section_number      |      %d\n", pmtTable->pmtHeader.lastSectionNumber);
    printf("program_info_legth       |      %d\n", pmtTable->pmtHeader.programInfoLength);

    for (i = 0; i < pmtTable->elementaryInfoCount; i++)
    {
        printf("-----------------------------------------\n");
        printf("stream_type              |      %d\n", pmtTable->pmtElementaryInfoArray[i].streamType);
        printf("elementary_pid           |      %d\n", pmtTable->pmtElementaryInfoArray[i].elementaryPid);
    }
    printf("\n********************PMT TABLE SECTION********************\n");

    return TABLES_PARSE_OK;
}

ParseErrorCode parseEitHeader(const uint8_t *eitHeaderBuffer, EitTableHeader *eitHeader)
{
    if (eitHeaderBuffer == NULL || eitHeader == NULL)
    {
        printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    eitHeader->table_id = (uint8_t)*eitHeaderBuffer;
    if (eitHeader->table_id != 0x4E)
    {
        printf("\n%s : ERROR it is not a EIT Table\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    uint8_t lower8Bits = 0;
    uint8_t higher8Bits = 0;
    uint16_t all16Bits = 0;

    higher8Bits = (uint8_t)(*(eitHeaderBuffer + 1));
    lower8Bits = (uint8_t)(*(eitHeaderBuffer + 2));
    all16Bits = (uint16_t)((higher8Bits << 8) + lower8Bits);
    eitHeader->section_length = all16Bits & 0x0FFF;

    higher8Bits = (uint8_t)(*(eitHeaderBuffer + 3));
    lower8Bits = (uint8_t)(*(eitHeaderBuffer + 4));
    all16Bits = (uint16_t)((higher8Bits << 8) + lower8Bits);
    eitHeader->servise_id = all16Bits;

    lower8Bits = (uint8_t)(*(eitHeaderBuffer + 6));
    eitHeader->section_number = lower8Bits;

    lower8Bits = (uint8_t)(*(eitHeaderBuffer + 13));
    eitHeader->last_table_id = lower8Bits;

    return TABLES_PARSE_OK;
}

ParseErrorCode parseEitInfo(const uint8_t *eitInfoBuffer, EitTableInfo *EitInfo)
{
    if (eitInfoBuffer == NULL || EitInfo == NULL)
    {
        printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    uint8_t lower8Bits = 0;
    uint8_t higher8Bits = 0;
    uint16_t all16BitsLower = 0;
    uint16_t all16BitsHigher = 0;
    uint32_t all32Bits;

    uint32_t i;
    uint32_t pomeraj = 0;

    /* Lenght in seconds */
    higher8Bits = (uint8_t)0;
    lower8Bits = (uint8_t)(*(eitInfoBuffer + 7));
    all16BitsHigher = (uint16_t)((higher8Bits << 8) + lower8Bits);

    higher8Bits = (uint8_t)(*(eitInfoBuffer + 8));
    lower8Bits = (uint8_t)(*(eitInfoBuffer + 9));
    all16BitsLower = (uint16_t)((higher8Bits << 8) + lower8Bits);

    all32Bits = (uint32_t)((all16BitsHigher << 16) + all16BitsLower);
    EitInfo->duration = all32Bits & 0x00FFFFFF;

    /* Running status */
    higher8Bits = (uint8_t)(*(eitInfoBuffer + 10));
    higher8Bits = (higher8Bits >> 5);
    EitInfo->running_status = higher8Bits & 0x03;

    /* Descriptor length */
    higher8Bits = (uint8_t)(*(eitInfoBuffer + 10));
    lower8Bits = (uint8_t)(*(eitInfoBuffer + 11));
    all16BitsLower = (uint16_t)((higher8Bits << 8) + lower8Bits);
    EitInfo->descriptor_loop_length = all16BitsLower & 0x0FFF;

    /* Obrada deskriptora */
    if (EitInfo->running_status == 0x4)
    {
        while (pomeraj < EitInfo->descriptor_loop_length)
        {
            EitInfo->event.descriptor_tag = *(eitInfoBuffer + 12 + pomeraj);
            EitInfo->event.descriptor_length = *(eitInfoBuffer + 12 + 1 + pomeraj);

            if (EitInfo->event.descriptor_tag == 0x4D)
            {
                EitInfo->event.event_name_length = *(eitInfoBuffer + 12 + 5 + pomeraj);
                for (i = 0; i < EitInfo->event.event_name_length; i++)
                {
                    EitInfo->event.event_name_char[i] = (char)(*(eitInfoBuffer + 12 + 6 + i + pomeraj + 1));
                }
                EitInfo->event.event_name_char[EitInfo->event.event_name_length - 1] = '\n';
            }

            pomeraj += EitInfo->event.descriptor_length + 2;
        }
    }

    return TABLES_PARSE_OK;
}

// FIXME: parseEitTable funkcija
ParseErrorCode parseEitTable(const uint8_t *eitSectionBuffer, EitTable *eitTable)
{
    uint8_t *currentBufferPosition = NULL;
    uint32_t parsedLength = 0;

    if (eitSectionBuffer == NULL || eitTable == NULL)
    {
        printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    if (parseEitHeader(eitSectionBuffer, &(eitTable->eitHeader)) != TABLES_PARSE_OK)
    {
        printf("\n%s : ERROR parsing EIT header\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    parsedLength = 14 /*EIT header size*/ + 4 /*CRC size*/ - 3;
    currentBufferPosition = (uint8_t *)(eitSectionBuffer + 14);
    eitTable->numEventsInSection = 0;

    while (parsedLength < eitTable->eitHeader.section_length)
    {
        if (eitTable->numEventsInSection > 3 - 1)
        {
            printf("\n%s : ERROR there is not enough space in EIT structure for elementary info\n", __FUNCTION__);
            return TABLES_PARSE_ERROR;
        }

        if (parseEitInfo(currentBufferPosition, &(eitTable->eitInfoArray[eitTable->numEventsInSection])) == TABLES_PARSE_OK)
        {
            currentBufferPosition += 12 + eitTable->eitInfoArray[eitTable->numEventsInSection].descriptor_loop_length;
            parsedLength += 12 + eitTable->eitInfoArray[eitTable->numEventsInSection].descriptor_loop_length;
            eitTable->numEventsInSection++;
        }
    }

    return TABLES_PARSE_OK;
}

ParseErrorCode printEitTable(EitTable *eitTable)
{
    uint8_t i = 0;

    if (eitTable == NULL)
    {
        printf("\n%s : ERROR received parameter is not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    printf("\n********************EIT TABLE SECTION********************\n");
    printf("table_id                 |      %d\n", eitTable->eitHeader.table_id);
    printf("section_length           |      %d\n", eitTable->eitHeader.section_length);
    printf("service_id               |      %d\n", eitTable->eitHeader.servise_id);
    printf("section_number           |      %d\n", eitTable->eitHeader.section_number);
    printf("last_table_id            |      %d\n", eitTable->eitHeader.last_table_id);

    for (i = 0; i < eitTable->numEventsInSection; i++)
    {
        printf("-----------------------------------------\n");
        printf("duration                 |      %d\n", eitTable->eitInfoArray[i].duration);
        printf("running_status           |      %d\n", eitTable->eitInfoArray[i].running_status);
        printf("descriptor_loop_length   |      %d\n", eitTable->eitInfoArray[i].descriptor_loop_length);
        printf("descriptor_length        |      %d\n", eitTable->eitInfoArray[i].event.descriptor_length);
        printf("descriptor_tag           |      %d\n", eitTable->eitInfoArray[i].event.descriptor_tag);
        printf("event_name_length        |      %d\n", eitTable->eitInfoArray[i].event.event_name_length);
        printf("event_name_char          |      %s\n", eitTable->eitInfoArray[i].event.event_name_char);
    }
    printf("\n********************EIT TABLE SECTION********************\n");

    return TABLES_PARSE_OK;
}
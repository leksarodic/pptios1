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
#include "tables.h"
#include "tdp_api.h"
#include "tables.h"
#include "pthread.h"
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>

#include "stream_controller.h"
#include "iscrtavanjeGrafike.h"

static PatTable *patTable;
static PmtTable *pmtTable;
static EitTable *eitTable;

static pthread_cond_t statusCondition = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t statusMutex = PTHREAD_MUTEX_INITIALIZER;

static int32_t sectionReceivedCallback(uint8_t *buffer);
static int32_t tunerStatusCallback(t_LockStatus status);

static uint32_t playerHandle = 0;
static uint32_t sourceHandle = 0;
static uint32_t streamHandleA = 0;
static uint32_t streamHandleV = 0;
static uint32_t filterHandle = 0;
static uint8_t threadExit = 0;
static bool changeChannel = false;
static int16_t programNumber = 0;
static ChannelInfo currentChannel;
static bool isInitialized = false;

static struct timespec lockStatusWaitTime;
static struct timeval now;
static pthread_t scThread;
static pthread_cond_t demuxCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t demuxMutex = PTHREAD_MUTEX_INITIALIZER;

static void *streamControllerTask();
static void startChannel(int32_t channelNumber);

StreamControllerError streamControllerInit()
{
    if (pthread_create(&scThread, NULL, &streamControllerTask, NULL))
    {
        printf("Error creating input event task!\n");
        return SC_THREAD_ERROR;
    }

    return SC_NO_ERROR;
}

StreamControllerError streamControllerDeinit()
{
    if (!isInitialized)
    {
        printf("\n%s : ERROR streamControllerDeinit() fail, module is not initialized!\n", __FUNCTION__);
        return SC_ERROR;
    }

    threadExit = 1;
    if (pthread_join(scThread, NULL))
    {
        printf("\n%s : ERROR pthread_join fail!\n", __FUNCTION__);
        return SC_THREAD_ERROR;
    }

    /* free demux filter */
    Demux_Free_Filter(playerHandle, filterHandle);

    /* remove audio stream */
    Player_Stream_Remove(playerHandle, sourceHandle, streamHandleA);

    /* remove video stream */
    Player_Stream_Remove(playerHandle, sourceHandle, streamHandleV);

    /* close player source */
    Player_Source_Close(playerHandle, sourceHandle);

    /* deinitialize player */
    Player_Deinit(playerHandle);

    /* deinitialize tuner device */
    Tuner_Deinit();

    /* free allocated memory */
    free(patTable);
    free(pmtTable);
    free(eitTable);

    /* set isInitialized flag */
    isInitialized = false;

    return SC_NO_ERROR;
}

StreamControllerError channelUp()
{
    if (programNumber >= patTable->serviceInfoCount - 2)
    {
        programNumber = 0;
    }
    else
    {
        programNumber++;
    }

    /* set flag to start current channel */
    changeChannel = true;

    return SC_NO_ERROR;
}

StreamControllerError channelDown()
{
    if (programNumber <= 0)
    {
        programNumber = patTable->serviceInfoCount - 2;
    }
    else
    {
        programNumber--;
    }

    /* set flag to start current channel */
    changeChannel = true;

    return SC_NO_ERROR;
}

StreamControllerError getChannelInfo(ChannelInfo *channelInfo)
{
    if (channelInfo == NULL)
    {
        printf("\n Error wrong parameter\n", __FUNCTION__);
        return SC_ERROR;
    }

    channelInfo->programNumber = currentChannel.programNumber;
    channelInfo->audioPid = currentChannel.audioPid;
    channelInfo->videoPid = currentChannel.videoPid;
    channelInfo->teletekst = currentChannel.teletekst;

    return SC_NO_ERROR;
}

/* Sets filter to receive current channel PMT table
 * Parses current channel PMT table when it arrives
 * Creates streams with current channel audio and video pids
 */
void startChannel(int32_t channelNumber)
{

    /* free PAT table filter */
    Demux_Free_Filter(playerHandle, filterHandle);

    /* set demux filter for receive PMT table of program */
    if (Demux_Set_Filter(playerHandle, patTable->patServiceInfoArray[channelNumber + 1].pid, 0x02, &filterHandle))
    {
        printf("\n%s : ERROR Demux_Set_Filter() fail\n", __FUNCTION__);
        return;
    }

    /* wait for a PMT table to be parsed*/
    pthread_mutex_lock(&demuxMutex);
    if (ETIMEDOUT == pthread_cond_wait(&demuxCond, &demuxMutex))
    {
        printf("\n%s : ERROR Lock timeout exceeded!\n", __FUNCTION__);
        streamControllerDeinit();
    }
    pthread_mutex_unlock(&demuxMutex);

    /* get audio and video pids */
    int16_t audioPid = -1;
    int16_t videoPid = -1;
    bool teletekst_dostupan = false;

    uint8_t i = 0;
    for (i = 0; i < pmtTable->elementaryInfoCount; i++)
    {
        if (((pmtTable->pmtElementaryInfoArray[i].streamType == 0x1) || (pmtTable->pmtElementaryInfoArray[i].streamType == 0x2) || (pmtTable->pmtElementaryInfoArray[i].streamType == 0x1b)) && (videoPid == -1))
        {
            videoPid = pmtTable->pmtElementaryInfoArray[i].elementaryPid;
        }
        else if (((pmtTable->pmtElementaryInfoArray[i].streamType == 0x3) || (pmtTable->pmtElementaryInfoArray[i].streamType == 0x4)) && (audioPid == -1))
        {
            audioPid = pmtTable->pmtElementaryInfoArray[i].elementaryPid;
        }

        /* Teletekst */
        if (pmtTable->pmtElementaryInfoArray[i].streamType == 0x6)
        {
            teletekst_dostupan = true;
        }
    }

    if (videoPid != -1)
    {
        /* Uklanjamo crni ekran kada prestane radio */
        ugasiRadio();

        /* remove previous video stream */
        if (streamHandleV != 0)
        {
            Player_Stream_Remove(playerHandle, sourceHandle, streamHandleV);
            streamHandleV = 0;
        }

        /* create video stream */
        if (Player_Stream_Create(playerHandle, sourceHandle, videoPid, VIDEO_TYPE_MPEG2, &streamHandleV))
        {
            printf("\n%s : ERROR Cannot create video stream\n", __FUNCTION__);
            streamControllerDeinit();
        }
    }

    /* Grafika kada slusamo radio */
    if (videoPid == -1)
    {
        /* remove previous video stream */
        if (streamHandleV != 0)
        {
            Player_Stream_Remove(playerHandle, sourceHandle, streamHandleV);
            streamHandleV = 0;
        }

        upaliRadio();
    }

    if (audioPid != -1)
    {
        /* remove previos audio stream */
        if (streamHandleA != 0)
        {
            Player_Stream_Remove(playerHandle, sourceHandle, streamHandleA);
            streamHandleA = 0;
        }

        /* create audio stream */
        if (Player_Stream_Create(playerHandle, sourceHandle, audioPid, AUDIO_TYPE_MPEG_AUDIO, &streamHandleA))
        {
            printf("\n%s : ERROR Cannot create audio stream\n", __FUNCTION__);
            streamControllerDeinit();
        }
    }

    /* store current channel info */
    currentChannel.programNumber = channelNumber + 1;
    currentChannel.audioPid = audioPid;
    currentChannel.videoPid = videoPid;
    currentChannel.teletekst = teletekst_dostupan;

    prikaziInfo(currentChannel.programNumber, currentChannel.teletekst, currentChannel.audioPid, currentChannel.videoPid, "startChannel data", 0);

    /* Filter za EIT tabelu, start */
    /* Oslobodi filter */
    Demux_Free_Filter(playerHandle, filterHandle);

    /* set demux filter for receive EIT table of program */
    if (Demux_Set_Filter(playerHandle, 0x12, 0x4E, &filterHandle))
    {
        printf("\n%s : ERROR Demux_Set_Filter() fail\n", __FUNCTION__);
        return;
    }

    /* wait for a EIT table to be parsed*/
    pthread_mutex_lock(&demuxMutex);
    if (ETIMEDOUT == pthread_cond_wait(&demuxCond, &demuxMutex))
    {
        printf("\n%s : ERROR Lock timeout exceeded!\n", __FUNCTION__);
        streamControllerDeinit();
    }
    pthread_mutex_unlock(&demuxMutex);

    Demux_Free_Filter(playerHandle, filterHandle);
    /* Filter za EIT tabelu, kraj */
}

void *streamControllerTask()
{
    gettimeofday(&now, NULL);
    lockStatusWaitTime.tv_sec = now.tv_sec + 10;

    /* allocate memory for PAT table section */
    patTable = (PatTable *)malloc(sizeof(PatTable));
    if (patTable == NULL)
    {
        printf("\n%s : ERROR Cannot allocate memory\n", __FUNCTION__);
        return (void *)SC_ERROR;
    }
    memset(patTable, 0x0, sizeof(PatTable));

    /* allocate memory for PMT table section */
    pmtTable = (PmtTable *)malloc(sizeof(PmtTable));
    if (pmtTable == NULL)
    {
        printf("\n%s : ERROR Cannot allocate memory\n", __FUNCTION__);
        return (void *)SC_ERROR;
    }
    memset(pmtTable, 0x0, sizeof(PmtTable));

    // EIT tabela
    eitTable = (EitTable *)malloc(sizeof(EitTable));
    if (eitTable == NULL)
    {
        printf("\n%s : ERROR Cannot allocate memory\n", __FUNCTION__);
        return (void *)SC_ERROR;
    }
    memset(eitTable, 0x0, sizeof(EitTable));

    /* initialize tuner device */
    if (Tuner_Init())
    {
        printf("\n%s : ERROR Tuner_Init() fail\n", __FUNCTION__);
        free(patTable);
        free(pmtTable);
        free(eitTable);
        return (void *)SC_ERROR;
    }

    /* register tuner status callback */
    if (Tuner_Register_Status_Callback(tunerStatusCallback))
    {
        printf("\n%s : ERROR Tuner_Register_Status_Callback() fail\n", __FUNCTION__);
    }

    /* lock to frequency */
    if (!Tuner_Lock_To_Frequency(DESIRED_FREQUENCY, BANDWIDTH, DVB_T))
    {
        printf("\n%s: INFO Tuner_Lock_To_Frequency(): %d Hz - success!\n", __FUNCTION__, DESIRED_FREQUENCY);
    }
    else
    {
        printf("\n%s: ERROR Tuner_Lock_To_Frequency(): %d Hz - fail!\n", __FUNCTION__, DESIRED_FREQUENCY);
        free(patTable);
        free(pmtTable);
        free(eitTable);
        Tuner_Deinit();
        return (void *)SC_ERROR;
    }

    /* wait for tuner to lock */
    pthread_mutex_lock(&statusMutex);
    if (ETIMEDOUT == pthread_cond_timedwait(&statusCondition, &statusMutex, &lockStatusWaitTime))
    {
        printf("\n%s : ERROR Lock timeout exceeded!\n", __FUNCTION__);
        free(patTable);
        free(pmtTable);
        free(eitTable);
        Tuner_Deinit();
        return (void *)SC_ERROR;
    }
    pthread_mutex_unlock(&statusMutex);

    /* initialize player */
    if (Player_Init(&playerHandle))
    {
        printf("\n%s : ERROR Player_Init() fail\n", __FUNCTION__);
        free(patTable);
        free(pmtTable);
        free(eitTable);
        Tuner_Deinit();
        return (void *)SC_ERROR;
    }

    /* open source */
    if (Player_Source_Open(playerHandle, &sourceHandle))
    {
        printf("\n%s : ERROR Player_Source_Open() fail\n", __FUNCTION__);
        free(patTable);
        free(pmtTable);
        free(eitTable);
        Player_Deinit(playerHandle);
        Tuner_Deinit();
        return (void *)SC_ERROR;
    }

    /* set PAT pid and tableID to demultiplexer */
    if (Demux_Set_Filter(playerHandle, 0x00, 0x00, &filterHandle))
    {
        printf("\n%s : ERROR Demux_Set_Filter() fail\n", __FUNCTION__);
    }

    /* register section filter callback */
    if (Demux_Register_Section_Filter_Callback(sectionReceivedCallback))
    {
        printf("\n%s : ERROR Demux_Register_Section_Filter_Callback() fail\n", __FUNCTION__);
    }

    pthread_mutex_lock(&demuxMutex);
    if (ETIMEDOUT == pthread_cond_wait(&demuxCond, &demuxMutex))
    {
        printf("\n%s:ERROR Lock timeout exceeded!\n", __FUNCTION__);
        free(patTable);
        free(pmtTable);
        free(eitTable);
        Player_Deinit(playerHandle);
        Tuner_Deinit();
        return (void *)SC_ERROR;
    }
    pthread_mutex_unlock(&demuxMutex);

    /* start current channel */
    startChannel(programNumber);

    /* set isInitialized flag */
    isInitialized = true;

    while (!threadExit)
    {
        if (changeChannel)
        {
            changeChannel = false;
            startChannel(programNumber);
        }
    }
}

int32_t sectionReceivedCallback(uint8_t *buffer)
{
    uint8_t tableId = *buffer;
    if (tableId == 0x00)
    {
        //printf("\n%s -----PAT TABLE ARRIVED-----\n",__FUNCTION__);

        if (parsePatTable(buffer, patTable) == TABLES_PARSE_OK)
        {
            //printPatTable(patTable);
            pthread_mutex_lock(&demuxMutex);
            pthread_cond_signal(&demuxCond);
            pthread_mutex_unlock(&demuxMutex);
        }
    }
    else if (tableId == 0x02)
    {
        //printf("\n%s -----PMT TABLE ARRIVED-----\n",__FUNCTION__);

        if (parsePmtTable(buffer, pmtTable) == TABLES_PARSE_OK)
        {
            //printPmtTable(pmtTable);
            pthread_mutex_lock(&demuxMutex);
            pthread_cond_signal(&demuxCond);
            pthread_mutex_unlock(&demuxMutex);
        }
    }
    else if (tableId == 0x4E)
    {
        //printf("\n%s -----EIT TABLE ARRIVED-----\n",__FUNCTION__);

        if (parseEitTable(buffer, eitTable) == TABLES_PARSE_OK)
        {
            printEitTable(eitTable);
            pthread_mutex_lock(&demuxMutex);
            pthread_cond_signal(&demuxCond);
            pthread_mutex_unlock(&demuxMutex);
        }
    }
    return 0;
}

int32_t tunerStatusCallback(t_LockStatus status)
{
    if (status == STATUS_LOCKED)
    {
        pthread_mutex_lock(&statusMutex);
        pthread_cond_signal(&statusCondition);
        pthread_mutex_unlock(&statusMutex);
        printf("\n%s -----TUNER LOCKED-----\n", __FUNCTION__);
    }
    else
    {
        printf("\n%s -----TUNER NOT LOCKED-----\n", __FUNCTION__);
    }
    return 0;
}

/**
 * @brief Na zahtev korisnika menjamo trenutni kanal, servis. Funkcija se poziva prilikom P+, P-, 0-9.
 * 
 * @param brojKanala 
 * @return StreamControllerError 
 */
StreamControllerError promenaKanala(uint16_t brojKanala)
{
    if ((brojKanala > -1) && (brojKanala < patTable->serviceInfoCount))
    {
        programNumber = brojKanala;
        changeChannel = true;
    }

    return SC_NO_ERROR;
}

/**
 * @brief Na zahtev korisnika menjamo jačinu zvuka.
 * 
 * @param novaJacinaZvuka 
 * @return StreamControllerError 
 */
StreamControllerError promenaJacineZvuka(uint32_t novaJacinaZvuka)
{
    uint32_t trenutnaJacinaZvuka = 0;
    Player_Volume_Get(playerHandle, &trenutnaJacinaZvuka);

    uint32_t racunanjeNoveJacineZvuka = novaJacinaZvuka * VOLUME_KONSTANTA_POJACAVANJA;
    Player_Volume_Set(playerHandle, racunanjeNoveJacineZvuka);

    prikaziJacineZvuka(novaJacinaZvuka);

    printf("PLAYER >> Jacina zvuka = %d\n", novaJacinaZvuka);

    return SC_NO_ERROR;
}

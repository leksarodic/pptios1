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

#ifndef __STREAM_CONTROLLER_H__
#define __STREAM_CONTROLLER_H__

#include <stdint.h>
#include <stdbool.h>

#define DESIRED_FREQUENCY 818000000 /* Tune frequency in Hz */
#define BANDWIDTH 8                 /* Bandwidth in Mhz */

/* Konstante za jačinu zvuka */
#define MAX_VOLUME 9
#define MIN_VOLUME 0
#define MUTE 0
#define VOLUME_KONSTANTA_POJACAVANJA 119304647

/**
 * @brief Structure that defines stream controller error
 */
typedef enum _StreamControllerError
{
    SC_NO_ERROR = 0,
    SC_ERROR,
    SC_THREAD_ERROR
} StreamControllerError;

/**
 * @brief Structure that defines channel info
 */
typedef struct _ChannelInfo
{
    int16_t programNumber;
    int16_t audioPid;
    int16_t videoPid;
    bool teletekst;
    uint32_t vremeTrajanja;
    char *trenutnoNaProgramu;
} ChannelInfo;

/**
 * @brief Initializes stream controller module
 *
 * @return stream controller error code
 */
StreamControllerError streamControllerInit();

/**
 * @brief Deinitializes stream controller module
 *
 * @return stream controller error code
 */
StreamControllerError streamControllerDeinit();

/**
 * @brief Channel up
 *
 * @return stream controller error
 */
StreamControllerError channelUp();

/**
 * @brief Channel down
 *
 * @return stream controller error
 */
StreamControllerError channelDown();

/**
 * @brief Returns current channel info
 *
 * @param [out] channelInfo - channel info structure with current channel info
 * @return stream controller error code
 */
StreamControllerError getChannelInfo(ChannelInfo *channelInfo);

/**
 * @brief Funkcija menja promenjivu programNumber u željeni broj kanala i obaveštava streamer da je potrebno promeniti kanal
 * 
 * @param brojKanala 
 * @return StreamControllerError
 */
StreamControllerError promenaKanala(uint16_t brojKanala);

/**
 * @brief Funkcija postavlja novu jačinu zvuka. Unosi se vrednost 0-9 što se prevodi na odgovarajuću vrednost za "player".
 * 
 * @param novaJacinaZvuka Vrednosti 0-9
 * @return StreamControllerError 
 */
StreamControllerError promenaJacineZvuka(uint32_t novaJacinaZvuka);

#endif /* __STREAM_CONTROLLER_H__ */

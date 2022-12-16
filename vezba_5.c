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

#include "remote_controller.h"
#include "stream_controller.h"
#include "iscrtavanjeGrafike.h"
#include "parsiranjeUlaznogFajla.h"

static inline void textColor(int32_t attr, int32_t fg, int32_t bg)
{
    char command[13];

    /* command is the control command to the terminal */
    sprintf(command, "%c[%d;%d;%dm", 0x1B, attr, fg + 30, bg + 40);
    printf("%s", command);
}

/* macro function for error checking */
#define ERRORCHECK(x)                                                         \
    {                                                                         \
        if (x != 0)                                                           \
        {                                                                     \
            textColor(1, 1, 0);                                               \
            printf(" Error!\n File: %s \t Line: <%d>\n", __FILE__, __LINE__); \
            textColor(0, 7, 0);                                               \
            return -1;                                                        \
        }                                                                     \
    }

static void remoteControllerCallback(uint16_t code, uint16_t type, uint32_t value);
static pthread_cond_t deinitCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t deinitMutex = PTHREAD_MUTEX_INITIALIZER;
static ChannelInfo channelInfo;

int main(int argc, char* argv[])
{
    /* Provera ulaznih parametara */
    if (argc != 2)
    {
        printf("Usage: ./program ulazniFajl.config\n");
        return 1;
    }

    /* Parsiranje ulaznoj fajla */
    KonfiguracioniFajl konfiguracioniFajl;
    parsiranjeKonfigFajla(argv[1], &konfiguracioniFajl);
    ispisKonfigFajl(&konfiguracioniFajl);

    /* initialize remote controller module */
    ERRORCHECK(remoteControllerInit());

    /* register remote controller callback */
    ERRORCHECK(registerRemoteControllerCallback(remoteControllerCallback));

    /* initialize stream controller module */
    ERRORCHECK(streamControllerInit());

    /* Inicijalizacija grafike */
    ERRORCHECK(inicijalizacijaGrafike());

    /* Postavka pocetne vrednosti jacine zvuka */
    postaviJacinuZvuka(3);

    /* wait for a EXIT remote controller key press event */
    pthread_mutex_lock(&deinitMutex);
    if (ETIMEDOUT == pthread_cond_wait(&deinitCond, &deinitMutex))
    {
        printf("\n%s : ERROR Lock timeout exceeded!\n", __FUNCTION__);
    }
    pthread_mutex_unlock(&deinitMutex);

    /* unregister remote controller callback */
    ERRORCHECK(unregisterRemoteControllerCallback(remoteControllerCallback));

    /* Deinicijalizacija grafike */
    ERRORCHECK(deinicijalizacijaGrafike());

    /* deinitialize remote controller module */
    ERRORCHECK(remoteControllerDeinit());

    /* deinitialize stream controller module */
    ERRORCHECK(streamControllerDeinit());

    return 0;
}

void remoteControllerCallback(uint16_t code, uint16_t type, uint32_t value)
{
    switch (code)
    {
    case KEYCODE_INFO:
        printf("\nInfo pressed\n");
        if (getChannelInfo(&channelInfo) == SC_NO_ERROR)
        {
            printf("\n********************* Channel info *********************\n");
            printf("Program number: %d\n", channelInfo.programNumber);
            printf("Audio pid: %d\n", channelInfo.audioPid);
            printf("Video pid: %d\n", channelInfo.videoPid);
            printf("Teletekst: %d\n", channelInfo.teletekst);
            printf("**********************************************************\n");

            // TODO: Ispis šta je trenutno na programu i trajanja
            prikaziInfo(channelInfo.programNumber, channelInfo.teletekst, channelInfo.audioPid, channelInfo.videoPid, "INFO button", 360);
        }
        break;
    case KEYCODE_P_PLUS:
        printf("\nCH+ pressed\n");
        channelUp();
        break;
    case KEYCODE_P_MINUS:
        printf("\nCH- pressed\n");
        channelDown();
        break;
    case KEYCODE_V_PLUS:
        printf("\nV+ pressed\n");
        promenaJacineZvuka(pojacaj());
        break;
    case KEYCODE_V_MINUS:
        printf("\nV- pressed\n");
        promenaJacineZvuka(smanji());
        break;
    case KEYCODE_MUTE:
        printf("\nMUTE pressed\n");
        promenaJacineZvuka(postaviJacinuZvuka(MUTE));
        break;
    case KEYCODE_0:
        printf("\n'0' pressed\n");
        prikaziBrojKanala(0);
        promenaKanala(0);
        break;
    case KEYCODE_1:
        printf("\n'1' pressed\n");
        prikaziBrojKanala(1);
        promenaKanala(1);
        break;
    case KEYCODE_2:
        printf("\n'2' pressed\n");
        prikaziBrojKanala(2);
        promenaKanala(2);
        break;
    case KEYCODE_3:
        printf("\n'3' pressed\n");
        prikaziBrojKanala(3);
        promenaKanala(3);
        break;
    case KEYCODE_4:
        printf("\n'4' pressed\n");
        prikaziBrojKanala(4);
        promenaKanala(4);
        break;
    case KEYCODE_5:
        printf("\n'5' pressed\n");
        prikaziBrojKanala(5);
        promenaKanala(5);
        break;
    case KEYCODE_6:
        printf("\n'6' pressed\n");
        prikaziBrojKanala(6);
        promenaKanala(6);
        break;
    case KEYCODE_7:
        printf("\n'7' pressed\n");
        prikaziBrojKanala(7);
        promenaKanala(7);
        break;
    case KEYCODE_8:
        printf("\n'8' pressed\n");
        prikaziBrojKanala(8);
        promenaKanala(8);
        break;
    case KEYCODE_9:
        printf("\n'9' pressed\n");
        prikaziBrojKanala(9);
        promenaKanala(9);
        break;
    case KEYCODE_EXIT:
        printf("\nExit pressed\n");
        pthread_mutex_lock(&deinitMutex);
        pthread_cond_signal(&deinitCond);
        pthread_mutex_unlock(&deinitMutex);
        break;
    default:
        printf("\nPress P+, P-, V+, V-, MUTE, info or exit! \n\n");
    }
}

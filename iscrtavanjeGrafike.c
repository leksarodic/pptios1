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
#include <directfb.h>
#include <linux/input.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>

#include "iscrtavanjeGrafike.h"

/* Globalne promenjive */
static IsrtavanjeGrafikeInfo grafikaInfo;

static IDirectFBSurface *primary = NULL;
static IDirectFB *dfbInterface = NULL;
static IDirectFBFont *fontInterface = NULL;
static DFBFontDescription fontDesc;
static DFBSurfaceDescription surfaceDesc;
static int32_t screenWidth = 0;
static int32_t screenHeight = 0;
static IDirectFBImageProvider *provider;
static IDirectFBSurface *volumeSurface = NULL;
static int32_t volumeHeight, volumeWidth;

/* Tajmeri */
/* Ispis broj kanala */
static timer_t ispisBrojKanalaTimerId;
static struct sigevent ispisBrojKanalaSignalEvent;
static struct itimerspec ispisBrojKanalaTimerSpec;
static struct itimerspec ispisBrojKanalaTimerSpecOld;

/* Ispis jacine zvuka */
static timer_t ispisJacinaZvukaTimerId;
static struct sigevent ispisJacinaZvukaSignalEvent;
static struct itimerspec ispisJacinaZvukaTimerSpec;
static struct itimerspec ispisJacinaZvukaTimerSpecOld;

/* Ispis informacija */
static timer_t ispisInformacijaTimerId;
static struct sigevent ispisInformacijaSignalEvent;
static struct itimerspec ispisInformacijaTimerSpec;
static struct itimerspec ispisInformacijaTimerSpecOld;

/* Niti */
static pthread_t renderNit;
static pthread_mutex_t grafikaMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t grafikaUslov = PTHREAD_COND_INITIALIZER;

PovratnaVrednostGrafika inicijalizacijaGrafike()
{
    printf("GRAFIKA >> Inicijalizacija, pocetak\n");

    /* Inicijalizacija tajmera */
    /* Ispis broja kanala */
    ispisBrojKanalaSignalEvent.sigev_notify = SIGEV_THREAD;
    ispisBrojKanalaSignalEvent.sigev_notify_function = (void *)ugasiIspisBrojKanala;
    ispisBrojKanalaSignalEvent.sigev_value.sival_ptr = NULL;
    ispisBrojKanalaSignalEvent.sigev_notify_attributes = NULL;
    timer_create(CLOCK_REALTIME, &ispisBrojKanalaSignalEvent, &ispisBrojKanalaTimerId);

    /* Ispis jačine zvuka */
    ispisJacinaZvukaSignalEvent.sigev_notify = SIGEV_THREAD;
    ispisJacinaZvukaSignalEvent.sigev_notify_function = (void *)ugasiIspisJacinaZvuka;
    ispisJacinaZvukaSignalEvent.sigev_value.sival_ptr = NULL;
    ispisJacinaZvukaSignalEvent.sigev_notify_attributes = NULL;
    timer_create(CLOCK_REALTIME, &ispisJacinaZvukaSignalEvent, &ispisJacinaZvukaTimerId);

    /* Ispis informacija */
    ispisInformacijaSignalEvent.sigev_notify = SIGEV_THREAD;
    ispisInformacijaSignalEvent.sigev_notify_function = (void *)ugasiIspsiInformacija;
    ispisInformacijaSignalEvent.sigev_value.sival_ptr = NULL;
    ispisInformacijaSignalEvent.sigev_notify_attributes = NULL;
    timer_create(CLOCK_REALTIME, &ispisInformacijaSignalEvent, &ispisInformacijaTimerId);

    /* Direct Frame Buffer */
    DFBCHECK(DirectFBInit(NULL, NULL));
    DFBCHECK(DirectFBCreate(&dfbInterface));
    DFBCHECK(dfbInterface->SetCooperativeLevel(dfbInterface, DFSCL_FULLSCREEN));

    surfaceDesc.flags = DSDESC_CAPS;
    surfaceDesc.caps = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
    DFBCHECK(dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &primary));

    fontDesc.flags = DFDESC_HEIGHT;
    fontDesc.height = 48;
    DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
    DFBCHECK(primary->SetFont(primary, fontInterface));

    DFBCHECK(primary->GetSize(primary, &screenWidth, &screenHeight));

    /* Pre pisanja "postavi praznu površinu" */
    obrisiEkran();

    /* Flag grafikaInfo.crtajGrafiku */
    grafikaInfo.crtajGrafiku = true;

    /* Kreiramo nit "render" koja na naš zahtev ispisuje određeni element */
    if (pthread_create(&renderNit, NULL, &render, NULL))
    {
        printf("Greska prilikom pokretanja niti!\n");
        return GRAFIKA_GRESKA_NIT;
    }

    printf("GRAFIKA >> Inicijalizacija, kraj\n");

    return GRAFIKA_USPEH;
}

PovratnaVrednostGrafika deinicijalizacijaGrafike()
{
    printf("GRAFIKA >> Deinicijalizacija, pocetak\n");

    /* Obustavljamo crtanje grafike */
    grafikaInfo.crtajGrafiku = false;

    // Spajanje niti
    pthread_mutex_lock(&grafikaMutex);
    if (ETIMEDOUT == pthread_cond_wait(&grafikaUslov, &grafikaMutex))
    {
        printf("\n%s : ERROR Lock timeout exceeded!\n", __FUNCTION__);
    }
    pthread_mutex_unlock(&grafikaMutex);

    if (pthread_join(renderNit, NULL))
    {
        printf("\n%s : ERROR pthread_join fail!\n", __FUNCTION__);
        return GRAFIKA_GRESKA_NIT;
    }

    /* Brišemo tajmere */
    timer_delete(ispisBrojKanalaTimerId);
    timer_delete(ispisJacinaZvukaTimerId);
    timer_delete(ispisInformacijaTimerId);

    /* DirectFB */
    primary->Release(primary);
    dfbInterface->Release(dfbInterface);

    printf("GRAFIKA >> Deinicijalizacija, kraj\n");

    return GRAFIKA_USPEH;
}

void ugasiIspisBrojKanala()
{
    grafikaInfo.ispisBrojaKanala = false;
}

void ugasiIspisJacinaZvuka()
{
    grafikaInfo.ispisJacineZvuka = false;
}

void ugasiIspsiInformacija()
{
    grafikaInfo.ispisInformacija = false;
}

void upaliRadio()
{
    grafikaInfo.ispisRadio = true;
}

void ugasiRadio()
{
    grafikaInfo.ispisRadio = false;
}

void obrisiEkran()
{
    DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
    DFBCHECK(primary->FillRectangle(primary, 0, 0, screenWidth, screenHeight));
}

void prikaziInfo(int16_t brojKanala, bool teletekst, int16_t audioPID, int16_t videoPID, char *trenutnoNaProgramu, int32_t trajanjeSekundi)
{
    /* Prosleđivanje informacija */
    grafikaInfo.brojKanala = brojKanala;
    grafikaInfo.teletekst = teletekst;
    grafikaInfo.audioPID = audioPID;
    grafikaInfo.videoPID = videoPID;
    grafikaInfo.trenutnoNaProgramu = trenutnoNaProgramu; // FIXME: Potencijalni problem, potrebno uraditi kopitanje stringa

    /* Obrada informacija o vremenu */
    grafikaInfo.trajanjeSati = (uint8_t)((trajanjeSekundi % 86400) / 3600);
    grafikaInfo.trajanjeMinuta = (uint8_t)((trajanjeSekundi % 3600) / 60);

    /* Pokrećemo tajmer */
    memset(&ispisInformacijaTimerSpec, 0, sizeof(ispisInformacijaTimerSpec));
    ispisInformacijaTimerSpec.it_value.tv_sec = 3;
    ispisInformacijaTimerSpec.it_value.tv_nsec = 0;
    timer_settime(ispisInformacijaTimerId, 0, &ispisInformacijaTimerSpec, &ispisInformacijaTimerSpec);

    /* Govorimo modulu za renderovanje da iscrta komponentu za ispis broja kanala */
    grafikaInfo.ispisInformacija = true;
}

void prikaziBrojKanala(int16_t brojKanala)
{
    /* prosleđujemo informaciju o broju kanala */
    grafikaInfo.brojKanala = brojKanala;

    /* Pokrećemo tajmer */
    memset(&ispisBrojKanalaTimerSpec, 0, sizeof(ispisBrojKanalaTimerSpec));
    ispisBrojKanalaTimerSpec.it_value.tv_sec = 3;
    ispisBrojKanalaTimerSpec.it_value.tv_nsec = 0;
    timer_settime(ispisBrojKanalaTimerId, 0, &ispisBrojKanalaTimerSpec, &ispisBrojKanalaTimerSpec);

    /* Govorimo modulu za renderovanje da iscrta komponentu za ispis broja kanala */
    grafikaInfo.ispisBrojaKanala = true;
}

void prikaziJacineZvuka(uint8_t jacinaZvuka)
{
    /* prosleđujemo informaciju o broju kanala */
    grafikaInfo.jacinaZvuka = jacinaZvuka;

    /* Pokrećemo tajmer */
    memset(&ispisJacinaZvukaTimerSpec, 0, sizeof(ispisJacinaZvukaTimerSpec));
    ispisJacinaZvukaTimerSpec.it_value.tv_sec = 3;
    ispisJacinaZvukaTimerSpec.it_value.tv_nsec = 0;
    timer_settime(ispisJacinaZvukaTimerId, 0, &ispisJacinaZvukaTimerSpec, &ispisJacinaZvukaTimerSpec);

    /* Govorimo modulu za renderovanje da iscrta komponentu za ispis broja kanala */
    grafikaInfo.ispisJacineZvuka = true;
}

/**
 * @brief Thread for rendering graphic
 * 
 * @return void* 
 */
void *render()
{
    char ispisString[25];

    /* Info */
    printf("grafikaInfo.crtajGrafiku = %d\n", grafikaInfo.crtajGrafiku);
    printf("grafikaInfo.ispisBrojaKanala = %d\n", grafikaInfo.ispisBrojaKanala);

    while (grafikaInfo.crtajGrafiku == true)
    {
        /* Svaki put iscrtavamo sve iz početak, zato na je potrebna čista površina */
        obrisiEkran();

        /* Crtaj radio, njega crtamo prvog ako "postoji" */
        if (grafikaInfo.ispisRadio == true)
        {
            /* Pozadina, crna */
            DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xFF));
            DFBCHECK(primary->FillRectangle(primary, 0, 0, screenWidth, screenHeight));

            /* Tekst, bele boje */
            DFBCHECK(primary->SetColor(primary, 0xFF, 0xFF, 0xFF, 0xFF));
            fontDesc.height = 98;

            /* Ispis da je videopid = -1 */
            sprintf(ispisString, "Radio");
            DFBCHECK(primary->DrawString(primary, ispisString, -1, screenWidth - 250, 500, DSTF_LEFT));
        }

        /* Crtaj informacije */
        if (grafikaInfo.ispisInformacija == true)
        {
            /* Pozadina, sivkasta */
            DFBCHECK(primary->SetColor(primary, 0x33, 0x33, 0x33, 0xFF));
            DFBCHECK(primary->FillRectangle(primary, 50, 50, 500, 350));

            /* Tekst, bele boje */
            DFBCHECK(primary->SetColor(primary, 0xFF, 0xFF, 0xFF, 0xFF));
            fontDesc.height = 48;

            /* Broj programa */
            sprintf(ispisString, "Broj programa: %d", grafikaInfo.brojKanala);
            DFBCHECK(primary->DrawString(primary, ispisString, -1, 100, 100, DSTF_LEFT));

            /* Audio PID */
            sprintf(ispisString, "Audio PID: %d", grafikaInfo.audioPID);
            DFBCHECK(primary->DrawString(primary, ispisString, -1, 100, 150, DSTF_LEFT));

            /* Video PID */
            if (grafikaInfo.videoPID != -1)
            {
                sprintf(ispisString, "Video PID: %d", grafikaInfo.videoPID);
                DFBCHECK(primary->DrawString(primary, ispisString, -1, 100, 200, DSTF_LEFT));
            }
            else
            {
                sprintf(ispisString, "Radio");
                DFBCHECK(primary->DrawString(primary, ispisString, -1, 100, 200, DSTF_LEFT));
            }

            /* Teletekst */
            if (grafikaInfo.teletekst == true)
            {
                sprintf(ispisString, "TXT dostupan");
            }
            else
            {
                sprintf(ispisString, "TXT nije dostupan");
            }
            DFBCHECK(primary->DrawString(primary, ispisString, -1, 100, 250, DSTF_LEFT));

            /* Trenutno na programu */
            // TODO: Dodati ispis ako informacija nije dostupna
            DFBCHECK(primary->DrawString(primary, grafikaInfo.trenutnoNaProgramu, -1, 100, 300, DSTF_LEFT));

            /* Trajanje */
            if ((grafikaInfo.trajanjeSati == 0) && (grafikaInfo.trajanjeMinuta == 0))
            {
                sprintf(ispisString, "Vreme nije dostupno");
            }
            else
            {
                if (grafikaInfo.trajanjeMinuta == 0)
                {
                    sprintf(ispisString, "Vreme: %dh", grafikaInfo.trajanjeSati);
                }
                else if (grafikaInfo.trajanjeSati == 0)
                {
                    sprintf(ispisString, "Vreme: %dm", grafikaInfo.trajanjeMinuta);
                }
                else
                {
                    sprintf(ispisString, "Vreme: %dh %dm", grafikaInfo.trajanjeSati, grafikaInfo.trajanjeMinuta);
                }
            }
            DFBCHECK(primary->DrawString(primary, ispisString, -1, 100, 350, DSTF_LEFT));
        }

        /* Crtaj promenu kanala P+, P- ili 0-9 */
        if (grafikaInfo.ispisBrojaKanala == true)
        {
            char ispisString[3];

            /* Pozadina, crvena */
            DFBCHECK(primary->SetColor(primary, 0x77, 0x00, 0x00, 0xFF));
            DFBCHECK(primary->FillRectangle(primary, screenWidth - 150, 50, 100, 100));

            /* Tekst, bele boje */
            DFBCHECK(primary->SetColor(primary, 0xFF, 0xFF, 0xFF, 0xFF));
            fontDesc.height = 120;

            /* Broj kanala */
            sprintf(ispisString, "%d", grafikaInfo.brojKanala);
            DFBCHECK(primary->DrawString(primary, ispisString, -1, screenWidth - 125, 125, DSTF_LEFT));
        }

        /* Crtaj promenu jačine zvuka, V+, V-, MUTE */
        if (grafikaInfo.ispisJacineZvuka == true)
        {
            switch (grafikaInfo.jacinaZvuka)
            {
            case 0:
                DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "grafike//volume_0.png", &provider));
                break;

            case 1:
                DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "grafike//volume_1.png", &provider));
                break;

            case 2:
                DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "grafike//volume_2.png", &provider));
                break;

            case 3:
                DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "grafike//volume_3.png", &provider));
                break;

            case 4:
                DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "grafike//volume_4.png", &provider));
                break;

            case 5:
                DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "grafike//volume_5.png", &provider));
                break;

            case 6:
                DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "grafike//volume_6.png", &provider));
                break;

            case 7:
                DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "grafike//volume_7.png", &provider));
                break;

            case 8:
                DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "grafike//volume_8.png", &provider));
                break;

            case 9:
                DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "grafike//volume_9.png", &provider));
                break;

            default:
                // TODO: Proveriti
                break;
            }

            DFBCHECK(provider->GetSurfaceDescription(provider, &surfaceDesc));
            DFBCHECK(dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &volumeSurface));
            DFBCHECK(provider->RenderTo(provider, volumeSurface, NULL));

            provider->Release(provider);

            DFBCHECK(volumeSurface->GetSize(volumeSurface, &volumeWidth, &volumeHeight));
            DFBCHECK(primary->Blit(primary, volumeSurface, NULL, screenWidth - volumeWidth - 50, 200));
        }

        /* Iscrtaj */
        DFBCHECK(primary->Flip(primary, NULL, 0));

        pthread_mutex_lock(&grafikaMutex);
        pthread_cond_signal(&grafikaUslov);
        pthread_mutex_unlock(&grafikaMutex);
    }
}

uint32_t pojacaj()
{
    if (grafikaInfo.jacinaZvuka < MAX_VOLUME)
    {
        grafikaInfo.jacinaZvuka++;
    }

    return grafikaInfo.jacinaZvuka;
}
uint32_t smanji()
{
    if (grafikaInfo.jacinaZvuka > MIN_VOLUME)
    {
        grafikaInfo.jacinaZvuka--;
    }

    return grafikaInfo.jacinaZvuka;
}

uint32_t postaviJacinuZvuka(uint32_t novaJacinaZvuka)
{
    if ((novaJacinaZvuka >= MIN_VOLUME) && (novaJacinaZvuka <= MAX_VOLUME))
    {
        grafikaInfo.jacinaZvuka = novaJacinaZvuka;
    }
    else
    {
        printf("Jacinu zvuka nije moguce postaviti na %d vrednost. Moguce vrednosti [0-10]\n", novaJacinaZvuka);
    }

    return grafikaInfo.jacinaZvuka;
}

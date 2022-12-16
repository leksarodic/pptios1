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

#ifndef __ISCRTAVANJE_GRAFIKE_H__
#define __ISCRTAVANJE_GRAFIKE_H__

#include <stdint.h>

typedef struct _iscrtavanjeGrafikeInfo
{
    /* Grafika enable flag */
    bool crtajGrafiku;

    /* Kontole ispisa elemenata */
    bool ispisBrojaKanala;
    bool ispisJacineZvuka;
    bool ispisInformacija;
    bool ispisRadio;

    /* Podaci za ispis */
    int16_t brojKanala;
    uint32_t jacinaZvuka;
    bool teletekst;
    int16_t audioPID;
    int16_t videoPID;
    char *trenutnoNaProgramu;
    uint8_t trajanjeSati;
    uint8_t trajanjeMinuta;
} IsrtavanjeGrafikeInfo;

typedef enum _povratneVrednostiGrafika
{
    GRAFIKA_USPEH = 0,
    GRAFIKA_GRESKA,
    GRAFIKA_GRESKA_NIT
} PovratnaVrednostGrafika;

#define DFBCHECK(x...)                                           \
    {                                                            \
        DFBResult err = x;                                       \
                                                                 \
        if (err != DFB_OK)                                       \
        {                                                        \
            fprintf(stderr, "%s <%d>:\n\t", __FILE__, __LINE__); \
            DirectFBErrorFatal(#x, err);                         \
        }                                                        \
    }

#define MAX_VOLUME 9
#define MIN_VOLUME 0

/* Deklaracije funkcija */
PovratnaVrednostGrafika inicijalizacijaGrafike();
PovratnaVrednostGrafika deinicijalizacijaGrafike();

/* Grafika, kontrole iscrtavanja */
void ugasiIspisBrojKanala();
void ugasiIspisJacinaZvuka();
void ugasiIspsiInformacija();
void upaliRadio();
void ugasiRadio();
void obrisiEkran();

/* Grafika, popunjavanje i prikaz elemenata */
void prikaziInfo(int16_t brojKanala, bool teletekst, int16_t audioPID, int16_t videoPID, char *trenutnoNaProgramu, int32_t trajanjeSekundi);
void prikaziBrojKanala(int16_t brojKanala);
void prikaziJacineZvuka(uint8_t jacinaZvuka);

/* Kontrole plejera, zvuk */
uint32_t pojacaj();
uint32_t smanji();
uint32_t postaviJacinuZvuka(uint32_t jacinaZvuka);

/* Funkcija koja se pokreće u niti zadužena za grafiku, DirectFB */
void *render();

#endif /* __ISCRTAVANJE_GRAFIKE_H__ */

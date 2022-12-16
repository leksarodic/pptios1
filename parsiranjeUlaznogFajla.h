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

#ifndef __PARSIRANJE_FAJLA_H__
#define __PARSIRANJE_FAJLA_H__

#include <stdint.h>

typedef struct _konfiguracioniFajl
{
    uint32_t tuneFrequency;
    uint32_t bandwidth;
    uint8_t modul;
    uint32_t audioPID;
    uint32_t videoPID;
    uint8_t audioType;
    uint8_t videoType;
    uint8_t programNumber;
} KonfiguracioniFajl;

typedef enum _povratneVrednostiParsiranje
{
    PARSIRANJE_USPEH = 0,
    PARSIRANJE_GRESKA
} PovratnaVrednostParsiranje;

PovratnaVrednostParsiranje parsiranjeKonfigFajla(char *imeFajla, KonfiguracioniFajl *konfigFajl);
PovratnaVrednostParsiranje ispisKonfigFajl(KonfiguracioniFajl *konfigFajl);

#endif /* __PARSIRANJE_FAJLA_H__ */
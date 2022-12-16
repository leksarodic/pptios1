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
* Primer konfiguracionog fajla
*   818000000; Tune freq. in Hz
*   8; Bandwidth in Mhz
*   0; 0=DVB-T, 1=DVB-T2
*   101; AudioPID
*   102; VideoPID
*   0; Audio type
*   0; Video type
*   0; Program number
*
* Kreirano marta 2019.
*
* @Author Aleksa Rodić
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "parsiranjeUlaznogFajla.h"

#define BROJ_PARAMETARA_FAJLA 8
#define LOG_ISPIS 0

PovratnaVrednostParsiranje parsiranjeKonfigFajla(char imeFajla[], KonfiguracioniFajl *konfigFajl)
{
    int32_t i;

    FILE *stream;
    char *line = NULL;
    uint32_t len = 0;
    uint32_t read;

    uint8_t brojProcitanihVrednosti = 0;

    uint32_t dobavljeneVrednostiRedom[BROJ_PARAMETARA_FAJLA];

    stream = fopen(imeFajla, "r");
    if (stream == NULL)
        return PARSIRANJE_GRESKA;

    while ((read = getline(&line, &len, stream)) != -1)
    {
#if LOG_ISPIS
        printf("Retrieved line of length %d :\n", read);
        printf("%s", line);
#endif

        sscanf(line, "%d", &dobavljeneVrednostiRedom[brojProcitanihVrednosti++]);
    }

#if LOG_ISPIS
    printf("Broj procitanih linija = %d\n", brojProcitanihVrednosti);
#endif
    if (brojProcitanihVrednosti != BROJ_PARAMETARA_FAJLA)
    {
        printf("Problem sa konfiguracionim fajlom. Pogledaj primer!\n");
        return PARSIRANJE_GRESKA;
    }

#if LOG_ISPIS
    for (i = 0; i < brojProcitanihVrednosti; i++)
    {
        printf("[%d] = %d\n", i, dobavljeneVrednostiRedom[i]);
    }
#endif

    free(line);
    fclose(stream);

    konfigFajl->tuneFrequency = dobavljeneVrednostiRedom[0];
    konfigFajl->bandwidth = dobavljeneVrednostiRedom[1];
    konfigFajl->modul = dobavljeneVrednostiRedom[2];
    konfigFajl->audioPID = dobavljeneVrednostiRedom[3];
    konfigFajl->videoPID = dobavljeneVrednostiRedom[4];
    konfigFajl->audioType = dobavljeneVrednostiRedom[5];
    konfigFajl->videoType = dobavljeneVrednostiRedom[6];
    konfigFajl->programNumber = dobavljeneVrednostiRedom[7];

    return PARSIRANJE_USPEH;
}

PovratnaVrednostParsiranje ispisKonfigFajl(KonfiguracioniFajl *konfigFajl)
{
    if (konfigFajl == NULL)
    {
        return PARSIRANJE_GRESKA;
    }

    printf("tuneFrequency = %d\n", konfigFajl->tuneFrequency);
    printf("bandwidth = %d\n", konfigFajl->bandwidth);
    printf("modul = %d\n", konfigFajl->modul);
    printf("audioPID = %d\n", konfigFajl->audioPID);
    printf("videoPID = %d\n", konfigFajl->videoPID);
    printf("audioType = %d\n", konfigFajl->audioType);
    printf("videoType = %d\n", konfigFajl->videoType);
    printf("programNumber = %d\n", konfigFajl->programNumber);

    return PARSIRANJE_USPEH;
}

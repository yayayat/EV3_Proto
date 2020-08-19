#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <math.h>

#define HEX_OUTPUT

uint8_t mas[200];
uint8_t pack[34];
uint16_t n = 0;

using namespace std;

uint8_t hex_decode(const uint8_t *in)
{
    uint8_t out;
    unsigned int i, t, hn, ln;
    hn = in[0] > '9' ? in[0] - 'A' + 10 : in[0] - '0';
    ln = in[1] > '9' ? in[1] - 'A' + 10 : in[1] - '0';
    out = (hn << 4) | ln;
    return out;
}

struct message
{
    uint8_t header;
    uint8_t kind, cmd, mode;
    //!  00        SYS
    //*    000000    SYNC
    //*    000010    NACK
    //*    000100    ACK
    //*    xxx110    ESC (reserved)
    //!  01        CMD
    //     LLL
    //*    |||000    TYPE
    //*    |||001    MODES
    //*    |||010    SPEED
    //*    |||011    SELECT
    //*    |||100    WRITE
    //*    |||101    RSRV
    //*    |||110    RSRV
    //*    |||111    RSRV
    //     |||
    //!  10|||     INFO
    //     LLLMMM
    //!  11||||||  DATA
    //     LLLMMM
    //     ||||||
    //*    000|||    Message payload is 1 byte not including command byte and check byte
    //*    001|||    Message payload is 2 bytes not including command byte and check byte
    //*    010|||    Message payload is 4 bytes not including command byte and check byte
    //*    011|||    Message payload is 8 bytes not including command byte and check byte
    //*    100|||    Message payload is 16 bytes not including command byte and check byte
    //*    101|||    Message payload is 32 bytes not including command byte and check byte
    //        |||
    //*       000    Mode 0 default
    //*       001    Mode 1
    //*       010    Mode 2
    //*       011    Mode 3
    //*       100    Mode 4
    //*       101    Mode 5
    //*       110    Mode 6
    //*       111    Mode 7
    uint8_t length;
    uint8_t *payload;
    bool crcPassed;
    uint8_t crc;
    uint8_t calculatedCrc;
};

int main(int argc, const char *argv[])
{
    FILE *pFile;
    long lSize;
    uint8_t *buffer;
    size_t result;
    pFile = fopen("hex/x8l.txt", "rb");
    if (pFile == NULL)
    {
        fputs("File error", stderr);
        exit(1);
    }

    //obtain file size:
    fseek(pFile, 0, SEEK_END);
    lSize = ftell(pFile);
    rewind(pFile);

    //allocate memory to contain the whole file:
    buffer = (uint8_t *)malloc(sizeof(uint8_t) * lSize);
    if (buffer == NULL)
    {
        fputs("Memory error", stderr);
        exit(2);
    }

    // copy the file into the buffer:
    result = fread(buffer, 1, lSize, pFile);
    if (result != lSize)
    {
        fputs("Reading error", stderr);
        exit(3);
    }
    // the whole file is now loaded in the memory buffer.
    vector<uint8_t> input(buffer, buffer + lSize);

    for (uint32_t i = 0; i < input.size() - 2; i += 2)
    {
        while (input[i] == '\n')
            input.erase(input.begin() + i);
        while (input[i + 2] != '\n')
            input.erase(input.begin() + i + 2);
        input.erase(input.begin() + i + 2);
    }

    vector<message> messages;
    for (int c = 0; c < 1000; c++)
    {
        if (input.size() < 2)
            break;
        static uint8_t crc;
        static message mes;
        static int8_t stage = -1, len = 0;
        uint8_t hex = hex_decode(input.data());
        if (input.size() >= 2)
            input.erase(input.begin(), input.begin() + 2);
        if (stage < 0)
        {
            crc = 0xFF;
            mes.header = hex;
            if (mes.header & 0xC0)
            {
                stage = len = mes.length = 1 << ((mes.header & 0x38) >> 3);
                mes.payload = (uint8_t *)malloc(sizeof(uint8_t) * len);
                if ((mes.header & 0xC0) == 0x80)
                {
                    crc ^= (uint8_t)hex;
                    hex = hex_decode(input.data());
                    if (input.size() >= 2)
                        input.erase(input.begin(), input.begin() + 2);
                    mes.cmd = hex;
                }
            }
            else
                mes.length = 0;
        }
        else if (stage == 0)
        {
            mes.crc = hex;
            mes.calculatedCrc = crc;
            mes.crcPassed = ((uint8_t)hex == (uint8_t)crc);
            messages.push_back(mes);
            stage--;
        }
        else
        {
            mes.payload[len - stage] = hex;
            stage--;
        }
        crc ^= (uint8_t)hex;
    }

    for (uint8_t i = 0; i < messages.size(); i++)
    {
#ifdef HEX_OUTPUT
        printf("#%d ", i);
        printf("Header: 0x%02X ", (uint8_t)messages[i].header);
        printf("Length: %d ", messages[i].length);
        printf("Payload: ");
        for (int8_t j = 0; j < messages[i].length; j++)
            printf("0x%02X ", (uint8_t)messages[i].payload[j]);
        printf("CRC: (calc/read:0x%02X/0x%02X) %s\n", messages[i].calculatedCrc, messages[i].crc, messages[i].crcPassed ? "Passed!" : "Error!");
#endif
        printf("[");
        switch (messages[i].header & 0xC0)
        {
        case 0x00:
            printf("SYS");
            switch (messages[i].header & 0x07)
            {
            case 0x00:
                printf(" SYNC");
                break;
            case 0x02:
                printf(" NACK");
                break;
            case 0x04:
                printf(" ACK");
                break;
            }
            break;
        case 0x40:
            printf("CMD");
            switch (messages[i].header & 0x07)
            {
            case 0x00:
                printf(" TYPE: 0x%X", (uint8_t)*messages[i].payload);
                break;
            case 0x01:
                printf(" MODES:");
                if (messages[i].length == 2)
                {
                    if (messages[i].payload[0])
                        printf(" 0-%d modes used", messages[i].payload[0]);
                    else
                        printf(" 0 mode only used");
                    if (messages[i].payload[1])
                        printf(" (0-%d modes in view and data log)", messages[i].payload[1]);
                    else
                        printf(" (0 mode only in view and data log)");
                }
                else
                {
                    if (messages[i].payload[0])
                        printf(" 0-%d modes in view and data log", messages[i].payload[0]);
                    else
                        printf(" 0 mode only in view and data log");
                }
                break;
            case 0x02:
                printf(" SPEED: %d boad", *(uint32_t *)messages[i].payload);
                break;
            case 0x03:
                printf(" SELECT");
                break;
            case 0x04:
                printf(" WRITE");
                break;
            }
            break;
        case 0x80:
            printf("INFO Mode %d ", messages[i].header & 0x07);
            switch (messages[i].cmd)
            {
            case 0x00:
                printf(" NAME: %s", messages[i].payload);
                break;
            case 0x01:
                printf(" RAW: %g-%g", *(float *)messages[i].payload, *(float *)(&messages[i].payload[4]));
                break;
            case 0x02:
                printf(" PCT: %g-%g", *(float *)messages[i].payload, *(float *)(&messages[i].payload[4]));
                break;
            case 0x03:
                printf(" SI: %g-%g", *(float *)messages[i].payload, *(float *)(&messages[i].payload[4]));
                break;
            case 0x04:
                printf(" SYMBOL %s", messages[i].payload);
                break;
            case 0x80:
                printf(" FORMAT: %d of ", *messages[i].payload);
                switch (messages[i].payload[1])
                {
                case 0x00:
                    printf(" DATA8");
                    break;
                case 0x01:
                    printf(" DATA16");
                    break;
                case 0x02:
                    printf(" DATA32");
                    break;
                case 0x03:
                    printf(" DATAF");
                    break;
                }
                if (messages[i].length > 2)
                    printf(", %d figure, %d decimals", messages[i].payload[2], messages[i].payload[3]);
                break;
            }
            break;
        case 0xC0:
            printf("DATA Mode%d", messages[i].header & 0x07);
            break;
        }
        printf("]\n\n");
    }
    // terminate
    fclose(pFile);
    free(buffer);
    return 0;
}
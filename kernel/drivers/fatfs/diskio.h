/*-----------------------------------------------------------------------*/
/* diskio.h - Interface entre FatFS e drivers de armazenamento           */
/*                                                                       */
/* Projeto: UpdateOS                                                     */
/* Local:   kernel/drivers/fatfs/diskio.h                                */
/*-----------------------------------------------------------------------*/
#ifndef _DISKIO_DEFINED
#define _DISKIO_DEFINED

#include "ff.h"

/* Tipo de status do dispositivo */
typedef BYTE DSTATUS;

/* Resultado das funções de disco */
typedef enum {
    RES_OK = 0,     /* Sucesso */
    RES_ERROR,      /* Erro de leitura/escrita */
    RES_WRPRT,      /* Protegido contra escrita */
    RES_NOTRDY,     /* Dispositivo não pronto */
    RES_PARERR      /* Parâmetro inválido */
} DRESULT;

/* Bits de status do dispositivo (DSTATUS) */
#define STA_NOINIT      0x01    /* Não inicializado */
#define STA_NODISK      0x02    /* Sem mídia */
#define STA_PROTECT     0x04    /* Protegido contra escrita */

/* Comandos genéricos do disk_ioctl() */
#define CTRL_SYNC           0   /* Conclui operações pendentes */
#define GET_SECTOR_COUNT    1   /* Nº total de setores (f_mkfs) */
#define GET_SECTOR_SIZE     2   /* Tamanho do setor (FF_MIN_SS < FF_MAX_SS) */
#define GET_BLOCK_SIZE      3   /* Tamanho do bloco de apagamento (f_mkfs) */
#define CTRL_TRIM           4   /* Avisa que setores foram liberados (FF_USE_TRIM) */

/* Protótipos das funções exigidas pelo FatFS */
DSTATUS disk_initialize (BYTE pdrv);
DSTATUS disk_status     (BYTE pdrv);
DRESULT disk_read       (BYTE pdrv, BYTE *buff, LBA_t sector, UINT count);
#if FF_FS_READONLY == 0
DRESULT disk_write      (BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count);
#endif
DRESULT disk_ioctl      (BYTE pdrv, BYTE cmd, void *buff);

#endif /* _DISKIO_DEFINED */
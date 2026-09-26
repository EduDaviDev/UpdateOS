/*-----------------------------------------------------------------------*/
/* diskio.c - Camada de baixo nível do FatFS para o UpdateOS             */
/*                                                                       */
/* Mapeia os drives físicos do FatFS para os discos ATA PIO detectados   */
/* pelo driver em kernel/drivers/atapio.c.                               */
/*                                                                       */
/*   pdrv 0 -> ATA drive 0 (master primário)                             */
/*   pdrv 1 -> ATA drive 1 (slave  primário)                             */
/*   pdrv 2 -> ATA drive 2 (master secundário)                           */
/*   pdrv 3 -> ATA drive 3 (slave  secundário)                           */
/*                                                                       */
/* Ajuste FF_VOLUMES no ffconf.h para o número de discos que você usa.   */
/*-----------------------------------------------------------------------*/
#include "diskio.h"
#include "ff.h"
#include "../atapio.h" /* sobe um nível: fatfs/ -> drivers/ */
#include "../rtc.h"

/*-----------------------------------------------------------------------*/
/* Inicializa o disco                                                    */
/*                                                                       */
/* Em um sistema com ATA, o driver já detectou os discos no boot via     */
/* ata_init(). Aqui apenas confirmamos presença e reportamos o estado.   */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize(BYTE pdrv)
{
	if (pdrv >= 4)
		return STA_NOINIT;
	if (!ata_present(pdrv))
		return STA_NOINIT;
	return 0;
}

/*-----------------------------------------------------------------------*/
/* Retorna o status atual do disco                                       */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status(BYTE pdrv)
{
	if (pdrv >= 4)
		return STA_NOINIT;
	if (!ata_present(pdrv))
		return STA_NOINIT;
	return 0;
}

/*-----------------------------------------------------------------------*/
/* Lê setores do disco                                                   */
/*                                                                       */
/* LBA_t e UINT dependem do ffconf.h:                                    */
/*   - FF_LBA64 = 0 -> LBA_t = DWORD                                     */
/*   - FF_LBA64 = 1 -> LBA_t = QWORD                                     */
/* O driver ATA PIO atual suporta LBA28, então convertemos para DWORD.   */
/*-----------------------------------------------------------------------*/
DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
	if (pdrv >= 4 || !buff)
		return RES_PARERR;
	if (count == 0)
		return RES_PARERR;
	if (count > 255)
		return RES_PARERR; /* limite do ATA PIO */

	/* Guarda contra overflow em LBA28 (máx. 2^28 setores) */
	if ((uint64_t)sector + count > 0x10000000ULL)
		return RES_PARERR;

	if (!ata_present(pdrv))
		return RES_NOTRDY;

	if (ata_read_sectors(pdrv, (uint32_t)sector, (uint8_t)count, buff) != 0)
		return RES_ERROR;

	return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Escreve setores no disco                                              */
/*                                                                       */
/* Só é compilado quando FF_FS_READONLY == 0.                            */
/*-----------------------------------------------------------------------*/
#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
	if (pdrv >= 4 || !buff)
		return RES_PARERR;
	if (count == 0)
		return RES_PARERR;
	if (count > 255)
		return RES_PARERR;

	if ((uint64_t)sector + count > 0x10000000ULL)
		return RES_PARERR;

	if (!ata_present(pdrv))
		return RES_NOTRDY;

	if (ata_write_sectors(pdrv, (uint32_t)sector, (uint8_t)count, buff) != 0)
		return RES_ERROR;

	return RES_OK;
}
#endif

/*-----------------------------------------------------------------------*/
/* Comandos de controle                                                  */
/*                                                                       */
/*   CTRL_SYNC         -> flush de cache (não usado no ATA PIO)          */
/*   GET_SECTOR_COUNT  -> capacidade total em setores (f_mkfs)           */
/*   GET_SECTOR_SIZE   -> tamanho do setor em bytes                      */
/*   GET_BLOCK_SIZE    -> tamanho do bloco de apagamento (f_mkfs)        */
/*   CTRL_TRIM         -> aviso de setores liberados (FF_USE_TRIM)       */
/*-----------------------------------------------------------------------*/
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
	if (pdrv >= 4)
		return RES_PARERR;
	if (!ata_present(pdrv))
		return RES_NOTRDY;

	switch (cmd)
	{
	case CTRL_SYNC:
		/* ATA PIO já conclui cada escrita de forma síncrona, com flush
		   ao final. Não há cache a sincronizar. */
		return RES_OK;

	case GET_SECTOR_COUNT:
		if (!buff)
			return RES_PARERR;
		*(DWORD *)buff = ata_get_sector_count(pdrv);
		return RES_OK;

	case GET_SECTOR_SIZE:
		if (!buff)
			return RES_PARERR;
		*(WORD *)buff = ATA_SECTOR_SIZE; /* 512 bytes */
		return RES_OK;

	case GET_BLOCK_SIZE:
		if (!buff)
			return RES_PARERR;
		*(DWORD *)buff = 1; /* 1 = não é flash */
		return RES_OK;

#if FF_USE_TRIM
	case CTRL_TRIM:
		/* O driver ATA PIO não implementa TRIM; aceita silenciosamente. */
		return RES_OK;
#endif

	default:
		return RES_PARERR;
	}
}
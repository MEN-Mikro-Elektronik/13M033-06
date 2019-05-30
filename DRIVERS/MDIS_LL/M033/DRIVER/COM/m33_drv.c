/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: m33_drv.c
 *      Project: M33 module LL-driver (MDIS5)
 *
 *       Author: ds
 *
 *  Description: MDIS low level driver for M33 module
 *
 *     Required:  ---
 *     Switches:  _ONE_NAMESPACE_PER_DRIVER_
 *
 *---------------------------------------------------------------------------
 * Copyright (c) 1998-2019, MEN Mikro Elektronik GmbH
 ****************************************************************************/
/*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <MEN/mdis_com.h>    /* info function codes            */
#include <MEN/men_typs.h>   /* system dependend definitions   */
#include <MEN/dbg.h>        /* debug functions                */
#include <MEN/oss.h>        /* os services                    */
#include <MEN/mdis_err.h>   /* mdis error numbers             */
#include <MEN/maccess.h>    /* hw access macros and types     */
#include <MEN/desc.h>       /* descriptor functions           */
#include <MEN/mdis_api.h>   /* global set/getstat codes          */
#include <MEN/modcom.h>     /* id prom access                 */

#include <MEN/ll_defs.h>    /* low level driver definitions   */
#include <MEN/ll_entry.h>    /* low level driver entry struct  */
#include <MEN/m33_drv.h>    /* M33 driver header file          */

static const char IdentString[]=MENT_XSTR(MAK_REVISION);

/*-----------------------------------------+
|  DEFINES                                 |
+------------------------------------------*/
#define CH_NUMBER            8            /* nr of device channels */
#define ADDRSPACE_COUNT        1            /* nr of required address spaces */
#define MOD_ID_MAGIC        0x5346      /* eeprom identification (magic) */
#define MOD_ID_SIZE            128            /* eeprom size */
#define MOD_ID                33            /* module id */
#define M33_DELAY            20            /* M33_DELAY for reset [msec] */

/* debug settings */
#define DBG_MYLEVEL        m33Hdl->dbgLevel
#define DBH             m33Hdl->dbgHdl

/*--------------------- m33 register defs/offsets ------------------------*/
/* channel registers (ch=0..7) */
#define M33_SETMOD_REG(ch)  0x00+((ch)<<4)        /* load+set mode (range) */
#define M33_SETVAL_REG(ch)  0x02+((ch)<<4)        /* load+set value (out) */
#define M33_LOADVAL_REG(ch) 0x06+((ch)<<4)        /* load value (out) */

/* common registers */
#define M33_SETALL_REG      0x0a                /* update all load values */
#define M33_RESET_REG       0xfe                /* reset all channels */

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/

/*-------------- m33 channel type (for block read/write) -----------------*/
typedef int16 CH_TYPE;

typedef struct
{
    MDIS_IDENT_FUNCT_TBL idFuncTbl;            /* id function table */
    u_int32         ownMemSize;
    OSS_HANDLE        *osHdl;
    MACCESS            ma;
    OSS_SEM_HANDLE    *devSem;
    u_int32            useId;
    int16            setmod[2];                /* mode shadow registers */
    u_int16            ch_opt[CH_NUMBER];
    /* debug */
    u_int32         dbgLevel;    /* debug level */
    DBG_HANDLE      *dbgHdl;    /* debug handle */
} M33_HANDLE;

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
static char* Ident( void );

static int32 M33_Init(
                DESC_SPEC *descSpec, OSS_HANDLE *OsHdl, MACCESS *ma,
                OSS_SEM_HANDLE *DevSem, OSS_IRQ_HANDLE *irqHandle, LL_HANDLE **LLDat);

static int32 M33_Exit(
                LL_HANDLE **LLDat);

static int32 M33_Read(
                LL_HANDLE *LLDat, int32 ch, int32 *value);

static int32 M33_Write(
                LL_HANDLE *LLDat, int32 ch, int32 value );

static int32 M33_SetStat(
                LL_HANDLE *LLDat, int32 code, int32 ch, INT32_OR_64 value32_or_64 );

static int32 M33_GetStat(
                LL_HANDLE *LLDat, int32 code, int32 ch, INT32_OR_64 *value32_or_64P );

static int32 M33_BlockRead(
                LL_HANDLE *LLDat, int32 ch, void *buf, int32 size, int32 *nbrRdBytesP);

static int32 M33_BlockWrite(
                LL_HANDLE *LLDat, int32 ch, void *buf, int32 size, int32 *nbrWrBytesP);

static int32 M33_Irq(
                LL_HANDLE *LLDat);

static int32 M33_Info(
                int32 infoType, ...);


/**************************** M33_GetEntry **********************************
 *
 *  Description:  Gets the entry points of the low-level driver functions.
 *
 *         Note:  Is used by MDIS kernel only.
 *
 *---------------------------------------------------------------------------
 *  Input......:  ---
 *
 *  Output.....:  drvP  pointer to the initialized structure
 *
 *  Globals....:  ---
 *
 ****************************************************************************/

#ifdef _ONE_NAMESPACE_PER_DRIVER_
    void LL_GetEntry( LL_ENTRY* drvP )
#else
    void M33_GetEntry( LL_ENTRY* drvP )
#endif /* _ONE_NAMESPACE_PER_DRIVER_ */
{
    drvP->init            = M33_Init;
    drvP->exit            = M33_Exit;
    drvP->read            = M33_Read;
    drvP->write            = M33_Write;
    drvP->blockRead        = M33_BlockRead;
    drvP->blockWrite    = M33_BlockWrite;
    drvP->setStat        = M33_SetStat;
    drvP->getStat        = M33_GetStat;
    drvP->irq            = M33_Irq;
    drvP->info            = M33_Info;
}


/******************************** M33_Init ***********************************
 *
 *  Description:  Allocates and initialize the LL structure
 *                  Decodes descriptor
 *                  Reads and checks the ID if in descriptor enabled
 *                  Initialize the hardware
 *                    - reset module ( all channels to 0V / 0(4)mA )
 *                    - sets all predefined ranges (channel options)
 *                The driver supports 8 analog output channels.
 *
 *                Deskriptor key    Default           Range/Unit
 *                  ------------------------------------------------------------
 *                DEBUG_LEVEL       OSS_DBG_DEFAULT  see dbg.h
 *                DEBUG_LEVEL_DESC  OSS_DBG_DEFAULT  see dbg.h
 *                ID_CHECK          0                0..1  0 - disabled
 *
 *                  CHANNEL_%d/       0                value  U[V]    I[mA]
 *                   M33_CH_RANGE                      -------------------------
 *                (%d = 0..7)                        0     0..+10   0(4)..+20
 *                                                   1    -5..+5    prohibited
 *                                                   3   -10..+10   prohibited
 *
 *----------------------------------------------------------------------------
 *  Input......:  descSpec        descriptor specifier
 *                osHdl            pointer to the os specific structure
 *                maHdl            access handle
 *                                (in simplest case module base address)
 *                devSemHdl        device semaphore for unblocking in wait
 *                irqHdl        irq handle for mask and unmask interrupts
 *                llHdlP        pointer to the variable where low level driver
 *                                handle will be stored
 *
 *  Output.....:  llHdlP        pointer to low level driver handle
 *                return        0 | error code
 *
 *  Globals....:  ---
 ****************************************************************************/
static int32 M33_Init(
    DESC_SPEC        *descSpec,
    OSS_HANDLE      *osHdl,
    MACCESS            *maHdl,
    OSS_SEM_HANDLE  *devSem,
    OSS_IRQ_HANDLE  *irqHdl,
    LL_HANDLE        **llHdlP)
{
    M33_HANDLE        *m33Hdl;
    int32            status;
    u_int32            gotsize;
    DESC_HANDLE        *descHdlP;
    int32            modIdMagic;
    int32            modId;
    u_int32            value;
    register int16     range, mod, ch;

    /*----------------------------+
    | initialize the LL structure |
    +-----------------------------*/
    /* get memory for the LL structure */
    m33Hdl = (M33_HANDLE*) OSS_MemGet(
                    osHdl, sizeof(M33_HANDLE), &gotsize );
    if (m33Hdl == NULL)    return ERR_OSS_MEM_ALLOC;

    /* set LL handle to the LL structure */
    *llHdlP = (LL_HANDLE*)m33Hdl;

    /* store data into the LL structure */
    m33Hdl->ownMemSize    = gotsize;
    m33Hdl->osHdl        = osHdl;
    m33Hdl->ma            = *maHdl;
    m33Hdl->devSem        = devSem;

    /*------------------------------+
    |  init id function table       |
    +------------------------------*/
     /* drivers ident function */
     m33Hdl->idFuncTbl.idCall[0].identCall = Ident;
     /* libraries ident functions */
     m33Hdl->idFuncTbl.idCall[1].identCall = DESC_Ident;
     m33Hdl->idFuncTbl.idCall[2].identCall = OSS_Ident;
     /* terminator */
     m33Hdl->idFuncTbl.idCall[3].identCall = NULL;

    /*------------------------------+
    |  prepare debugging            |
    +------------------------------*/
    DBG_MYLEVEL = OSS_DBG_DEFAULT;    /* set OS specific debug level */
    DBGINIT((NULL,&DBH));

    /*--------------------------------+
    | get data from the LL descriptor |
    +--------------------------------*/
    /* init descHdl */
    status = DESC_Init(    descSpec, osHdl, &descHdlP );
    if (status) {
        DBGEXIT((&DBH));
        OSS_MemFree( osHdl, (char*)m33Hdl, gotsize);
        return status;
    }

    /* get DEBUG_LEVEL_DESC */
    status = DESC_GetUInt32( descHdlP, OSS_DBG_DEFAULT, &value,
                "DEBUG_LEVEL_DESC");
    if ( (status!=0) && (status!=ERR_DESC_KEY_NOTFOUND) ) {
        DESC_Exit( &descHdlP );
        DBGEXIT((&DBH));
        OSS_MemFree( osHdl, (char*)m33Hdl, gotsize);
        return status;
    }
    DESC_DbgLevelSet(descHdlP, value);    /* set level */

    /* get DEBUG_LEVEL */
    status = DESC_GetUInt32( descHdlP, OSS_DBG_DEFAULT, &(m33Hdl->dbgLevel),
                "DEBUG_LEVEL");
    if ( (status!=0) && (status!=ERR_DESC_KEY_NOTFOUND) ) {
        DESC_Exit( &descHdlP );
        DBGEXIT((&DBH));
        OSS_MemFree( osHdl, (char*)m33Hdl, gotsize);
        return status;
    }

    DBGWRT_1((DBH, "LL - M33_Init\n"));

    /* get ID_CHECK */
    status = DESC_GetUInt32( descHdlP, 1, &(m33Hdl->useId), "ID_CHECK");
    if ( (status!=0) && (status!=ERR_DESC_KEY_NOTFOUND) ) {
        DESC_Exit( &descHdlP );
        DBGEXIT((&DBH));
        OSS_MemFree( osHdl, (char*)m33Hdl, gotsize);
        return status;
    }

    /* get CHANNEL_xx/M33_CH_RANGE */
    for( ch=0; ch<CH_NUMBER; ch++){
        status = DESC_GetUInt32( descHdlP, 0,
            (u_int32*)(&(m33Hdl->ch_opt[ch])), "CHANNEL_%d/M33_CH_RANGE", ch);
        if ( (status!=0) && (status!=ERR_DESC_KEY_NOTFOUND) ) {
            DESC_Exit( &descHdlP );
            DBGEXIT((&DBH));
            OSS_MemFree( osHdl, (char*)m33Hdl, gotsize);
            return status;
        }
        /* check range (only 0, 1, 3 is valid) */
        if( (m33Hdl->ch_opt[ch] > 3) || (m33Hdl->ch_opt[ch] == 2) ){
            DESC_Exit( &descHdlP );
            DBGEXIT((&DBH));
            OSS_MemFree( osHdl, (char*)m33Hdl, gotsize);
            return ERR_LL_DESC_PARAM;
        }
    }

    /* exit descHdl */
    status = DESC_Exit( &descHdlP );
    if (status) {
        DBGEXIT((&DBH));
        OSS_MemFree( osHdl, (char*)m33Hdl, gotsize);
        return status;
    }

    if(m33Hdl->useId){
        /*--------------------------------+
        | check module Id                  |
        +--------------------------------*/
        modIdMagic = m_read((U_INT32_OR_64)m33Hdl->ma, 0);
        modId      = m_read((U_INT32_OR_64)m33Hdl->ma, 1);

        if( modIdMagic != MOD_ID_MAGIC ) {
            DBGWRT_ERR((DBH,"*** LL - M33_Init: illegal magic=0x%04x\n", modIdMagic));
            status = ERR_LL_ILL_ID;
            DBGEXIT((&DBH));
            OSS_MemFree( osHdl, (char*)m33Hdl, gotsize);
            return status;
        }

        if( modId != MOD_ID ) {
            DBGWRT_ERR((DBH,"*** LL - M33_Init: illegal id=%d\n", modId));
            status = ERR_LL_ILL_ID;
            DBGEXIT((&DBH));
            OSS_MemFree( osHdl, (char*)m33Hdl, gotsize);
            return status;
        }
    }

   /*----------------------------------------+
   |  reset all channels to 0V / 0(4)mA      |
   +----------------------------------------*/
   MWRITE_D16(m33Hdl->ma, M33_RESET_REG, 0x00);
   OSS_Delay(m33Hdl->osHdl, M33_DELAY);            /* M33_DELAY for the optocoupler */
   MWRITE_D16(m33Hdl->ma, M33_RESET_REG, 0x01);
   OSS_Delay(m33Hdl->osHdl, M33_DELAY);            /* M33_DELAY for the optocoupler */

   /*-------------------------------------------+
   |  calculate and set modes of channels 0..3  |
   +-------------------------------------------*/
   for (mod=0, ch=0; ch<4; ch++)  {                    /* for channel 0..3: */
      range = m33Hdl->ch_opt[ch];                    /* get range */
      mod <<= 1;                                    /* shift mode */
      mod |= (range & 1) | ((range & 2)<<3);        /* set range bits */
   }

    m33Hdl->setmod[0] = (int16)((mod<<8) & 0xff00);
    //m33Hdl->setmod[0] = (mod<<8) & 0xff00;
    MWRITE_D16(m33Hdl->ma, M33_SETMOD_REG(0), m33Hdl->setmod[0]);    /* set ranges 0..3 */

   /*-------------------------------------------+
   |  calculate and set modes of channels 4..7  |
   +-------------------------------------------*/
   for (mod=0, ch=4; ch<8; ch++)  {                    /* for channel 4..7: */
      range = m33Hdl->ch_opt[ch];                    /* get range */
      mod <<= 1;                                    /* shift mode */
      mod |= (range & 1) | ((range & 2)<<3);        /* set range bits */
   }

       m33Hdl->setmod[1] = (int16)((mod<<8) & 0xff00);
    MWRITE_D16(m33Hdl->ma, M33_SETMOD_REG(4), m33Hdl->setmod[1]);    /* set ranges 4..7 */

    return 0;

}/*M33_init*/


/****************************** M33_Exit *************************************
 *
 *  Description:  Deinitializes the hardware
 *                    - resets all ranges
 *                    - reset module ( all channels to 0V / 0(4)mA )
 *                  Frees allocated memory
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdlP        pointer to low level driver handle
 *
 *  Output.....:  llHdlP        NULL
 *                return        0 | error code
 *
 *  Globals....:  ---
 ****************************************************************************/
static int32 M33_Exit(
    LL_HANDLE    **llHdlP)
{
    M33_HANDLE*     m33Hdl = (M33_HANDLE*) *llHdlP;
    int32            status;

    DBGWRT_1((DBH, "LL - M33_Exit\n"));

    /*------------------------------------+
    |  reset all channels to null volt    |
    +------------------------------------*/
    MWRITE_D16(m33Hdl->ma, M33_RESET_REG, 0x00);
    OSS_Delay(m33Hdl->osHdl, M33_DELAY);            /* M33_DELAY for the optocoupler */
    MWRITE_D16(m33Hdl->ma, M33_RESET_REG, 0x01);
    OSS_Delay(m33Hdl->osHdl, M33_DELAY);            /* M33_DELAY for the optocoupler */

    DBGEXIT((&DBH));
    /* free memory for the LL structure */
    status = OSS_MemFree( m33Hdl->osHdl, (int8*) m33Hdl, m33Hdl->ownMemSize );

    return(status);

}/*M33_exit*/


/****************************** M33_Read *************************************
 *
 *  Description:  (unused)
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl        pointer to low-level driver data structure
 *                ch        current channel
 *
 *  Output.....:  valueP    read value
 *                return    0 | error code
 *
 *  Globals....:  ---
 ****************************************************************************/
static int32 M33_Read(
    LL_HANDLE    *llHdl,
    int32        ch,
    int32        *valueP)
{
    DBGCMD(M33_HANDLE*       m33Hdl = (M33_HANDLE*) llHdl);

    DBGWRT_1((DBH, "LL - M33_Read: ch=%d\n",ch));

    return(ERR_LL_ILL_FUNC);

}/*M33_read*/


/****************************** M33_Write ************************************
 *
 *  Description:  Writing to channel 'ch' on module
 *                - sets output value (12 bits left-aligned)
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl        pointer to low-level driver data structure
 *                ch        current channel (0..7)
 *                value        0x0000..0xFFF0
 *
 *  Output.....:  return    0 | error code
 *
 *  Globals....:  ---
 ****************************************************************************/
static int32 M33_Write(
    LL_HANDLE    *llHdl,
    int32        ch,
    int32        value)
{
    M33_HANDLE*       m33Hdl = (M33_HANDLE*) llHdl;

    DBGWRT_1((DBH, "LL - M33_Write: ch=%d\n",ch));

    /* load+set new value */
    MWRITE_D16(m33Hdl->ma, M33_SETVAL_REG(ch), (int16)value );

    return(0);

}/*M33_write*/


/******************************** M33_SetStat *******************************
 *
 *  Description:  Changes the device state.
 *
 *        common codes            values            meaning
 *        ---------------------------------------------------------------------
 *        M_LL_DEBUG_LEVEL        see dbg.h       enable/disable debug output
 *      M_LL_CH_DIR             M_CH_OUT        channel direction
 *
 *        M33 specific codes        values          meaning
 *        ---------------------------------------------------------------------
 *        M33_CH_RANGE            0, 1, 3         set output range
 *
 *                                                value  U[V]    I[mA]
 *                                                -------------------------
 *                                                0     0..+10   0(4)..+20
 *                                                1    -5..+5    prohibited
 *                                                3   -10..+10   prohibited
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl                pointer to low-level driver data structure
 *                code                setstat code
 *                ch                current channel (0..7)
 *                value32_or_64        setstat value or pointer to blocksetstat data
 *
 *  Output.....:  return    0 | error code
 *
 *  Globals....:  ---
 ****************************************************************************/
static int32 M33_SetStat(
    LL_HANDLE    *llHdl,
    int32        code,
    int32        ch,
    INT32_OR_64 value32_or_64)
{
    register int16 mod, range;
    int32       value = (int32)value32_or_64;   /* 32bit value     */
    /* INT32_OR_64 valueP = value32_or_64;         stores 32/64bit pointer */

    M33_HANDLE* m33Hdl = (M33_HANDLE*) llHdl;

    DBGWRT_1((DBH, "LL - M33_SetStat: ch=%d code=0x%04x value=0x%x\n",
          ch,code,value));

    switch(code)
    {
        /* ----- common setstat codes ----- */

        /* set debug level */
        case M_LL_DEBUG_LEVEL:
            m33Hdl->dbgLevel = value;
            break;

        /* set channel direction */
        case M_LL_CH_DIR:
            if( value != M_CH_OUT )
                return ERR_LL_ILL_DIR;
            break;

        /* ----- module specific setstat codes ----- */

        case M33_CH_RANGE:
           range = (int16)value & 0x03;                    /* range */
               /* check range (only 0, 1, 3 is valid) */
            if( range == 0x02) return ERR_LL_ILL_PARAM;
            /* store range for channel */
            m33Hdl->ch_opt[ch] = range;

           if (ch < 4) {
              mod = (int16)((m33Hdl->setmod[0] & ~(0x8800>>ch)) |   /* clear mx/gx */
                            (range & 1)<<(11-ch)                |   /* set mx */
                            (range & 2)<<(14-ch));                  /* set gx */

              /* set ranges 0..3 */
              m33Hdl->setmod[0] = mod;
              MWRITE_D16(m33Hdl->ma, M33_SETMOD_REG(0), m33Hdl->setmod[0]);
           }
           else {
              ch -= 4;
              mod = (int16)((m33Hdl->setmod[1] & ~(0x8800>>ch)) |   /* clear mx/gx */
                            (range & 1)<<(11-ch)                |   /* set mx */
                            (range & 2)<<(14-ch));                   /* set gx */

              /* set ranges 4..7 */
              m33Hdl->setmod[1] = mod;
              MWRITE_D16(m33Hdl->ma, M33_SETMOD_REG(4), m33Hdl->setmod[1]);
           }
           DBGWRT_2((DBH, " M33_CH_RANGE: set mod=$%04x\n",mod ));
           break;

         /* unknown setstat */
        default:
            return ERR_LL_UNK_CODE;
    }

    return(0);

}/*M33_setstat*/


/******************************** M33_GetStat *******************************
 *
 *  Description:  Gets the device state.
 *
 *        common codes            values            meaning
 *        ---------------------------------------------------------------------
 *        M_LL_CH_NUMBER            8                number of channels
 *
 *        M_LL_CH_DIR                M_CH_OUT        direction of curr ch
 *                                               always out
 *
 *        M_LL_CH_LEN                12                channel length in bit
 *
 *        M_LL_CH_TYP                M_CH_ANALOG        channel type
 *
 *        M_LL_ID_CHECK            0 or 1            check EEPROM-Id from module
 *
 *        M_LL_DEBUG_LEVEL        see dbg.h       current debug level
 *
 *        M_LL_ID_SIZE            128             eeprom size [bytes]
 *
 *        M_LL_BLK_ID_DATA        -               eeprom raw data
 *
 *      *** NOTE: getting M_LL_BLK_ID_DATA, resets the module, i.e. resets ***
 *      ***       all Parameters their reset state.                        ***
 *      ***       (this is a known driver bug)                             ***
 *
 *        M_MK_BLK_REV_ID            pointer to the ident function table
 *
 *
 *        M33 specific codes        values          meaning
 *        ---------------------------------------------------------------------
 *        M33_CH_RANGE            0, 1, 3            get output range
 *                                              (see M33_SetStat)
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl        pointer to low-level driver data structure
 *                code        getstat code
 *                ch        current channel (0..7)
 *
 *
 *  Output.....:  value32_or_64P  pointer to getstat value or
 *                                pointer to blocksetstat data
 *                return    0 | error code
 *
 *  Globals....:  ---
 ****************************************************************************/
static int32 M33_GetStat(
    LL_HANDLE    *llHdl,
    int32        code,
    int32        ch,
    INT32_OR_64 *value32_or_64P)
{
    int32                 *valueP   = (int32*)value32_or_64P;       /* pointer to 32bit value  */
    INT32_OR_64	          *value64P = value32_or_64P;               /* stores 32/64bit pointer  */
    M_SG_BLOCK            *blk      = (M_SG_BLOCK*)value32_or_64P;  /* stores block struct pointer */
    M33_HANDLE*            m33Hdl   = (M33_HANDLE*)llHdl;

    DBGWRT_1((DBH, "LL - M33_GetStat: ch=%d code=0x%04x\n",
              ch,code));

    switch(code)
    {
        /* get number of channels */
        case M_LL_CH_NUMBER:
            *valueP = CH_NUMBER;
            break;

        /* get channel direction */
        case M_LL_CH_DIR:
            *valueP = M_CH_OUT;
            break;

        /* get channel length in bit */
        case M_LL_CH_LEN:
            *valueP = 12;
            break;

        /* get channel type */
        case M_LL_CH_TYP:
            *valueP = M_CH_ANALOG;
            break;

        /* get setting of descriptor key 'ID_CHECK' */
        case M_LL_ID_CHECK:
            *valueP = m33Hdl->useId;
            break;

        /* get debug level */
        case M_LL_DEBUG_LEVEL:
            *valueP = m33Hdl->dbgLevel;
            break;

        /* id prom size */
        case M_LL_ID_SIZE:
            *valueP = MOD_ID_SIZE;
            break;

        /* get ID data */
        case M_LL_BLK_ID_DATA:
        {
            u_int32 n;
            u_int16 *dataP = (u_int16*)blk->data;

            if (blk->size < MOD_ID_SIZE)        /* check buf size */
                return(ERR_LL_USERBUF);

            for (n=0; n<MOD_ID_SIZE/2; n++)        /* read MOD_ID_SIZE/2 words */
                *dataP++ = (int16)m_read((U_INT32_OR_64)m33Hdl->ma, (int8)n);

            break;
        }

        /* get IRQ counter */
        case M_LL_IRQ_COUNT:
            return ERR_LL_UNK_CODE;

        /* get revision ID */
        case M_MK_BLK_REV_ID:
            *value64P = (INT32_OR_64)&m33Hdl->idFuncTbl;
            break;

        /* ----- module specific setstat codes ----- */

        /* get channel option */
        case M33_CH_RANGE:
            *valueP = m33Hdl->ch_opt[ch];
            break;

        /* unknown getstat */
        default:
            return ERR_LL_UNK_CODE;
    }

    return(0);

}/*M33_getstat*/


/***************************** M33_BlockRead ********************************
 *
 *  Description:  (unused)
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl            pointer to low-level driver data structure
 *                ch            current channel
 *                buf            buffer to store read values
 *                size            should be multiple of width
 *
 *  Output.....:  nbrRdBytesP    number of read bytes
 *                return        0 | error code
 *
 *  Globals....:  ---
 ****************************************************************************/
static int32 M33_BlockRead(
     LL_HANDLE    *llHdl,
     int32        ch,
     void        *buf,
     int32        size,
     int32        *nbrRdBytesP)
{
    DBGCMD( M33_HANDLE*            m33Hdl = (M33_HANDLE*)llHdl);

    DBGWRT_1((DBH, "LL - M33_BlockRead: ch=%d, size=%d\n",ch,size));

    return ERR_LL_ILL_FUNC;

}/*M33_block_read*/


/***************************** M33_BlockWrite *******************************
 *
 *  Description:  Output values in 'buf' to all channels of m33 module.
 *                Channels will be written in rising order (0..7).
 *                - loads the values of all channels
 *                - outputs all channels at the same time
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl            pointer to low-level driver data structure
 *                ch            current channel (always ignored)
 *                buf            buffer where output values are stored
 *                  size            number of bytes to write (8*2=16 bytes)
 *
 *  Output.....:  nbrWrBytesP    number of written bytes (0 or 16)
 *                  return        0 | error code
 *
 *  Globals....:  ---
 ****************************************************************************/
static int32 M33_BlockWrite(
     LL_HANDLE    *llHdl,
     int32        ch,
     void        *buf,
     int32        size,
      int32        *nbrWrBytesP
)
{
    register CH_TYPE    *bufP = (CH_TYPE*)buf;      /* ptr to buffer */
    register int16        n;
    M33_HANDLE*            m33Hdl = (M33_HANDLE*)llHdl;

    DBGWRT_1((DBH, "LL - M33_BlockWrite: ch=%d, size=%d\n",ch,size));

    /* error if size != 2*CH_NUMBER */
    if( size != (2 * CH_NUMBER) ){
        *nbrWrBytesP = 0;    /* no bytes written */
        return ERR_LL_ILL_PARAM;
    }

    for (n=0; n<8; n++){
        /* load output value for channel 0..7: */
        MWRITE_D16(m33Hdl->ma, M33_LOADVAL_REG(n), *bufP );
        bufP++;
    }

    /* set all values */
    MWRITE_D16(m33Hdl->ma, M33_SETALL_REG, 0);

    /* return number of written bytes */
    *nbrWrBytesP = 2 * CH_NUMBER;

    return(0);

}/*M33_block_write*/


/******************************** M33_Irq ***********************************
 *
 *  Description:  (unused - the module have no interrupt )
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl  pointer to ll-drv data structure
 *
 *  Output.....:  return LL_IRQ_DEV_NOT
 *
 *  Globals....:  ---
 ****************************************************************************/
static int32 M33_Irq(
   LL_HANDLE    *llHdl)
{
    DBGCMD( M33_HANDLE     *m33Hdl = (M33_HANDLE*) llHdl);

    IDBGWRT_1((DBH, "LL - M33_Irq:\n"));

    /* not my interrupt */
    return LL_IRQ_DEV_NOT;

}/*M33_irq_c*/


/****************************** M33_Info ************************************
 *
 *  Description:  Gets low level driver info.
 *
 *                NOTE: can be called before M33_Init().
 *
 *  supported info type codes        value          meaning
 *  ------------------------------------------------------------------------
 *  LL_INFO_HW_CHARACTER
 *   arg2  u_int32 *addrModeChar    MDIS_MA08        M-Module characteristic
 *   arg3  u_int32 *dataModeChar    MDIS_MD16        M-Module characteristic
 *
 *  LL_INFO_ADDRSPACE_COUNT
 *   arg2  u_int32 *nbrOfMaddr        1                number of address spaces
 *
 *  LL_INFO_ADDRSPACE
 *   arg2  u_int32 addrNr            0        current address space
 *   arg3  u_int32 *addrMode        MDIS_MA08        used address mode
 *   arg4  u_int32 *dataMode        MDIS_MD16        used data mode
 *   arg5  u_int32 *addrSize        0xff            needed size
 *
 *  LL_INFO_IRQ
 *   arg2  u_int32 *useIrq            0            module use no interrupt
 *
 *  The LL_INFO_LOCKMODE
 *   arg2  u_int32 *lockModeP        LL_LOCK_CALL            lock mode needed by driver
 *
 *--------------------------------------------------------------------------
 *  Input......:  infoType            desired information
 *                ...                variable argument list
 *
 *  Output.....:  return            0 | error code
 *
 *  Globals....:  -
 ****************************************************************************/
static int32 M33_Info
(
   int32  infoType,
   ...
)
{
    va_list        argptr;

    va_start(argptr,infoType);

    switch( infoType )
    {
        /* module characteristic */
        case LL_INFO_HW_CHARACTER:
        {
            u_int32 *addrModeChar = va_arg( argptr, u_int32* );
            u_int32 *dataModeChar = va_arg( argptr, u_int32* );

            *addrModeChar = MDIS_MA08;
            *dataModeChar = MDIS_MD16;

            break;
        }

        /* number of address spaces */
        case LL_INFO_ADDRSPACE_COUNT:
        {
            u_int32 *nbrOfMaddr = va_arg( argptr, u_int32* );

            *nbrOfMaddr = 1;

            break;
        }

        /* info about address space */
        case LL_INFO_ADDRSPACE:
        {
            u_int32 addrNr = va_arg( argptr, u_int32 );
            u_int32 *addrMode = va_arg( argptr, u_int32* );
            u_int32 *dataMode = va_arg( argptr, u_int32* );
            u_int32 *addrSize = va_arg( argptr, u_int32* );

            if( addrNr == 0 ){
                *addrMode = MDIS_MA08;
                *dataMode = MDIS_MD16;
                *addrSize = 0x100;
            }
            else {
                va_end( argptr );
                return ERR_LL_ILL_PARAM;
            }

            break;
        }

        /* use no interrupt */
        case LL_INFO_IRQ:
        {
            u_int32 *useIrq = va_arg( argptr, u_int32* );

            *useIrq = 0;
            break;
        }

                /*-------------------------------+
                |   process lock mode            |
                +-------------------------------*/
                case LL_INFO_LOCKMODE:
                {
                    u_int32 *lockModeP = va_arg(argptr, u_int32*);

                    *lockModeP = LL_LOCK_CALL;
                    break;
                }

        /* error */
        default:
            va_end( argptr );
            return ERR_LL_UNK_CODE;
    }

    /* all was ok */
    va_end( argptr );
    return 0;

}/*M33_info*/

/*******************************  Ident  ************************************
 *
 *  Description:  Return ident string
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *
 *  Output.....:  return  ptr to ident string
 *
 *  Globals....:  -
 ****************************************************************************/
static char* Ident( void )
{
    return( (char*)IdentString );
}




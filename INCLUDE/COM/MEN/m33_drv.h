/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: m33_ll.h
 *
 *       Author: ds
 *        $Date: 2010/12/10 15:05:37 $
 *    $Revision: 1.5 $
 *
 *  Description: M33 specific set-/getstat codes and LL prototypes
 *
 *     Switches:  _ONE_NAMESPACE_PER_DRIVER_
 *				  _LL_DRV_
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m33_drv.h,v $
 * Revision 1.5  2010/12/10 15:05:37  amorbach
 * R: driver ported to MDIS5, new MDIS_API and men_typs
 * M: for backward compatibility to MDIS4 optionally define new types
 *
 * Revision 1.4  2004/04/14 14:51:49  cs
 * Minor modifications for MDIS4/2004 conformity
 *       added swapped access variant of M33_GetEntry
 *       changed function prototypes to static and moved to m33_drv.c
 *
 * Revision 1.3  1998/08/03 14:48:27  Schmidt
 * M_DEV_CH_OPTION renamed to M33_CH_RANGE
 * M33_CHNL_NBR removed
 *
 * Revision 1.2  1998/03/06 17:56:48  Schmidt
 * add $Log $
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1992,1993 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _M33_LL_H_                   
#define _M33_LL_H_

#ifdef __cplusplus
	extern "C" {
#endif


/*---------------------------------------------------------------------------+
|    DEFINES                                                                 |
+---------------------------------------------------------------------------*/
/*-- M33 specific status codes ( M_DEV_OF / M_DEV_BLK_OF + 0x00...0xff ) --*/
/*											S,G: S=setstat, G=getstat code */
#define M33_CH_RANGE	M_DEV_OF+0x01	/* S	: channel mode (0, 1, 3)   */

/*---------------------------------------------------------------------------+
|    PROTOTYPES                                                              |
+---------------------------------------------------------------------------*/
#ifdef _LL_DRV_

#	ifndef _ONE_NAMESPACE_PER_DRIVER_

		/* variant for swapped access */
#		ifdef M33_SW
#			define M33_GetEntry             M33_SW_GetEntry
#		endif

		extern void M33_GetEntry( LL_ENTRY* drvP );
#	endif /* _ONE_NAMESPACE_PER_DRIVER_ */

#endif /* _LL_DRV_ */

/*-----------------------------------------+
|  BACKWARD COMPATIBILITY TO MDIS4         |
+-----------------------------------------*/
#ifndef U_INT32_OR_64
 /* we have an MDIS4 men_types.h and mdis_api.h included */
 /* only 32bit compatibility needed!                     */
 #define INT32_OR_64  int32
 #define U_INT32_OR_64 u_int32
 typedef INT32_OR_64  MDIS_PATH;
#endif /* U_INT32_OR_64 */


#ifdef __cplusplus
	}
#endif

#endif /* _M33_LL_H_ */



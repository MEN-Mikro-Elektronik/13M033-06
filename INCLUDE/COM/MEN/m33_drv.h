/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: m33_ll.h
 *
 *       Author: ds
 *
 *  Description: M33 specific set-/getstat codes and LL prototypes
 *
 *     Switches:  _ONE_NAMESPACE_PER_DRIVER_
 *				  _LL_DRV_
 *
 *---------------------------------------------------------------------------
 * Copyright 1992-2019, MEN Mikro Elektronik GmbH
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



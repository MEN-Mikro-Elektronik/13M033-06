/****************************************************************************
 ************                                                    ************
 ************	                    m33_demo.c                   ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: ds
 *        $Date: 2010/12/10 15:01:02 $
 *    $Revision: 1.7 $
 *
 *  Description: Demonstration program for the m33 M-Module
 *                                            
 *     Required: -
 *     Switches: NO_MAIN_FUNC
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m33_demo.c,v $
 * Revision 1.7  2010/12/10 15:01:02  amorbach
 * R: Porting to MDIS5
 * M: added support for 64bit (MDIS_PATH)
 *
 * Revision 1.6  2004/04/14 14:48:02  cs
 * Minor modifications for MDIS4/2004 conformity
 *       changed function M33_Demo prototype to static
 *
 * Revision 1.5  1998/08/03 14:48:13  Schmidt
 * M_DEV_CH_OPTION renamed to M33_CH_RANGE
 *
 * Revision 1.4  1998/07/27 14:31:41  see
 * run-loop removed: single executed
 * M_close error check was wrong
 * cosmetics
 *
 * Revision 1.3  1998/07/17 11:54:12  Schmidt
 * VXWORKS defines removed, support for variable device names implemented,
 * cosmetics to new style
 *
 * Revision 1.2  1998/03/10 17:50:12  Schmidt
 * modified for VXWORKS
 *
 * Revision 1.1  1998/03/04 17:28:55  Schmidt
 * Added by mcvs
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/

static char *RCSid="$Id: m33_demo.c,v 1.7 2010/12/10 15:01:02 amorbach Exp $";

#include <stdio.h>
#include <MEN/men_typs.h>	/* men type definitions */
#include <MEN/mdis_api.h>	/* mdis user interface */
#include <MEN/m33_drv.h>	/* m33 definitions */
#include <MEN/usr_oss.h>	/* user mode system services */

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
static int  M33_Demo( char *devName );
static void ShowError( void );


#ifndef NO_MAIN_FUNC
/******************************** main **************************************
 *
 *  Description:  main() - function
 *
 *---------------------------------------------------------------------------
 *  Input......:  argc		number of arguments
 *				  *argv		pointer to arguments
 *				  argv[1]	device name	 
 *
 *  Output.....:  return	0	if no error
 *							1	if error
 *
 *  Globals....:  -
 ****************************************************************************/
 int main( int argc, char *argv[ ] )
 {
	if( argc < 2){
		printf("usage: m33_demo <device name>\n");
		return 1;
	}
	M33_Demo(argv[1]);
	return 0;
 }
#endif/* NO_MAIN_FUNC */


/******************************* M33_Demo ***********************************
 *
 *  Description:  - open the device
 *				  - configure the device
 *				  - write operations
 *				  - close the device
 *
 *---------------------------------------------------------------------------
 *  Input......:  DeviceName
 *
 *  Output.....:  return	0	if no error
 *							1	if error
 *
 *  Globals....:  -
 ****************************************************************************/
static int M33_Demo( char *devName )
{
	int32 ch, value, range, n;
    MDIS_PATH devHdl;

    printf("\n");
	printf("m33_demo - demonstration program for the M33 module\n");
    printf("===================================================\n\n");

    printf("%s\n\n", RCSid);

    /*----------------------+  
    | open the device       |
    +-----------------------*/
	if( (devHdl = M_open(devName)) < 0 ) goto CLEANUP;
	printf("Device %s opened\n\n", devName);

    /*----------------------+  
    | device configuration  |
    +-----------------------*/
	/* select channel 0 */
	ch = 0;
	printf("M_setstat - set current channel to 0\n");
	if( M_setstat(devHdl, M_MK_CH_CURRENT, ch) < 0 ) goto CLEANUP;	
	/* set range of channel */
	range = 1;
	printf("M_setstat - set range for current channel to -5..+5V\n\n");
	if( M_setstat(devHdl, M33_CH_RANGE, range) < 0 ) goto CLEANUP;   

	/*----------------------+  
    | write operation       |
    +-----------------------*/
	printf("channel 0: produce ramps\n");
	for (n=0; n<10; n++)  {
		/* set channel 0 lowest to highest */
		printf(" lowest..highest ramp\n");
		for (value=0x000; value <= 0xFFF; value++)  {
			if( M_write(devHdl, value<<4) < 0 ) goto CLEANUP;
		}

		/* set channel 0 from highest to lowest */
		printf(" highest..lowest ramp\n");
		for (value=0xFFF; value > 0x000; value--)  {
			if( M_write(devHdl, value<<4) < 0 ) goto CLEANUP;
		}
	}

	/* toggle channel 0 */
	printf("\nchannel 0: toggle lowest/highest\n");
	for (n=0; n<10; n++)  {
		if( M_write(devHdl, 0x000<<4) < 0 ) goto CLEANUP;
		UOS_Delay( 300 );
		if( M_write(devHdl, 0xFFF<<4) < 0 ) goto CLEANUP;
		UOS_Delay( 300 );
	}
	
    /*----------------------+  
    | close the device      |
    +-----------------------*/
    if( M_close(devHdl) < 0) goto CLEANUP;
	printf("\nDevice %s closed\n", devName);

    printf("=> OK\n");
    return 0;
	
	
CLEANUP:
    ShowError();
    printf("=> Error\n");
    return 1;
}

/******************************** ShowError *********************************
 *
 *  Description:  Show MDIS or OS error message.
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *
 *  Output.....:  -
 *
 *  Globals....:  -
 ****************************************************************************/
static void ShowError( void )
{
   u_int32 error;

   error = UOS_ErrnoGet();

   printf("*** %s ***\n",M_errstring( error ) );
}

	
	
	




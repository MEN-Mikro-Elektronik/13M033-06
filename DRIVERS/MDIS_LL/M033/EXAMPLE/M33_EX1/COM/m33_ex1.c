/****************************************************************************
 ************                                                    ************
 ************	                   m33_ex1.c                     ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: ds
 *        $Date: 2010/12/10 15:00:52 $
 *    $Revision: 1.8 $
 *
 *  Description: Example program for the M33 M-Module
 *                                            
 *     Required: -
 *     Switches: NO_MAIN_FUNC
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m33_ex1.c,v $
 * Revision 1.8  2010/12/10 15:00:52  amorbach
 * R: Porting to MDIS5
 * M: added support for 64bit (MDIS_PATH)
 *
 * Revision 1.7  2004/04/14 14:48:07  cs
 * Minor modifications for MDIS4/2004 conformity
 *       changed function M33_Ex1 prototype to static
 *
 * Revision 1.6  1998/08/03 14:48:08  Schmidt
 * M_DEV_CH_OPTION renamed to M33_CH_RANGE
 *
 * Revision 1.5  1998/07/27 14:31:39  see
 * TEST M_setblock(): endless loop replace by for 0..9 loop
 * TEST M_setblock(): print info
 * cosmetics
 *
 * Revision 1.4  1998/07/17 11:52:46  Schmidt
 * VXWORKS defines removed, support for variable device names implemented,
 * cosmetics to new style
 *
 * Revision 1.3  1998/03/13 15:33:59  Schmidt
 * change values in array from 0xffff to 0xfff0
 *
 * Revision 1.2  1998/03/10 17:50:08  Schmidt
 * modified for VXWORKS
 *
 * Revision 1.1  1998/03/04 17:28:49  Schmidt
 * Added by mcvs
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/

static char *RCSid="$Id: m33_ex1.c,v 1.8 2010/12/10 15:00:52 amorbach Exp $";

#include <stdio.h>
#include <MEN/men_typs.h>	/* men type definitions */
#include <MEN/mdis_api.h>	/* mdis user interface */
#include <MEN/m33_drv.h>	/* m33 definitions */
#include <MEN/usr_oss.h>	/* user mode system services */

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
static int  M33_Ex1( char *devName );
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
		printf("usage: m33_ex1 <device name>\n");
		return 1;
	}
	M33_Ex1(argv[1]);
	return 0;
 }
#endif/* NO_MAIN_FUNC */


/******************************* M33_Ex1 ************************************
 *
 *  Description:  - open the device
 *				  - configure the device
 *				  - write operations
 *                - produce an error
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
static int M33_Ex1( char *devName )
{
	u_int16	*arr;
	MDIS_PATH 	devHdl;
	int32	ch;
	int32	n=0;
	int32	value;
	int32	data;
	int32	range[8] = { 0, 1, 3, 0, 1, 3, 0, 1 };   /* ranges for channels 0..7 */
	u_int16 valarr[2][8] = {
				{ 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000 }, /* all min. */
				{ 0xfff0,0xfff0,0xfff0,0xfff0,0xfff0,0xfff0,0xfff0,0xfff0 }  /* all max. */
			};


    printf("\n");
	printf("m33_ex1 - simple example program for the M33 module\n");
    printf("===================================================\n\n");

    printf("%s\n\n", RCSid);

    /*----------------------+  
    | open the device       |
    +-----------------------*/
	if( (devHdl = M_open(devName)) < 0 ) goto CLEANUP;
	printf("Device %s opened\n\n", devName);

    /*----------------------+  
    | get device properties |
    +-----------------------*/
	printf("Device properties:\n");
	printf("------------------\n");
	/* number of channels */
	if( M_getstat(devHdl,M_LL_CH_NUMBER, &data) < 0 ) goto CLEANUP;
	printf(" number of channels:      %d\n", (int) data);
	/* channel length */
	if( M_getstat(devHdl,M_LL_CH_LEN, &data) < 0 ) goto CLEANUP;
	printf(" channel length:          %dBit\n", (int) data);
	/* channel direction */
	if( M_getstat(devHdl,M_LL_CH_DIR, &data) < 0 ) goto CLEANUP;
	printf(" channel direction type:  %d\n", (int) data);
	/* channel type */
	if( M_getstat(devHdl,M_LL_CH_TYP, &data) < 0 ) goto CLEANUP;
	printf(" channel type:            %d\n\n", (int) data);

    /*----------------------+  
    | device configuration  |
    +-----------------------*/
	for (ch=0; ch < 8; ch++)  { 
		/* select channel ch */
		if( M_setstat(devHdl, M_MK_CH_CURRENT, ch) < 0 ) goto CLEANUP;	
		/* set range of channel */
		if( M_setstat(devHdl, M33_CH_RANGE, range[ch]) < 0 ) goto CLEANUP;   
		printf("channel %ld: range=%ld\n", ch, range[ch]);
	}

	/*----------------------+  
    | write operation       |
    +-----------------------*/
	printf("\nWrite data to the device:\n");
	printf("-------------------------\n");

	/* TEST M_write() (channel 2..5, lowest/highest) */
	printf("TEST M_write() (channel 2..5, lowest/highest) \n");
	for (ch=2; ch <= 5; ch++)  { 
		/* select channel ch */
		if( M_setstat(devHdl, M_MK_CH_CURRENT, ch) < 0 ) goto CLEANUP;
		value = 0x0000;
		printf("channel %ld: value %ld\n", ch, value);
		if( M_write(devHdl, value) < 0 ) goto CLEANUP;
		UOS_Delay( 3000 );
		value = 0xFFFF;
		printf("channel %ld: value %ld\n", ch, value);
		if( M_write(devHdl, value) < 0 ) goto CLEANUP;
		UOS_Delay( 3000 );
	}

	/* TEST channel 0 (low..high) */
	printf("\nTEST channel 0 (low..high ramp) \n");
	/* select channel 0 */
	if( M_setstat(devHdl, M_MK_CH_CURRENT, 0) < 0 ) goto CLEANUP;
	for (value=0x000; value <= 0xFFF; value+=0x4)  {
		if( M_write(devHdl, value<<4) < 0 ) goto CLEANUP;
		UOS_Delay( 1 );
	}

	/* TEST M_setblock() (channel 0..7, lowest/highest) */
	printf("\nTEST M_setblock() (channel 0..7, lowest/highest): \n");
	
	for (n=0; n<10; n++) {
		/* select array */
		arr = &valarr[n & 1][0];
		/* set all channels */
		printf("all channels: %s\n",(n & 1 ? "highest":"lowest"));
	
		if( M_setblock(devHdl,(u_int8*)arr,16) < 0 ) goto CLEANUP;
		UOS_Delay( 2000 );
	}

	/*----------------------+  
    | produce an error      |
    +-----------------------*/
    printf("\n");
	printf("Produce an error:\n");
	printf("-----------------\n");
	if( M_setstat(devHdl,M_LL_NOTEXIST, 0) < 0 ) ShowError();

    /*----------------------+  
    | close the device      |
    +-----------------------*/
    if( M_close(devHdl) ) goto CLEANUP;
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






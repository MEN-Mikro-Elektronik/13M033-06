#**************************  M a k e f i l e ********************************
#  
#        $Author: cs $
#          $Date: 2004/04/14 14:48:00 $
#      $Revision: 1.3 $
#  
#    Description: Makefile definitions for the m33_demo example program
#                      
#---------------------------------[ History ]---------------------------------
#
#   $Log: program.mak,v $
#   Revision 1.3  2004/04/14 14:48:00  cs
#   Minor modifications for MDIS4/2004 conformity
#         removed MAK_OPTIM=$(OPT_1)
#
#   Revision 1.2  1998/07/17 11:54:07  Schmidt
#   cosmetics
#
#   Revision 1.1  1998/03/04 17:28:52  Schmidt
#   Added by mcvs
#
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany 
#*****************************************************************************


MAK_NAME=m33_demo

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX) \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)

MAK_INCL=$(MEN_INC_DIR)/men_typs.h \
         $(MEN_INC_DIR)/mdis_api.h \
         $(MEN_INC_DIR)/m33_drv.h \
         $(MEN_INC_DIR)/usr_oss.h

MAK_INP1=m33_demo$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)

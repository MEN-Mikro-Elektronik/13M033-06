#**************************  M a k e f i l e ********************************
#  
#         Author: ds
#          $Date: 2005/07/11 09:18:35 $
#      $Revision: 1.5 $
#  
#    Description: makefile descriptor for MDIS LL-Driver 
#                      
#---------------------------------[ History ]---------------------------------
#
#   $Log: driver.mak,v $
#   Revision 1.5  2005/07/11 09:18:35  cs
#   swapped oss and id_sw in MAK_LIBS
#
#   Revision 1.4  2004/04/14 14:27:29  cs
#   Minor modifications for MDIS4/2004 conformity
#         removed MAK_OPTIM=$(OPT_1)
#         added MAK_SWITCH=$(SW_PREFIX)MAC_MEM_MAPPED
#
#   Revision 1.3  1998/07/17 11:52:22  Schmidt
#   libs in new order, includes added, cosmetics
#
#   Revision 1.2  1998/03/05 10:34:44  Schmidt
#   cosmetics
#
#   Revision 1.1  1998/03/04 17:28:34  Schmidt
#   Added by mcvs
#
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany 
#*****************************************************************************

MAK_NAME=m33

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/desc$(LIB_SUFFIX)    \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/id$(LIB_SUFFIX)      \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/oss$(LIB_SUFFIX)     \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/dbg$(LIB_SUFFIX)

MAK_INCL=$(MEN_INC_DIR)/mdis_com.h   \
         $(MEN_INC_DIR)/men_typs.h   \
         $(MEN_INC_DIR)/oss.h        \
         $(MEN_INC_DIR)/mdis_err.h   \
         $(MEN_INC_DIR)/maccess.h    \
         $(MEN_INC_DIR)/desc.h       \
         $(MEN_INC_DIR)/ll_defs.h    \
         $(MEN_INC_DIR)/mdis_api.h   \
         $(MEN_INC_DIR)/ll_entry.h   \
         $(MEN_INC_DIR)/modcom.h     \
         $(MEN_INC_DIR)/m33_drv.h    \
         $(MEN_INC_DIR)/dbg.h

MAK_SWITCH=$(SW_PREFIX)MAC_MEM_MAPPED

MAK_INP1=m33_drv$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)


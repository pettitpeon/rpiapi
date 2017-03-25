/*------------------------------------------------------------------------------
 *                             (c) 2015 by Ivan Peon
 *                             All rights reserved
 *------------------------------------------------------------------------------
 *   Module   : BaIpcRegistry.cpp
 *   Date     : 17.03.2017
 *------------------------------------------------------------------------------
 */

/*------------------------------------------------------------------------------
    Includes
 -----------------------------------------------------------------------------*/
#include "BaIpcRegistry.h"
#include "CBaIpcRegistry.h"
#include "BaBool.h"
/*------------------------------------------------------------------------------
    Defines
 -----------------------------------------------------------------------------*/
#define C_HDL_ ((IBaIpcRegistry*) hdl)

/*------------------------------------------------------------------------------
    Type definitions
 -----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    C Interface
 -----------------------------------------------------------------------------*/
//
TBaIpcRegistryHdl BaIpcRegistryCreate() {
   return CBaIpcRegistry::Create();
}

//
TBaBoolRC BaIpcRegistryDestroy(TBaIpcRegistryHdl hdl) {
   return IBaIpcRegistryDestroy(C_HDL_);
}
/*------------------------------------------------------------------------------
    C++ Factories
 -----------------------------------------------------------------------------*/
//
IBaIpcRegistry * IBaIpcRegistryCreate() {
   return CBaIpcRegistry::Create();
}

//
TBaBoolRC IBaIpcRegistryDestroy(IBaIpcRegistry *pHdl) {
   CBaIpcRegistry *p = dynamic_cast<CBaIpcRegistry*>(pHdl);
   if (p) {
      return CBaIpcRegistry::Destroy(p) ? eBaBoolRC_Success : eBaBoolRC_Error;
   }
   return eBaBoolRC_Error;
}

/*------------------------------------------------------------------------------
    C++ Interface
 -----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    Local functions
 -----------------------------------------------------------------------------*/
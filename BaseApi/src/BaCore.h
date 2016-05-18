/*------------------------------------------------------------------------------
 *                             (c) 2015 by Ivan Peon
 *                             All rights reserved
 *------------------------------------------------------------------------------
 *   Module   : BaCore.h
 *   Date     : 23/06/2015
 *------------------------------------------------------------------------------
 *   Module description:
 */
/** @file
 *   OS level API. Core functions include:
 *    - Timers
 *    - Threads
 *    - Sleeps
 */
/*------------------------------------------------------------------------------
 */

#ifndef _BACORE_H
#define _BACORE_H

/*------------------------------------------------------------------------------
 *  Includes
 */
#include "BaBool.h"
#include <unistd.h>


/*------------------------------------------------------------------------------
 *  Type definitions
 */

/// Thread priority and scheduler. RT is FIFO
// Windows does not have RT
typedef enum EBaCorePrio {
   eBaCorePrio_Minimum = 0, ///< 0
   eBaCorePrio_Low,         ///< 1
   eBaCorePrio_Normal,      ///< 2
   eBaCorePrio_High,        ///< 3
   eBaCorePrio_Highest,     ///< 4
   eBaCorePrio_RT_Normal,   ///< 5
   eBaCorePrio_RT_High,     ///< 6
   eBaCorePrio_RT_Highest   ///< 7
} EBaCorePrio;

/// Thread function arguments
typedef struct TBaCoreThreadArg {
   void    *pArg; ///< General arguments used in the thread
   TBaBool exitTh;  ///< Exit flag to inform that it must return
} TBaCoreThreadArg;

/// Thread info
typedef struct TBaCoreThreadInfo {
   char         name[64];  ///< Thread name
   EBaCorePrio  prio;      ///< Thread priority and scheduler
   pid_t        tid;       ///< Thread ID
   TBaBool      isRunning; ///< Is running flag
} TBaCoreThreadInfo;


/// General function of the API
typedef void (*TBaCoreFun)(void* arg);

/// Functions for threads
typedef void (*TBaCoreThreadFun)(TBaCoreThreadArg* pArg);

/// Thread handle.
typedef const void* TBaCoreThreadHdl;

/*------------------------------------------------------------------------------
 *  API Functions
 */
#ifdef __cplusplus
extern "C" {
#endif


/// @name Sleep functions
//@{
/******************************************************************************/
/** Puts the thread to sleep and causes context change.
 *  if s = 0 || ms = 0 || us = 0, there is no sleep
 *  if ns = 0, the thread sleeps for one tick as specified by nanosleep()
 */
void BaCoreSleep(
      int64_t s ///< [in] Sleep time in s
      );
void BaCoreMSleep(
      int64_t ms ///< [in] Sleep time in ms
      );
void BaCoreUSleep(
      int64_t us ///< [in] Sleep time in us
      );
void BaCoreNSleep(
      int64_t ns ///< [in] Sleep time in ns
      );
//@} Sleep functions


/// @name Timing functions
//@{
/******************************************************************************/
/** Timing function
 *  @return Time in the defined unit
 */
int64_t BaCoreTimedS(
      TBaCoreFun fun, ///< [in] Function to be timed
      void* pArg      ///< [in] Function arguments
      );
int64_t BaCoreTimedMs(
      TBaCoreFun fun, ///< [in] Function to be timed
      void* pArg      ///< [in] Function arguments
      );
int64_t BaCoreTimedUs(
      TBaCoreFun fun, ///< [in] Function to be timed
      void* pArg      ///< [in] Function arguments
      );
//@} Timing functions


/// @name Multi-threading
//@{
/******************************************************************************/
/** Thread factory
 *  @return Thread handle
 */
TBaCoreThreadHdl BaCoreCreateThread(
      const char *name,         ///< [in] Thread name
      TBaCoreThreadFun routine, ///< [in] Thread entry function
      TBaCoreThreadArg *pArg,   ///< [in] Arguments of thread function
      EBaCorePrio prio          ///< [in] Thread priority
      );

/******************************************************************************/
/** Thread destructor. It signals the thread to end and waits a maximum of
 *  @c timeOutMs ms. If it timeouts, the thread detaches, stays alive, and
 *  releases the resources when the thread routine returns.
 *  @return Error if bad handle, or success
 */
TBaBoolRC BaCoreDestroyThread(
      TBaCoreThreadHdl hdl, ///< [in] Thread handle
      uint32_t timeOutMs    ///< [in] Timeout to wait in ms.
      );

/******************************************************************************/
/** Get the thread info
 *  @return Error or success
 */
TBaBoolRC BaCoreGetThreadInfo(
      const TBaCoreThreadHdl hdl, ///< [in] Thread handle
      TBaCoreThreadInfo *pInfo    ///< [out] Thread info
      );
//@} Multi-threading

/******************************************************************************/
/** Set the priority of the calling process. TODO: unit test
 *  @return Error or success
 */
TBaBoolRC BaCoreSetOwnProcPrio(
      EBaCorePrio prio    ///< [in] New priority
      );

/******************************************************************************/
/** Get the priority of the calling process. TODO: unit test
 *  @return the priority
 */
EBaCorePrio BaCoreGetOwnProcPrio();

// ///////////////
const char* BaCoreGetOwnName();

int BaCoreReadPidFile(const char *progName);

TBaBoolRC BaCoreTestPidFile(const char *progName);

int BaCoreWritePidFile(const char *progName);

int BaCoreRemovePidFile(const char *progName);
// ///////////////

#ifdef __cplusplus
}
#endif
#endif // _BACORE_H


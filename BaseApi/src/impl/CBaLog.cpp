/*------------------------------------------------------------------------------
 *                             (c) 2015 by Ivan Peon
 *                             All rights reserved
 *------------------------------------------------------------------------------
 *   Module   : CBalog.cpp
 *   Date     : Dec 9, 2015
 *------------------------------------------------------------------------------
 */


/*------------------------------------------------------------------------------
    C Includes
 -----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

/*------------------------------------------------------------------------------
    C++ Includes
 -----------------------------------------------------------------------------*/
#include <map>
#include <iostream>
#include <ctime>

/*------------------------------------------------------------------------------
    Local Includes
 -----------------------------------------------------------------------------*/
#include "BaUtils.hpp"
#include "CBaLog.h"
#include "../BaGenMacros.h"
#include "BaCore.h"
#include "BaIniParse.h"

/*------------------------------------------------------------------------------
    Defines
 -----------------------------------------------------------------------------*/
#ifdef __linux
# define LOGDIR "/var/log/"
# define CFGDIR "/var/log/config/"
#else
# define LOGDIR "C:\\log\\"
# define CFGDIR "C:\\log\\config\\"
#endif

// Portable make dir
#ifdef _WIN32
# define _MKDIR(dir) mkdir(dir)
#else
# define _MKDIR(dir) mkdir(dir, 0660)
#endif

#define LOGEXT    ".log"
#define CHRONOHRC std::chrono::high_resolution_clock
#define CHRONO    std::chrono
#define TAG       "BaLog"
#define TAGSZ     7
#define FULLPATH(PATH, NAME)  PATH + NAME + LOGEXT

/*------------------------------------------------------------------------------
    Static variables
 -----------------------------------------------------------------------------*/
static std::map<std::string, CBaLog*> sLoggers;
static TBaCoreThreadHdl sLogdHdl = 0;
static TBaCoreThreadArg sLogdArg = {0};
static std::mutex sMtx;
static std::string sPrioTochar[eBaLogPrio_UpsCrash + 1] = {"T", "W", "E", "C"};

/*------------------------------------------------------------------------------
    Local Declarations
 -----------------------------------------------------------------------------*/
namespace tmp_4_9_2 {
LOCAL tm localtime(const std::time_t& rTime);
LOCAL std::string put_time(const std::tm* pDateTime, const char* cTimeFormat);
}

/*------------------------------------------------------------------------------
    Implementation
 -----------------------------------------------------------------------------*/

//
void CBaLog::logRoutine(TBaCoreThreadArg *pArg) {
   while (!sLogdArg.exitTh) {
      { // RAII Scope
         std::lock_guard<std::mutex> lck(sMtx);

         // iterate loggers
         for (auto &kv : sLoggers) {
            kv.second->flush2Disk();
         }
      }

      BaCoreMSleep(50);
   }
}

//
bool CBaLog::init() {
   if (sLogdHdl) {
      return true;
   }

   // Check if dir exists
   struct stat info;
   if (stat(LOGDIR, &info) != 0) {
      // Create directory
      if (_MKDIR(LOGDIR) != 0) {
         // not ok
         return false;
      }
   }

   sLogdArg.exitTh = false;
   sLogdHdl = BaCoreCreateThread("BaLogD", logRoutine, &sLogdArg, eBaCorePrio_High);
   return sLogdHdl;
}

//
bool CBaLog::exit() {
   if (!sLogdHdl) {
      return true;
   }

   BaCoreDestroyThread(sLogdHdl, 100);
   sLogdHdl = 0;
   return true;
}

//
CBaLog* CBaLog::commonCreate(std::string name, std::string path, EBaLogPrio prioFilt,
      EBaLogOut out, int32_t maxFileSizeB, uint16_t maxNoFiles, uint16_t maxBufLength,
      uint16_t fileCnt, int32_t fileSizeB, bool fromCfg)  {

   if (name.empty() || !init()) {
      // todo Log error
      return 0;
   }

   // Check if already exists
   auto logger = sLoggers.find(name);
   if (logger != sLoggers.end()) {
      logger->second->mOpenCnt++;
      return logger->second;
   }

   if (path == "") {
      path = LOGDIR;
   }

   // //////////////// Create //////////////
   CBaLog *p = new CBaLog(name, path, prioFilt, out, maxFileSizeB, maxNoFiles, maxBufLength, fileCnt,
         fileSizeB);
   if (!p) {
      // todo Log error
      return 0;
   }
   // //////////////// Create //////////////


   p->mFullPath = FULLPATH(p->mPath, p->mName);
   p->mLog.open(p->mFullPath, std::ios_base::binary | std::ios_base::out | std::ios_base::app);
   if (p->mLog.fail()) {
      delete p;
      return 0;
      // todo: error
   }
   sLoggers[p->mName] = p;
   return p;
}

//
CBaLog* CBaLog::Create(std::string name, std::string path, EBaLogPrio  prioFilt,
      EBaLogOut out, uint32_t maxFileSizeB, uint16_t maxNoFiles,
      uint16_t maxBufLength) {
   std::lock_guard<std::mutex> lck(sMtx);

   return commonCreate(name, path, prioFilt, out, maxFileSizeB, maxNoFiles, maxBufLength, 1, 1, 0);
}

//
CBaLog* CBaLog::Create(TBaLogOptions &rOpts) {
   std::lock_guard<std::mutex> lck(sMtx);

   return commonCreate(rOpts.name, rOpts.path, rOpts.prioFilt, rOpts.out,
         rOpts.maxFileSizeB, rOpts.maxNoFiles, rOpts.maxBufLength, 1, 1, 0);
}

//
CBaLog* CBaLog::CreateFromCfg(std::string cfgFile) {

   IBaIniParser *pIni = CBaIniParserCreate(cfgFile.c_str());
   if (!pIni) {
      // todo: log?
      return 0;
   }

   std::string name = pIni->GetString(TAG ":mName", "");
   std::string path = pIni->GetString(TAG ":mPath", "");
   EBaLogPrio prioFilt = (EBaLogPrio) pIni->GetInt(TAG ":mPrioFilt", eBaLogPrio_UpsCrash);
   EBaLogOut out = (EBaLogOut) pIni->GetInt(TAG ":mOut", eBaLogOut_LogAndConsole);
   uint32_t maxFileSizeB = (uint32_t) pIni->GetInt(TAG ":mMaxFileSizeB", -1);
   int32_t  maxNoFiles = pIni->GetInt(TAG ":mMaxNoFiles", -1);
   uint32_t maxBufLength = (uint32_t) pIni->GetInt(TAG ":mMaxBufLength", -1);
   uint32_t fileSizeB = (uint32_t) pIni->GetInt(TAG ":mFileSizeB", -1);
   uint16_t fileCnt = (uint16_t) pIni->GetInt(TAG ":mFileCnt", -1);

   if (name == "" || path == "" || maxFileSizeB == (uint32_t) -1 ||
       maxNoFiles == -1 || maxBufLength == (uint32_t) -1 ||
       fileCnt == (uint16_t) -1) {
      // todo: log?
      return 0;
   }

   std::lock_guard<std::mutex> lck(sMtx);

   return commonCreate(name, path, prioFilt, out, maxFileSizeB, maxNoFiles, maxBufLength,
         fileCnt, fileSizeB, true);
}

//
bool CBaLog::Destroy(IBaLog *pHdl, bool saveCfg) {
   std::lock_guard<std::mutex> lck(sMtx);

   CBaLog *p = dynamic_cast<CBaLog*>(pHdl);
   if (!p ) {
      return false;
   }

   if (--p->mOpenCnt == 0) {
      // Erase from loggers
      sLoggers.erase(p->mName);
      p->flush2Disk();
      p->mLog.close();

      // Save state
      if (saveCfg) {
         p->saveCfg();
      }

      delete p;
   }

   if (sLoggers.empty()) {
      exit();
   }

   return true;
}

//
bool inline CBaLog::Log(EBaLogPrio prio, const char* tag, const char* msg) {
   // This uses a member mutex so that msgs can be buffered
   // while another log is flushing
   std::lock_guard<std::mutex> lck(mMtx);
   return log(prio, tag, msg);
}

//
bool inline CBaLog::Trace(const char* tag, const char* msg){
   std::lock_guard<std::mutex> lck(mMtx);
   return log(eBaLogPrio_Trace, tag, msg);
}

//
bool inline CBaLog::Warning(const char* tag, const char* msg){
   std::lock_guard<std::mutex> lck(mMtx);
   return log(eBaLogPrio_Warning, tag, msg);
}

//
bool inline CBaLog::Error(const char* tag, const char* msg){
   std::lock_guard<std::mutex> lck(mMtx);
   return log(eBaLogPrio_Error, tag, msg);
}

//
bool inline CBaLog::LogF(EBaLogPrio prio, const char* tag, const char* fmt, ...) {
   va_list arg;
   va_start(arg, fmt);
   bool ret = logV(prio, tag, fmt, arg);
   va_end(arg);
   return ret;
}

//
bool inline CBaLog::TraceF(const char* tag, const char* fmt, ...) {
   va_list arg;
   va_start(arg, fmt);
   bool ret = logV(eBaLogPrio_Trace, tag, fmt, arg);
   va_end(arg);
   return ret;
}

//
bool inline CBaLog::WarningF(const char* tag, const char* fmt, ...) {
   va_list arg;
   va_start(arg, fmt);
   bool ret = logV(eBaLogPrio_Warning, tag, fmt, arg);
   va_end(arg);
   return ret;
}

//
bool inline CBaLog::ErrorF(const char* tag, const char* fmt, ...) {
   va_list arg;
   va_start(arg, fmt);
   bool ret = logV(eBaLogPrio_Error, tag, fmt, arg);
   va_end(arg);
   return ret;
}

//
inline void CBaLog::flush2Disk() {

   // Avoid messing around with the buffer while it is being printed
   std::lock_guard<std::mutex> lck(mMtx);

   // Iterate messages in buffer
   for (auto &msg : mBuf) {

      // Check file size. If it is the first line written to the file,
      // write it to disk anyways.
      mFileSizeB += msg.size();
      if (msg.size() != mFileSizeB && mFileSizeB > mMaxFileSizeB) {
         // Open new file
         if (++mFileCnt > mMaxNoFiles) {
            mFileCnt = 1;
            // Rewrite file
         }

         mFileSizeB = 0;

         // Close the output stream
         mLog.close();
         if (mLog.fail()) {
            // todo: error
         }

         // Rename file
         mTmpPath = BaPath::ChangeFileExtension(mFullPath,
               "_" + std::to_string(mFileCnt) + ".log");

         std::cout << mFullPath << " to: " << mTmpPath << std::endl;

         // Windows does not let rewriting the file!! fix it to make it portable
#ifdef __WIN32
         remove(mTmpPath.c_str());
#endif
         if (rename(mFullPath.c_str(), mTmpPath.c_str()) == -1) {
//            std::cout << errno << std::endl;
            // todo: error
         }

         mLog.open(mFullPath, std::ios_base::binary | std::ios_base::out);
         if (mLog.fail()) {
            // todo: error
         }
      }

      // /////////// Log to disc ///////////////////////
      mLog << msg << std::endl;
      std::cout << msg << std::endl;
      // ///////////////////////////////////////////////

   }
   mBuf.clear();
}

//
bool CBaLog::saveCfg() {

   // Create file-less
   IBaIniParser *pIni = CBaIniParserCreate(0);

   // Set tag for section
   pIni->Set(TAG, "");

   // Set the rest of the values
   pIni->Set(TAG ":mName", mName.c_str());
   pIni->Set(TAG ":mPath", mPath.c_str());

   std::string s = std::to_string(mPrioFilt);
   pIni->Set(TAG ":mPrioFilt", s.c_str());

   s = std::to_string(mOut);
   pIni->Set(TAG ":mOut", s.c_str());


   s = std::to_string(mMaxFileSizeB);
   pIni->Set(TAG ":mMaxFileSizeB", s.c_str());

   s = std::to_string(mMaxNoFiles);
   pIni->Set(TAG ":mMaxNoFiles", s.c_str());

   s = std::to_string(mFileCnt);
   pIni->Set(TAG ":mFileCnt", s.c_str());

   s = std::to_string(mFileSizeB);
   pIni->Set(TAG ":mFileSizeB", s.c_str());

   s = std::to_string(mMaxBufLength);
   pIni->Set(TAG ":mMaxBufLength" ,s.c_str());



   // Check if dir exists
   struct stat info;
   if (stat(CFGDIR, &info) != 0) {
      // Create directory
      if (_MKDIR(CFGDIR) != 0) {
         // not ok
         return false;
      }
   }

   // Generate path
   s = CFGDIR + mName + ".cfg";

   // Open file for saving cfg
   FILE *pF = fopen(s.c_str(), "w+");
   if (!pF) {
      return false;
   }

   pIni->DumpIni(pF);
   return fclose(pF) == 0;
}

//
bool CBaLog::log(EBaLogPrio prio, const char* tag, const char* msg) {
   if (!msg) {
      return false;
   }

   // Generate time stamp with priority
   // Get the prio
   prio = MINMAX(prio, eBaLogPrio_Trace, eBaLogPrio_UpsCrash);


   // Priority filter. Only log things equal or higher prio than the filter
   if (prio < mPrioFilt) {
      return true;
   }

   // Get the time structure to generate time stamp
   auto nowT = CHRONOHRC::now();
   std::time_t tt = CHRONOHRC::to_time_t(nowT);
   struct tm timeStruct = tmp_4_9_2::localtime(tt);

   // Get the milliseconds
   auto ms = CHRONO::duration_cast<CHRONO::milliseconds>(nowT.time_since_epoch());
   snprintf(mMillis, 4, "%03d", (int) (ms.count() % 1000));

   // Get the tag
   // FIXME: Test this under Linux. The stupid snprintf() is not portable!!
   uint8_t cnt = snprintf(mTag, TAGSZ-1, "%s", tag ? tag : "");
   mTag[TAGSZ - 1] = 0;
   if (cnt < TAGSZ - 1) {
      SETARR(mTag + cnt, TAGSZ - 1 - cnt, ' ');
   }

   // Put everything together
   std::string entry = tmp_4_9_2::put_time(&timeStruct, "%y/%m/%d %H:%M:%S") +
         "." + mMillis + "|" + sPrioTochar[prio] + "|" + mTag + "| " + msg;


   // ////////////////// Write to buffer if it is not full /////////////////////
   if ((!mMaxBufLength || mBuf.size() < mMaxBufLength) && (mOut & eBaLogOut_Log)) {
      mBuf.push_back(entry);
   } else {
      // todo: else? log error?
      return false;
   }
   // //////////////////////////////////////////////////////////////////////////


   // Write to console if desired
   if (mOut & eBaLogOut_Console) {
      std::cout << entry << std::endl;
   }

   return true;
}

//
bool inline CBaLog::logV(EBaLogPrio prio, const char* tag, const char* fmt, va_list arg) {
   if (!fmt) {
      return false;
   }

   uint16_t size = snprintf(0, 0, fmt, arg);

   // WHAT ABOUT REENTRANCY? No problema!
   char msg[size + 1];
   vsnprintf(msg, size, fmt, arg);
   std::lock_guard<std::mutex> lck(mMtx);
   return log(prio, tag, msg);
}

/*------------------------------------------------------------------------------
    Local functions
 -----------------------------------------------------------------------------*/
// put_time is not implemented yet in 4.9.2 thus the tmp NS
namespace tmp_4_9_2 {
LOCAL tm localtime(const std::time_t& rTime) {
   std::tm tm_snapshot;
#ifdef __WIN32
   localtime_s(&tm_snapshot, &rTime);
#else
   localtime_r(&rTime, &tm_snapshot); // POSIX
#endif
   return tm_snapshot;
}


// To simplify things the return value is just a string. I.e. by design!
LOCAL std::string put_time(const std::tm* pDateTime, const char* cTimeFormat) {
   const size_t size = 1024;
   char buffer[size];

   if (!std::strftime(buffer, size, cTimeFormat, pDateTime)) {
      return cTimeFormat;
   }

   return buffer;
}
} // NS tmp_4_9_2




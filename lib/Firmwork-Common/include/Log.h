//
// Created by Andrew Simmons on 10/25/25.
//

#ifndef FIRMWORK_DEBUG_H
#define FIRMWORK_DEBUG_H

#define FANCY_LOGGING
#include <Arduino.h>

#ifndef FANCY_LOGGING
#define DEBUG(str) Serial.println(String("[DEBUG] ")+str)
#define LOG(str) Serial.println(str)
#else

#include <stdarg.h>


namespace Log {
  enum Level { Debug=0, Info, Warn, Error, None };

  constexpr const char* base(const char* p){
    const char* b=p; for(const char* c=p; *c; ++c) if(*c=='/'||*c=='\\') b=c+1; return b;
  }

  struct Logger {
    // Optional override
    static void begin(Print& out = Serial, Level lvl = Debug){ _out=&out; _lvl=lvl; _inited=true; }
    static void level(Level lvl){ _lvl=lvl; }

    // Stream-style: LOGD("Hey", 5, " -> ", 3.14)
    template<typename... Args>
    static void log(Level lvl, const char* file, int line, Args&&... args){
      ensure(); if(lvl<_lvl) return;
      header(lvl, file, line);
      printJoin(std::forward<Args>(args)...);
      _out->write("\r\n");
    }

    // printf-style: LOGDf("v=%d t=%.2f", v, t)
    static void logf(Level lvl, const char* file, int line, const char* fmt, ...){
      ensure(); if(lvl<_lvl) return;
      header(lvl, file, line);
      char buf[256];
      va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
      _out->print(buf); _out->write("\r\n");
    }

  private:
    static void ensure(){
      if(_inited) return;
      _out = &Serial;
      #ifndef LOG_NO_AUTO_SERIAL_BEGIN
        if(!Serial) Serial.begin(115200);
      #endif
      _lvl = Debug; _inited=true;
    }

    static void header(Level lvl, const char* file, int line){
      _out->print('['); _out->print(levelName(lvl)); _out->print("] ");
      _out->print(base(file)); _out->print(':'); _out->print(line); _out->print(" - ");
    }

    // Printing helpers
    static void printOne(const __FlashStringHelper* s){ _out->print(s); }
    static void printOne(const String& s){ _out->print(s); }
    static void printOne(const char* s){ _out->print(s); }
    static void printOne(char* s){ _out->print(s); }
    static void printOne(bool b){ _out->print(b ? "true" : "false"); }
    template<typename T> static void printOne(const T& v){ _out->print(v); }

    static void printJoin(){ /* nothing */ }
    template<typename T, typename... Rest>
    static void printJoin(T&& first, Rest&&... rest){
      printOne(std::forward<T>(first));
      if constexpr (sizeof...(rest)>0) { _out->print(' '); printJoin(std::forward<Rest>(rest)...); }
    }

    static const char* levelName(Level l){
      switch(l){case Debug:return "DEBUG";case Info:return "INFO";
                case Warn:return "WARN";case Error:return "ERROR"; default:return "?";}
    }
    static inline Print* _out=nullptr;
    static inline Level  _lvl=Debug;
    static inline bool   _inited=false;
  };
}
// Macros (compile out with -DLOG_DISABLE or #define LOG_DISABLE)
#ifdef LOG_DISABLE
#define DEBUG(...)
#define INFO(...)
#define WARN(...)
#define ERROR(...)
#else
#define DEBUG(fmt, ...) Log::Logger::log(Log::Debug, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define INFO(fmt, ...) Log::Logger::log(Log::Info,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define WARN(fmt, ...) Log::Logger::log(Log::Warn,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) Log::Logger::log(Log::Error, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#endif

#endif

#endif //FIRMWORK_DEBUG_H
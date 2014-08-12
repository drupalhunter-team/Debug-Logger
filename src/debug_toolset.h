#ifndef _DBG_LOG_
#define _DBG_LOG_
#include "lib_linkers\dbglinker.h"

#include <assert.h>
#include <cstdio>

#include <vector>

#include <fstream>
#include <sstream>

#include <thread>
#include <functional>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
	#include <windows.h>
/*
Function for grabbing the timestamps
Windows Version
*/
	inline std::string NowTime()
	{
		const int MAX_LEN = 200;
		char buffer[MAX_LEN];
		if ( GetTimeFormatA(LOCALE_USER_DEFAULT, 0, 0,
			"HH':'mm':'ss", buffer, MAX_LEN) == 0 )
			return "Error in NowTime()";

		char result[100] = { 0 };
		static DWORD first = GetTickCount();
		std::sprintf(result, "%s.%03ld", buffer, (long)(GetTickCount() - first) % 1000);
		return result;
	}

#else
	#include <sys/time.h>

	inline std::string NowTime()
	{
		char buffer[11];
		time_t t;
		time(&t);
		tm r = {0};
		strftime(buffer, sizeof(buffer), "%X", localtime_r(&t, &r));
		struct timeval tv;
		gettimeofday(&tv, 0);
		char result[100] = {0};
		std::sprintf(result, "%s.%03ld", buffer, (long)tv.tv_usec / 1000); 
		return result;
	}

#endif //WIN32

namespace dbg
{
	//Enums representing the different log levels
	// Implemented as bits in a Byte as to facilitate turning specific levels on and off
	enum LogLevel 
	{ 
		logFATAL = 1 << 0,			logERROR = 1 << 1, 
		logWARNING = 1 << 2,		logINFO = 1 << 3, 
		logDEBUG1 = 1 << 4,		logDEBUG2 = 1 << 5, 
		logDEBUG3 = 1 << 6,		logDEBUG4 = 1 << 7 
	};

	//Forward declaration for friend statement
	class FileLog_Mgr;

	/*
	Logger template class

	Implemented with a built-in Object of the OutputPolicy class
	Logger expects very little of this Policy class itself as its methods' only use of the
	Output Object are where it is being overwritten with a new OutputPolicy object
	*/
	template <typename OutputPolicy>
	class Logger
	{
		friend FileLog_Mgr;
		public:
			virtual ~Logger()
			{
			}
			Logger(){}
			Logger(const Logger& other)
			{
				Output = other.Output;
			}
			Logger& operator=(const Logger& other)
			{
				Output = other.Output;
				return *this;
			}

			inline LogLevel& ReportingLevel()
			{
				static LogLevel reportingLevel = logDEBUG4;
				return reportingLevel;
			}
			inline std::string ToString(LogLevel level)
			{
				switch ( level )
				{
					case logFATAL:
						return "\t~FATAL~\t\t";
						break;

					case logERROR:
						return "\tERROR: \t\t";
						break;

					case logWARNING:
						return "WARNING: \t";
						break;

					case logINFO:
						return "INFO:\t  ";
						break;

					case logDEBUG1:
						return "DEBUG1:\t\t";
						break;

					case logDEBUG2:
						return "DEBUG2:\t\t  ";
						break;

					case logDEBUG3:
						return "DEBUG3:\t\t    ";
						break;

					case logDEBUG4:
						return "DEBUG4:\t\t      ";
						break;
				}
			}
			inline std::ostringstream& Get(LogLevel level = logINFO)
			{
				    buffer << std::endl << " -  " << NowTime() << " - \t";
					buffer << ToString(level);
					return buffer;
			}

		protected:
			std::ostringstream buffer;
			OutputPolicy Output;
	};

	/*
	*/
	class FileStream_Policy
	{
		public:
			virtual ~FileStream_Policy()
			{
				if ( file_stream.is_open() )
				{
					file_stream.close();
				}
			}
			FileStream_Policy(){};
			FileStream_Policy(const FileStream_Policy& other)
			{
				file_name = other.file_name;
			}
			FileStream_Policy& operator=(const FileStream_Policy& other)
			{
				file_name = other.file_name;
				return *this;
			}

			inline std::ofstream& Stream()
			{
				if ( !file_stream.is_open() )
					file_stream.open(file_name, std::ofstream::out);// | std::ofstream::app
				return file_stream;
			}
			inline std::wstring& FileName()
			{
				return file_name;
			}

		protected:
			std::ofstream file_stream;
			std::wstring file_name;
	};

	//Type Definition for a File Log using Logger<FileStream_Policy>
	using FileLog = Logger<FileStream_Policy>;

	class FileLog_Mgr
	{
		public:
			static std::ostringstream& Get(int index, LogLevel level)
			{
				try
				{
					return Logs[index].Get(level);
				}
				catch ( int exception )
				{
					assert("Indexed Log does not exist");
					exit(-404);
				}
			}
			static void RegisterNewLog(std::wstring file_name)
			{
				if ( !ThreadRunning )
				{
					FileLog newLog;
					newLog.Output.FileName() = file_name;
					Logs.push_back(newLog);
				}
			}
			static bool CheckIndex(int index)
			{
				if ( index >= 0 && index < Logs.size() )
					return true;
				return false;
			}
			static bool macroCheck(int index, LogLevel level)
			{
				if ( index >= 0 && index < Logs.size() )
				{
					if ( level > Logs[index].ReportingLevel() || !Logs[index].Output.Stream() )
						return false;
					return true;
				}
				return false;
			}
			static bool Start()
			{
				if ( WtL_Thread.joinable() )
					return false;

				ThreadRunning = true;
				WtL_Thread = std::thread(&FileLog_Mgr::WriteToLogs);
				std::this_thread::sleep_for(std::chrono::milliseconds(1500));
				return true;
			}
			static bool Stop()
			{
				if ( WtL_Thread.joinable() )
				{
					ThreadRunning = false;
					WtL_Thread.join();
					return true;
				}
				return false;
			}

		protected:
			static std::vector<FileLog> Logs;

		private:
			virtual ~FileLog_Mgr()
			{
				Stop();
			}
			static bool ThreadRunning;
			static void WriteToLogs()
			{
				while ( ThreadRunning )
				{
					for ( int i = 0; i < Logs.size(); ++i )
					{
						Logs[i].Output.Stream() << Logs[i].buffer.str();
						Logs[i].buffer.str("");
						Logs[i].Output.Stream().flush();
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(2000));
				}

				//There might be buffered data
				for ( int i = 0; i < Logs.size(); ++i )
				{
					Logs[i].Output.Stream() << Logs[i].buffer.str();
					Logs[i].buffer.str("");
					Logs[i].Output.Stream().flush();
				}
			}
			static std::thread WtL_Thread;
	};
}

#ifndef LOG_MAX_LEVEL
#define LOG_MAX_LEVEL (dbg::logFATAL + dbg::logERROR + dbg::logINFO + dbg::logDEBUG1 + dbg::logDEBUG2 + dbg::logDEBUG3 )
#endif

#define LOGFILE1(level) \
	if ( level & ~LOG_MAX_LEVEL || !dbg::FileLog_Mgr::macroCheck(0, level) ); \
	else dbg::FileLog_Mgr::Get(0, level)

#define LOGFILE2(level) \
	if ( level & ~LOG_MAX_LEVEL || !dbg::FileLog_Mgr::macroCheck(1, level) ); \
	else dbg::FileLog_Mgr::Get(1, level)

#define LOGFILE3(level) \
	if ( level & ~LOG_MAX_LEVEL || !dbg::FileLog_Mgr::macroCheck(2, level) ); \
	else dbg::FileLog_Mgr::Get(2, level)

#define LOGFILE4(level) \
	if ( level & ~LOG_MAX_LEVEL || !dbg::FileLog_Mgr::macroCheck(3, level) ); \
	else dbg::FileLog_Mgr::Get(3, level)

#define LOGFILE5(level) \
	if ( level & ~LOG_MAX_LEVEL || !dbg::FileLog_Mgr::macroCheck(4, level) ); \
	else dbg::FileLog_Mgr::Get(4, level)

#endif
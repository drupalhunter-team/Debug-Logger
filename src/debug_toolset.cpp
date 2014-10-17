#include <debug_toolset.h>

	bool dbg::FileLog_Mgr::ThreadRunning = false;
	//std::vector<dbg::FileLog> dbg::FileLog_Mgr::Logs;
	std::thread dbg::FileLog_Mgr::WtL_Thread;
	int dbg::FileLog_Mgr::WriteInterval = 2000;
	int dbg::FileLog_Mgr::StartDelay = 1500;
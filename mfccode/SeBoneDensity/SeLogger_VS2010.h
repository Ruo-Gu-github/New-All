// SeLogger_VS2010.h
// VS2010兼容版本 - 使用Windows API替代C++11特性
#pragma once

#include <windows.h>
#include <time.h>
#include <stdio.h>
#include <string>
#include <sstream>

// 日志级别
enum LogLevel {
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_DEBUG
};

// 单例日志类 - VS2010兼容版本
class SeLogger {
private:
	FILE* m_pLogFile;
	CRITICAL_SECTION m_cs;  // 使用Windows临界区替代std::mutex
	CString m_csLogPath;
	bool m_bInitialized;

	// 私有构造函数
	SeLogger() : m_pLogFile(NULL), m_bInitialized(false) {
		InitializeCriticalSection(&m_cs);
	}

	// 禁止拷贝
	SeLogger(const SeLogger&);
	SeLogger& operator=(const SeLogger&);

public:
	~SeLogger() {
		Close();
		DeleteCriticalSection(&m_cs);
	}

	// 获取单例
	static SeLogger& GetInstance() {
		static SeLogger instance;
		return instance;
	}

	// 初始化日志系统
	bool Initialize(const CString& logPath) {
		EnterCriticalSection(&m_cs);
		
		if (m_bInitialized) {
			LeaveCriticalSection(&m_cs);
			return true;
		}

		m_csLogPath = logPath;
		
		// 以追加模式打开文件，指定UTF-8编码（带BOM）
		errno_t err = _wfopen_s(&m_pLogFile, CStringW(logPath), L"a+, ccs=UTF-8");
		if (err != 0 || m_pLogFile == NULL) {
			LeaveCriticalSection(&m_cs);
			return false;
		}

		// 如果是新文件，写入UTF-8 BOM
		long fileSize = 0;
		fseek(m_pLogFile, 0, SEEK_END);
		fileSize = ftell(m_pLogFile);
		if (fileSize == 0) {
			// 写入UTF-8 BOM: EF BB BF
			unsigned char bom[3] = {0xEF, 0xBB, 0xBF};
			fwrite(bom, 1, 3, m_pLogFile);
			fflush(m_pLogFile);
		}

		m_bInitialized = true;
		LeaveCriticalSection(&m_cs);

		// 写入启动标记
		WriteLog(LOG_LEVEL_INFO, CString("========== 日志系统启动 =========="));
		return true;
	}

	// 关闭日志系统
	void Close() {
		EnterCriticalSection(&m_cs);
		
		if (m_bInitialized && m_pLogFile) {
			WriteLog(LOG_LEVEL_INFO, CString("========== 日志系统关闭 =========="));
			fclose(m_pLogFile);
			m_pLogFile = NULL;
			m_bInitialized = false;
		}
		
		LeaveCriticalSection(&m_cs);
	}

	// 通用日志写入
	void WriteLog(LogLevel level, const CString& message) {
		if (!m_bInitialized || !m_pLogFile) return;

		EnterCriticalSection(&m_cs);

		// 获取当前时间（精确到毫秒）
		SYSTEMTIME st;
		GetLocalTime(&st);

		// 格式化时间戳
		CString timestamp;
		timestamp.Format(_T("[%04d-%02d-%02d %02d:%02d:%02d.%03d]"),
			st.wYear, st.wMonth, st.wDay,
			st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

		// 日志级别
		const TCHAR* levelStr = _T("INFO ");
		switch (level) {
			case LOG_LEVEL_WARNING: levelStr = _T("WARN "); break;
			case LOG_LEVEL_ERROR:   levelStr = _T("ERROR"); break;
			case LOG_LEVEL_DEBUG:   levelStr = _T("DEBUG"); break;
		}

		// 写入日志 - 转换为Unicode
		fwprintf(m_pLogFile, L"%s [%s] %s\n", 
			CStringW(timestamp).GetString(), 
			CStringW(levelStr).GetString(), 
			CStringW(message).GetString());
		fflush(m_pLogFile);

		LeaveCriticalSection(&m_cs);
	}

	// 便捷方法
	void Info(const CString& message) {
		WriteLog(LOG_LEVEL_INFO, message);
	}

	void Warning(const CString& message) {
		WriteLog(LOG_LEVEL_WARNING, message);
	}

	void Error(const CString& message) {
		WriteLog(LOG_LEVEL_ERROR, message);
	}

	void Debug(const CString& message) {
		WriteLog(LOG_LEVEL_DEBUG, message);
	}
};

// 全局便捷宏
#define LOG_INFO(msg)     SeLogger::GetInstance().Info(CString(msg))
#define LOG_WARNING(msg)  SeLogger::GetInstance().Warning(CString(msg))
#define LOG_ERROR(msg)    SeLogger::GetInstance().Error(CString(msg))
#define LOG_DEBUG(msg)    SeLogger::GetInstance().Debug(CString(msg))

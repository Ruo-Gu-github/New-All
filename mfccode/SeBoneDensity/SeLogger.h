#pragma once
#include <string>
#include <fstream>
#include <windows.h>
#include <time.h>
#include <sys/timeb.h>
#include <sstream>
#include <iomanip>
#include <atlstr.h>

// 日志级别 (为兼容 VS2010 使用旧式 enum)
enum LogLevel {
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_DEBUG
};

// 单例日志类
class SeLogger {
public:
	// 获取单例实例
	static SeLogger& GetInstance() {
		static SeLogger instance;
		return instance;
	}

	// 初始化日志文件
	bool Initialize(const CString& logFilePath) {
		EnterCriticalSection(&m_cs);
		
		if (m_logFile.is_open()) {
			m_logFile.close();
		}

		m_logFile.open(CStringA(logFilePath), std::ios::out | std::ios::app);
		if (!m_logFile.is_open()) {
			LeaveCriticalSection(&m_cs);
			return false;
		}

		// 写入启动日志
		LeaveCriticalSection(&m_cs);
		WriteLog(LOG_LEVEL_INFO, _T("=== 日志系统启动 ==="));
		return true;
	}

	// 关闭日志文件
	void Close() {
		EnterCriticalSection(&m_cs);
		if (m_logFile.is_open()) {
			WriteLog(LOG_LEVEL_INFO, _T("=== 日志系统关闭 ==="));
			m_logFile.close();
		}
		LeaveCriticalSection(&m_cs);
	}

	// 记录加载文件操作
	void LogLoadFiles(const CString& folderPath, int fileCount) {
		CString msg;
		msg.Format("加载文件 - 文件夹: %s, 文件数量: %d", folderPath, fileCount);
	WriteLog(LOG_LEVEL_INFO, msg);
	}

	// 记录骨密度裁切操作
	void LogBoneCrop(int xs, int xe, int ys, int ye, int zs, int ze, const CString& rotationMatrix = "") {
		CString msg;
		if (rotationMatrix.IsEmpty()) {
			msg.Format("骨密度裁切 - 范围: X[%d-%d], Y[%d-%d], Z[%d-%d]", xs, xe, ys, ye, zs, ze);
		} else {
			msg.Format("骨密度裁切 - 范围: X[%d-%d], Y[%d-%d], Z[%d-%d], 旋转矩阵: %s", 
				xs, xe, ys, ye, zs, ze, rotationMatrix);
		}
	WriteLog(LOG_LEVEL_INFO, msg);
	}

	// 记录ROI操作
	void LogROI(int layerCount, int startLayer, int endLayer, const CString& roiType = _T("未指定")) {
		CString msg;
		msg.Format("ROI操作 - 类型: %s, 层数: %d, 开始层: %d, 结束层: %d", roiType, layerCount, startLayer, endLayer);
	WriteLog(LOG_LEVEL_INFO, msg);
	}

	// 记录二值化操作
	void LogBinarization(double minValue, double maxValue) {
		CString msg;
		msg.Format("二值化操作 - 最小值: %.2f, 最大值: %.2f", minValue, maxValue);
	WriteLog(LOG_LEVEL_INFO, msg);
	}

	// 记录参数计算操作
	void LogCalculation(const CString& paramList) {
		CString msg;
		msg.Format("参数计算 - 计算参数: %s", paramList);
	WriteLog(LOG_LEVEL_INFO, msg);
	}

	// 记录输出操作
	void LogExport(const CString& exportType, const CString& outputPath) {
		CString msg;
		msg.Format("输出操作 - 类型: %s, 输出路径: %s", exportType, outputPath);
	WriteLog(LOG_LEVEL_INFO, msg);
	}

	// 通用日志记录
	void Log(LogLevel level, const CString& message) {
		WriteLog(level, message);
	}

	// 记录错误
	void LogError(const CString& errorMsg) {
	WriteLog(LOG_LEVEL_ERROR, errorMsg);
	}

	// 记录警告
	void LogWarning(const CString& warningMsg) {
	WriteLog(LOG_LEVEL_WARNING, warningMsg);
	}

	// 记录调试信息
	void LogDebug(const CString& debugMsg) {
	WriteLog(LOG_LEVEL_DEBUG, debugMsg);
	}

private:
	SeLogger() {
		InitializeCriticalSection(&m_cs);
	}
	~SeLogger() {
		Close();
		DeleteCriticalSection(&m_cs);
	}

	// 禁止拷贝和赋值
	SeLogger(const SeLogger&);
	SeLogger& operator=(const SeLogger&);

	// 获取当前时间戳字符串
	std::string GetTimestamp() {
		struct _timeb timebuffer;
		char timeline[26];
		_ftime_s(&timebuffer);
		ctime_s(timeline, 26, &timebuffer.time);
		
		// Remove newline from ctime_s output
		timeline[24] = '\0';
		
		std::ostringstream oss;
		oss << timeline;
		oss << '.' << std::setfill('0') << std::setw(3) << timebuffer.millitm;
		return oss.str();
	}

	// 获取日志级别字符串
	std::string GetLevelString(LogLevel level) {
		switch (level) {
		case LOG_LEVEL_INFO:    return "[INFO]   ";
		case LOG_LEVEL_WARNING: return "[WARNING]";
		case LOG_LEVEL_ERROR:   return "[ERROR]  ";
		case LOG_LEVEL_DEBUG:   return "[DEBUG]  ";
		default:                return "[UNKNOWN]";
		}
	}

	// 写入日志
	void WriteLog(LogLevel level, const CString& message) {
		EnterCriticalSection(&m_cs);
		
		if (!m_logFile.is_open()) {
			LeaveCriticalSection(&m_cs);
			return;
		}

		std::string timestamp = GetTimestamp();
		std::string levelStr = GetLevelString(level);
		std::string msg = CStringA(message).GetString();

		m_logFile << timestamp << " " << levelStr << " " << msg << std::endl;
		m_logFile.flush(); // 立即刷新到文件
		
		LeaveCriticalSection(&m_cs);
	}

	std::ofstream m_logFile;
	CRITICAL_SECTION m_cs;
};

// 便捷宏定义
#define LOG_INFO(msg)       SeLogger::GetInstance().Log(LOG_LEVEL_INFO, msg)
#define LOG_WARNING(msg)    SeLogger::GetInstance().LogWarning(msg)
#define LOG_ERROR(msg)      SeLogger::GetInstance().LogError(msg)
#define LOG_DEBUG(msg)      SeLogger::GetInstance().LogDebug(msg)

#define LOG_LOAD_FILES(folder, count)              SeLogger::GetInstance().LogLoadFiles(folder, count)
#define LOG_BONE_CROP(xs, xe, ys, ye, zs, ze)      SeLogger::GetInstance().LogBoneCrop(xs, xe, ys, ye, zs, ze)
#define LOG_ROI(count, start, end, type)           SeLogger::GetInstance().LogROI(count, start, end, type)
#define LOG_BINARIZATION(min, max)                 SeLogger::GetInstance().LogBinarization(min, max)
#define LOG_CALCULATION(params)                    SeLogger::GetInstance().LogCalculation(params)
#define LOG_EXPORT(type, path)                     SeLogger::GetInstance().LogExport(type, path)

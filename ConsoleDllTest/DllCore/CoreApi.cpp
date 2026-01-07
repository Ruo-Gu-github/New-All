#include "pch.h"
#include "CoreApi.h"
#include <string>
#include <fstream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <map>
#include <ctime>

static thread_local std::string g_lastError;

static void SetLastError(const std::string& error) {
    g_lastError = error;
}

const char* Core_GetLastError() {
    return g_lastError.c_str();
}

static size_t g_totalAllocated = 0;
static size_t g_peakUsage = 0;
static std::mutex g_memMutex;

void* Core_Malloc(size_t size) {
    std::lock_guard<std::mutex> lock(g_memMutex);
    void* ptr = malloc(size);
    if (ptr) {
        g_totalAllocated += size;
        if (g_totalAllocated > g_peakUsage) {
            g_peakUsage = g_totalAllocated;
        }
    }
    return ptr;
}

void Core_Free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

void Core_Memcpy(void* dest, const void* src, size_t size) {
    memcpy(dest, src, size);
}

NativeResult Core_GetMemoryStats(size_t* totalAllocated, size_t* peakUsage) {
    if (!totalAllocated || !peakUsage) {
        SetLastError("Invalid arguments");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(g_memMutex);
    *totalAllocated = g_totalAllocated;
    *peakUsage = g_peakUsage;
    return NATIVE_OK;
}

struct Logger {
    std::ofstream file;
    LogLevel minLevel;
    std::mutex mutex;
    bool initialized;
    Logger() : minLevel(LOG_LEVEL_INFO), initialized(false) {}
};

static Logger g_logger;

NativeResult Core_InitLogger(const char* logFilePath, LogLevel minLevel) {
    if (!logFilePath) {
        SetLastError("Invalid log file path");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(g_logger.mutex);
    g_logger.file.open(logFilePath, std::ios::out | std::ios::app);
    if (!g_logger.file.is_open()) {
        SetLastError("Failed to open log file");
        return NATIVE_E_INTERNAL_ERROR;
    }
    g_logger.minLevel = minLevel;
    g_logger.initialized = true;
    return NATIVE_OK;
}

void Core_Log(LogLevel level, const char* message) {
    if (!g_logger.initialized || !message) return;
    if (level < g_logger.minLevel) return;
    const char* levelStr[] = { "DEBUG", "INFO", "WARNING", "ERROR" };
    std::lock_guard<std::mutex> lock(g_logger.mutex);
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    char timeStr[100];
    struct tm timeInfo;
    localtime_s(&timeInfo, &time);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeInfo);
    g_logger.file << "[" << timeStr << "] [" << levelStr[level] << "] " << message << std::endl;
    g_logger.file.flush();
}

void Core_ShutdownLogger() {
    std::lock_guard<std::mutex> lock(g_logger.mutex);
    if (g_logger.file.is_open()) {
        g_logger.file.close();
    }
    g_logger.initialized = false;
}

struct Task {
    TaskCallback callback;
    void* userData;
};

struct ThreadPool {
    std::vector<std::thread> workers;
    std::queue<Task> tasks;
    std::mutex mutex;
    std::condition_variable condition;
    bool stop;
    int activeTasks;
    ThreadPool(int threadCount) : stop(false), activeTasks(0) {
        for (int i = 0; i < threadCount; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    Task task;
                    {
                        std::unique_lock<std::mutex> lock(this->mutex);
                        this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty()) return;
                        task = this->tasks.front();
                        this->tasks.pop();
                        this->activeTasks++;
                    }
                    task.callback(task.userData);
                    {
                        std::lock_guard<std::mutex> lock(this->mutex);
                        this->activeTasks--;
                    }
                    this->condition.notify_all();
                }
            });
        }
    }
    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers) {
            worker.join();
        }
    }
};

ThreadPoolHandle Core_CreateThreadPool(int threadCount) {
    if (threadCount <= 0) threadCount = std::thread::hardware_concurrency();
    return new ThreadPool(threadCount);
}

void Core_DestroyThreadPool(ThreadPoolHandle handle) {
    if (handle) delete static_cast<ThreadPool*>(handle);
}

NativeResult Core_SubmitTask(ThreadPoolHandle handle, TaskCallback callback, void* userData) {
    if (!handle || !callback) {
        SetLastError("Invalid arguments");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    ThreadPool* pool = static_cast<ThreadPool*>(handle);
    {
        std::lock_guard<std::mutex> lock(pool->mutex);
        pool->tasks.push({ callback, userData });
    }
    pool->condition.notify_one();
    return NATIVE_OK;
}

NativeResult Core_WaitAllTasks(ThreadPoolHandle handle) {
    if (!handle) {
        SetLastError("Invalid handle");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    ThreadPool* pool = static_cast<ThreadPool*>(handle);
    std::unique_lock<std::mutex> lock(pool->mutex);
    pool->condition.wait(lock, [pool] { return pool->tasks.empty() && pool->activeTasks == 0; });
    return NATIVE_OK;
}

NativeResult Core_GetThreadPoolStats(ThreadPoolHandle handle, int* queuedTasks, int* activeTasks) {
    if (!handle || !queuedTasks || !activeTasks) {
        SetLastError("Invalid arguments");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    ThreadPool* pool = static_cast<ThreadPool*>(handle);
    std::lock_guard<std::mutex> lock(pool->mutex);
    *queuedTasks = static_cast<int>(pool->tasks.size());
    *activeTasks = pool->activeTasks;
    return NATIVE_OK;
}

struct Timer {
    std::chrono::high_resolution_clock::time_point startTime;
    bool running;
    Timer() : running(false) {}
};

TimerHandle Core_CreateTimer() {
    return new Timer();
}

void Core_DestroyTimer(TimerHandle handle) {
    if (handle) delete static_cast<Timer*>(handle);
}

void Core_StartTimer(TimerHandle handle) {
    if (handle) {
        Timer* timer = static_cast<Timer*>(handle);
        timer->startTime = std::chrono::high_resolution_clock::now();
        timer->running = true;
    }
}

double Core_StopTimer(TimerHandle handle) {
    if (!handle) return 0.0;
    Timer* timer = static_cast<Timer*>(handle);
    if (!timer->running) return 0.0;
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - timer->startTime);
    timer->running = false;
    return duration.count() / 1000.0;
}

struct Config {
    std::map<std::string, std::string> values;
};

ConfigHandle Core_LoadConfig(const char* filepath) {
    if (!filepath) {
        SetLastError("Invalid file path");
        return nullptr;
    }
    std::ifstream file(filepath);
    if (!file.is_open()) {
        SetLastError("Failed to open config file");
        return nullptr;
    }
    Config* config = new Config();
    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            config->values[key] = value;
        }
    }
    return config;
}

void Core_DestroyConfig(ConfigHandle handle) {
    if (handle) delete static_cast<Config*>(handle);
}

const char* Core_GetConfigString(ConfigHandle handle, const char* key, const char* defaultValue) {
    if (!handle || !key) return defaultValue;
    Config* config = static_cast<Config*>(handle);
    auto it = config->values.find(key);
    if (it != config->values.end()) {
        return it->second.c_str();
    }
    return defaultValue;
}

int Core_GetConfigInt(ConfigHandle handle, const char* key, int defaultValue) {
    const char* value = Core_GetConfigString(handle, key, nullptr);
    if (value) return std::atoi(value);
    return defaultValue;
}

float Core_GetConfigFloat(ConfigHandle handle, const char* key, float defaultValue) {
    const char* value = Core_GetConfigString(handle, key, nullptr);
    if (value) return static_cast<float>(std::atof(value));
    return defaultValue;
}

bool Core_GetConfigBool(ConfigHandle handle, const char* key, bool defaultValue) {
    const char* value = Core_GetConfigString(handle, key, nullptr);
    if (value) {
        std::string str(value);
        return (str == "true" || str == "1" || str == "yes");
    }
    return defaultValue;
}

// ==================== Win32 窗口管理 ====================
#include <Windows.h>

NativeResult Core_SetWindowTopmost(void* hwnd, bool topmost) {
    if (!hwnd) {
        SetLastError("Invalid window handle");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    HWND hWnd = static_cast<HWND>(hwnd);
    HWND insertAfter = topmost ? HWND_TOPMOST : HWND_NOTOPMOST;
    
    // 使用 SetWindowPos 设置窗口 z-order
    // SWP_NOMOVE | SWP_NOSIZE: 不改变位置和大小，只改变 z-order
    BOOL result = SetWindowPos(hWnd, insertAfter, 0, 0, 0, 0, 
                               SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    
    if (!result) {
        SetLastError("SetWindowPos failed");
        return NATIVE_E_INTERNAL_ERROR;
    }
    
    return NATIVE_OK;
}

const char* Core_GetVersion() {
    return "1.0.0";
}

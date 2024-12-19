// clang-format off
#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

#include "logfila.h"
// clang-format on
// 防止自己记自己,受到mutex保护
static int __is_in_logging = 1;

// 记录等级
static int __Logging_start_level = LEVEL_TRACE;

// 存储扩展日志处理组件
static FunctionPointer logWares[MAX_LOG_WARES];
static int logWareCount = 0;

/**
 * @brief 日志级别字符串
 */
const char *LogLevelStrings[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL", "PANIC"};

/**
 * sync.Mutex
 */

// 定义一个静态变量作为锁
static int mutex = 0;

// 原子测试并设置的简单模拟（用在 lock 函数中）
static inline int test_and_set(int *lock) {
	int old_value = *lock;
	*lock         = 1; // 设置为被锁状态
	return old_value;  // 返回之前的值
}

// lock 函数：获取锁
void lock() {
	while (1) {
		// 如果 mutex 为 0，则抢占锁（原子操作）
		if (test_and_set(&mutex) == 0) {
			break; // 成功获取锁，退出循环
		}
		// 自旋等待（模拟忙等待）
	}
}

// unlock 函数：释放锁
void unlock() {
	mutex = 0; // 释放锁
}

////////////////////////////////////

/**
 * @brief 日志函数入口实现
 *
 * @param[in] stage: 阶段
 * @param[in] level: 日志级别
 * @param[in] fmt: 需要输出的格式化字符串
 * @param[in] ...: 格式化字符
 */
void LogFuncEntry(char *stage, enum LogLevels level, char *fmt, ...) {
	if (level < __Logging_start_level) {
		return;
	}
	lock();
	if (__is_in_logging) {
		unlock();
		return;
	}
	__is_in_logging |= 1;
	unlock();

	if (level < LEVEL_TRACE || level > LEVEL_PANIC) {
		printl("Invalid log level!\n");
		return;
	}

	// 获取可变参数
	va_list args       = (va_list)((char *)(&fmt) + 4);
	char buf[128]      = ""; // log 字符串
	char full_buf[128] = "";
	_vsprintf(buf, fmt, args);
	_sprintf(full_buf, "[%s] [%s]: %s\n", LogLevelStrings[level - LEVEL_TRACE], stage, buf);

	// 传递给所有扩展日志处理器
	for (int i = 0; i < logWareCount; i++) {
		if (logWares[i]) {
			logWares[i](stage, level, full_buf);
		}
	}

	lock();
	__is_in_logging &= 0;
	unlock();
}

/**
 * @brief 增加日志处理部件
 *
 * @param[in] func: 额外的日志组件
 */
void AddLogWare(FunctionPointer func) {
	if (logWareCount < MAX_LOG_WARES) {
		logWares[logWareCount++] = func;
	} else {
		// make sure we will not exceed this macro number :) XD
		// panic("can not add more logware");
	}
}

void DefaultLogHandler(char *stage, enum LogLevels level, char *str) {
	printl("{test}: %s", str);
}

void DiskLogHandler(char *stage, enum LogLevels level, char *str) {
	MESSAGE msg;
	msg.u.m3.m3p1 = str;
	msg.u.m3.m3i1 = strlen(str);
	msg.type      = 1;

	send_recv(SEND, TASK_LOGS, &msg);
}

/**
 * @brief 初始化日志处理器
 */
void InitLogWares() {
	__Logging_start_level = LEVEL_INFO;
	for (int i = 0; i < MAX_LOG_WARES; i++) {
		logWares[i] = NULL;
	}
	logWareCount    = 0;
	__is_in_logging = 0;

	// AddLogWare(DefaultLogHandler);
	AddLogWare(DiskLogHandler);
}

void DisableLOGGING() {
	// lock();
	__is_in_logging &= 1;
	// unlock();
}

void EnableLOGGING() {
	// lock();
	__is_in_logging &= 0;
	// unlock();
}

void SwitchLogLevel(int level) {
	__Logging_start_level = level;
}
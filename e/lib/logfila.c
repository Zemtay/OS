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

// 存储扩展日志处理组件
static FunctionPointer logWares[MAX_LOG_WARES];
static int logWareCount = 0;

/**
 * @brief 日志级别字符串
 */
const char *LogLevelStrings[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL", "PANIC"};

/**
 * @brief 日志函数入口实现
 *
 * @param[in] stage: 阶段
 * @param[in] level: 日志级别
 * @param[in] fmt: 需要输出的格式化字符串
 * @param[in] ...: 格式化字符
 */
void LogFuncEntry(char *stage, enum LogLevels level, char *fmt, ...) {
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
		panic("can not add more logware");
	}
}

void DefaultLogHandler(char *stage, enum LogLevels level, char *str) {
	if (level >= LEVEL_TRACE)
		printl("{test}: %s", str);
}

// void DiskLogHandler(char *stage, enum LogLevels level, char *str) {

// }

/**
 * @brief 初始化日志处理器
 */
void InitLogWares() {
	for (int i = 0; i < MAX_LOG_WARES; i++) {
		logWares[i] = NULL;
	}
	logWareCount = 0;

	// 添加默认的日志处理器（示例）
	AddLogWare(DefaultLogHandler);
}
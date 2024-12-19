/**
 * @file logfila.h
 * @brief 提供C语言中业务逻辑无关的日志中间件和可扩展日志逻辑处理。
 *
 * @details
 * 1. 该日志系统支持动态注册日志组件（日志处理器）。
 * 2. 用户可以通过调用LogFuncEntry输出日志，同时扩展自定义日志处理逻辑。
 * 3. 提供日志级别控制，支持多阶段、多组件日志处理。
 *
 * @author scarletborder
 * @version 1.0
 * @date 2024-12-15
 */

#ifndef LOGFILA_H
#define LOGFILA_H

// #include <stdarg.h>

#define MAX_LOG_WARES 8  // 支持的最大日志处理器数

/**
 * @brief 日志级别枚举类型，数值越大表示日志级别越紧急。
 */
enum LogLevels {
	LEVEL_TRACE = 1, ///< 详细的跟踪日志
	LEVEL_DEBUG, ///< 调试日志
	LEVEL_INFO,      ///< 信息日志
	LEVEL_WARN, ///< 警告日志
	LEVEL_ERROR,     ///< 错误日志
	LEVEL_FATAL,     ///< 致命错误日志
	LEVEL_PANIC      ///< 崩溃日志
};

/**
 * @brief 日志处理器函数指针类型。
 *
 * @param[in] stage 日志的阶段。
 * @param[in] level 日志级别。
 * @param[in] str   格式化字符串。
 */
typedef void (*FunctionPointer)(char *stage, enum LogLevels level, char *str);

/**
 * @brief 日志函数入口。
 *
 * @details 该函数为日志的统一入口，可以输出标准日志，
 * 并调用所有已注册的日志处理器，实现扩展功能。
 *
 * @param[in] stage 阶段标识符，描述日志所属的阶段或组件。
 * @param[in] level 日志级别，表示日志的重要程度。
 * @param[in] fmt   需要输出的格式化字符串。
 * @param[in] ...   可变参数列表，用于格式化输出。
 *
 * @note
 * - 如果级别无效，该函数将忽略日志输出。
 * 
 * - 用户可以通过AddLogWare函数注册额外的日志处理器。
 */
void LogFuncEntry(char *stage, enum LogLevels level, char *fmt, ...);

/**
 * @brief 注册额外的日志处理组件。
 *
 * @details 用户可以通过该接口注册自定义的日志逻辑，
 * 例如将日志输出到文件、网络或其他存储介质。
 *
 * @param[in] func 自定义的日志处理器函数，
 * 该函数需符合FunctionPointer类型签名。
 *
 * @note
 * - 系统最多支持注册8(MAX_LOG_WARES)个额外的日志处理器。
 * 
 * - 注册成功后，LogFuncEntry会调用所有已注册的处理器。
 */
void AddLogWare(FunctionPointer func);

void InitLogWares();

void DisableLOGGING();

void EnableLOGGING();

void SwitchLogLevel(int level);
#endif // LOGFILA_H

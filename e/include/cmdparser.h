#ifndef _SCARLETBORDER_CMDPARSER_H_
#define _SCARLETBORDER_CMDPARSER_H_

// 定义选项类型
typedef enum {
	OPT_TYPE_FLAG, // 无值的选项（如 -l）
	OPT_TYPE_VALUE // 需要值的选项（如 -o value）
} OptionType;

// 定义选项结构
typedef struct {
	const char *short_opt; // 短选项，如 "-l"
	const char *long_opt;  // 长选项，如 "--long"
	OptionType type;       // 选项类型
	void *value;           // 存储结果的指针
} CmdOption;

// 定义全局解析状态
typedef struct {
	const char *positional_args[16]; // 固定大小数组存储位置参数
	int positional_count;            // 位置参数数量
} CmdParserResult;

/**
 * @brief 解析命令行参数并填充选项和位置参数。
 *
 * 该函数处理传递给程序的命令行参数，根据用户提供的选项列表匹配选项，
 * 并将选项和位置参数分类存储。支持布尔类型的标志选项（如 `-l`）和需要值的选项（如 `-o value`）。
 *
 * @param argc 命令行参数的数量，包括程序名。
 * @param argv 一个字符串数组，存储命令行参数。
 * @param options 一个 `CmdOption` 结构数组，定义支持的选项。
 *                每个 `CmdOption` 包含短选项/长选项名称、选项类型以及存储解析结果的指针。
 * @param opt_count `options` 数组的长度（选项数量）。
 * @param result 一个指向 `CmdParserResult` 结构的指针，用于存储位置参数。
 *               `positional_args` 字段将被填充为位置参数，`positional_count` 表示位置参数的数量。
 *
 * @return 返回 0 表示解析成功。如果解析失败（例如遇到未知选项、缺少必需值或位置参数超出限制），
 *         返回 -1 并在终端打印错误信息。
 *
 * @note 该函数不使用动态内存分配。`CmdParserResult` 的 `positional_args` 是一个固定大小的数组，
 *       最多支持 16 个位置参数。如果位置参数数量超过此限制，则返回错误。
 *
 * @warning 确保在调用此函数之前正确初始化 `options` 数组以及对应的 `value` 指针。
 *
 * @example 示例用法：
 * @code
 * int option_l = 0;
 * char *output_file = NULL;
 * CmdOption options[] = {
 *     {"-l", "--long", OPT_TYPE_FLAG, &option_l},
 *     {"-o", "--output", OPT_TYPE_VALUE, &output_file}
 * };
 * CmdParserResult result;
 * if (parse_cmdline(argc, argv, options, 2, &result) != 0) {
 *     printf("解析命令行参数时出错。\n");
 *     return 1;
 * }
 * printf("选项 -l: %d\n", option_l);
 * printf("选项 -o: %s\n", output_file);
 * for (int i = 0; i < result.positional_count; i++) {
 *     printf("位置参数: %s\n", result.positional_args[i]);
 * }
 * @endcode
 */
int parse_cmdline(int argc, char *argv[], CmdOption *options, int opt_count, CmdParserResult *result);

#endif
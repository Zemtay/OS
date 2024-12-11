// clang-format off

/*************************************************************************//**
 *****************************************************************************
 * @file   misc.c
 * @brief
 * @author Forrest Y. Yu
 * @date   2008
 *****************************************************************************
 *****************************************************************************/

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

#ifndef NULL
#define NULL 0
#endif

/*****************************************************************************
 *                                send_recv
 *****************************************************************************/
/**
 * <Ring 1~3> IPC syscall.
 *
 * It is an encapsulation of `sendrec',
 * invoking `sendrec' directly should be avoided
 *
 * @param function  SEND, RECEIVE or BOTH
 * @param src_dest  The caller's proc_nr
 * @param msg       Pointer to the MESSAGE struct
 * 
 * @return always 0.
 *****************************************************************************/
PUBLIC int send_recv(int function, int src_dest, MESSAGE* msg)
{
	int ret = 0;

	if (function == RECEIVE)
		memset(msg, 0, sizeof(MESSAGE));

	switch (function) {
	case BOTH:
		ret = sendrec(SEND, src_dest, msg);
		if (ret == 0)
			{
				// DEBUG_PRINT("send_recv", "111 src_dest: %d, I am %d", src_dest, msg->source);
				ret = sendrec(RECEIVE, src_dest, msg);
				// DEBUG_PRINT("send_recv", "222 src_dest: %d, I am %d", src_dest, msg->source);
			}
		break;
	case SEND:
	case RECEIVE:
		ret = sendrec(function, src_dest, msg);
		break;
	default:
		assert((function == BOTH) ||
		       (function == SEND) || (function == RECEIVE));
		break;
	}

	return ret;
}

/*****************************************************************************
 *                                memcmp
 *****************************************************************************/
/**
 * Compare memory areas.
 * 
 * @param s1  The 1st area.
 * @param s2  The 2nd area.
 * @param n   The first n bytes will be compared.
 * 
 * @return  an integer less than, equal to, or greater than zero if the first
 *          n bytes of s1 is found, respectively, to be less than, to match,
 *          or  be greater than the first n bytes of s2.
 *****************************************************************************/
PUBLIC int memcmp(const void * s1, const void *s2, int n)
{
	if ((s1 == 0) || (s2 == 0)) { /* for robustness */
		return (s1 - s2);
	}

	const char * p1 = (const char *)s1;
	const char * p2 = (const char *)s2;
	int i;
	for (i = 0; i < n; i++,p1++,p2++) {
		if (*p1 != *p2) {
			return (*p1 - *p2);
		}
	}
	return 0;
}

/*****************************************************************************
 *                                strcmp
 *****************************************************************************/
/**
 * Compare two strings.
 * 
 * @param s1  The 1st string.
 * @param s2  The 2nd string.
 * 
 * @return  an integer less than, equal to, or greater than zero if s1 (or the
 *          first n bytes thereof) is  found,  respectively,  to  be less than,
 *          to match, or be greater than s2.
 *****************************************************************************/
PUBLIC int strcmp(const char * s1, const char *s2)
{
	if ((s1 == 0) || (s2 == 0)) { /* for robustness */
		return (s1 - s2);
	}

	const char * p1 = s1;
	const char * p2 = s2;

	for (; *p1 && *p2; p1++,p2++) {
		if (*p1 != *p2) {
			break;
		}
	}

	return (*p1 - *p2);
}

/*****************************************************************************
 *                                strcat
 *****************************************************************************/
/**
 * Concatenate two strings.
 * 
 * @param s1  The 1st string.
 * @param s2  The 2nd string.
 * 
 * @return  Ptr to the 1st string.
 *****************************************************************************/
PUBLIC char * strcat(char * s1, const char *s2)
{
	if ((s1 == 0) || (s2 == 0)) { /* for robustness */
		return 0;
	}

	char * p1 = s1;
	for (; *p1; p1++) {}

	const char * p2 = s2;
	for (; *p2; p1++,p2++) {
		*p1 = *p2;
	}
	*p1 = 0;

	return s1;
}

/*****************************************************************************
 *                                spin
 *****************************************************************************/
PUBLIC void spin(char * func_name)
{
	printl("\nspinning in %s ...\n", func_name);
	while (1) {}
}

char *strchr(const char *str, int c) {
    // 遍历字符串，查找字符 c
    while (*str != '\0') {
        if (*str == c) {
            return (char *)str;  // 找到字符 c，返回当前字符的地址
        }
        str++;  // 移动到下一个字符
    }
    
    // 如果没有找到字符 c，返回 NULL
    return NULL;
}

/**
 * strtok
 */
char *strtok(char *str, const char *delim) {
    static char *next_token = NULL;  // 保存上次分割的位置

    if (str != NULL) {
        // 如果是第一次调用，初始化
        next_token = str;
    }

    if (next_token == NULL) {
        return NULL;  // 如果没有可分割的字符串，则返回 NULL
    }

    // 跳过分隔符
    char *start = next_token;
    while (*start && strchr(delim, *start)) {
        start++;  // 跳过前导的分隔符
    }

    if (*start == '\0') {
        next_token = NULL;  // 如果已经到达字符串的结尾，返回 NULL
        return NULL;
    }

    // 查找下一个分隔符
    char *end = start;
    while (*end && !strchr(delim, *end)) {
        end++;  // 查找当前子字符串的结束
    }

    if (*end == '\0') {
        // 如果已经到达字符串的末尾，更新 next_token 为 NULL
        next_token = NULL;
    } else {
        // 否则，将分隔符替换为 '\0'
        *end = '\0';
        next_token = end + 1;
    }

    return start;  // 返回当前子字符串
}

/**
 * strcpy
 */
char *_strcpy(char *dest, char *src) {
    // 用指针逐个复制字符
    char *d = dest;
	char *s = src;

    // 逐个字符复制，直到遇到字符串结束符 '\0'
    while (*s != '\0') {
        *d = *s;  // 复制当前字符
        d++;         // 移动目标指针
        s++;       // 移动源指针
    }

    // 复制结束符 '\0'
    *d = '\0';

    return dest;  // 返回目标字符串的起始地址
}

char *_strncpy(char *dest, const char *src, size_t n) {
    size_t i;

    // 逐字符复制，直到到达 n 或 src 的末尾
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }

    // 如果 src 长度不足 n，则用 '\0' 填充剩余空间
    for (; i < n; i++) {
        dest[i] = '\0';
    }

    return dest;
}

PUBLIC  void memmove(void* dst, void* src, int size){
    char* d = (char*)dst;
    char* s = (char*)src;

    if (d < s) {
        // 前向复制
        for (int i = 0; i < size; i++) {
            d[i] = s[i];
        }
    } else if (d > s) {
        // 后向复制
        for (int i = size - 1; i >= 0; i--) {
            d[i] = s[i];
        }
    }
    // 如果 dst == src，无需复制
}


PUBLIC  void proc_memcpy(void* p_dst, int pid_dst, void* p_src, int pid_src, int len){
	_strcpy(p_dst, p_src);
	// MESSAGE mm_msg;
	// mm_msg.u.m3.m3i1 = pid_dst;
	// mm_msg.u.m3.m3i2 = pid_src;
	// mm_msg.u.m3.m3p1 = (void*)p_dst;
	// mm_msg.u.m3.m3p2 = (void*)p_src;
	// mm_msg.u.m3.m3i3 = len;

	// mm_msg.type = TYPESHELL;

	// send_recv(SEND, TASK_MM, &mm_msg);
}

/**
 * @brief Converts a string to an integer.
 * 
 * @param str The string to be converted.
 * @return The integer representation of the string.
 */
PUBLIC int atoi(const char *str) {
    int result = 0;  // Initialize result
    int sign = 1;    // Initialize sign as positive
    
    // Handle optional whitespace characters
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r' || *str == '\f' || *str == '\v') {
        str++;
    }

    // Handle optional sign
    if (*str == '-') {
        sign = -1;  // If negative sign, update sign
        str++;
    } else if (*str == '+') {
        str++;  // If positive sign, move to the next character
    }

    // Convert the string to an integer
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');  // Update result for each digit
        str++;
    }

    return result * sign;  // Return result with sign
}



/*****************************************************************************
 *                           assertion_failure
 *************************************************************************//**
 * Invoked by assert().
 *
 * @param exp       The failure expression itself.
 * @param file      __FILE__
 * @param base_file __BASE_FILE__
 * @param line      __LINE__
 *****************************************************************************/
PUBLIC void assertion_failure(char *exp, char *file, char *base_file, int line)
{
	printl("%c  assert(%s) failed: file: %s, base_file: %s, ln%d",
	       MAG_CH_ASSERT,
	       exp, file, base_file, line);

	/**
	 * If assertion fails in a TASK, the system will halt before
	 * printl() returns. If it happens in a USER PROC, printl() will
	 * return like a common routine and arrive here. 
	 * @see sys_printx()
	 * 
	 * We use a forever loop to prevent the proc from going on:
	 */
	spin("assertion_failure()");

	/* should never arrive here */
        __asm__ __volatile__("ud2");
}

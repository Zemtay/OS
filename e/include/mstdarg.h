#ifndef _SCARLETBORDER_STDARG_H_
#define _SCARLETBORDER_STDARG_H_

typedef char *va_list;

#define va_start(ap, param) ((ap) = (va_list)(&(param) + 1))
#define va_arg(ap, type) (*(type *)((ap) += sizeof(type), (ap) - sizeof(type)))
#define va_end(ap) ((ap) = NULL)

#endif

#ifndef NULL
#define NULL 0
#endif
#ifndef _DATATYPE_H_
#define _DATATYPE_H_

typedef int             BOOL;
typedef unsigned char   BOOLEAN;
typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned long   DWORD;
typedef long 			LONG;

typedef unsigned char	UINT8;
typedef signed char		SINT8;
typedef unsigned short	UINT16;
typedef signed short	SINT16;
typedef unsigned int	UINT32;
typedef signed int		SINT32;
typedef unsigned short	WCHAR;

typedef signed char     int8;
typedef signed short    int16;
typedef long            int32;
typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned long   uint32;
typedef long long		int64;
typedef unsigned long long uint64;

#define MG_OK					0
#define MG_ERR_LIST_IS_EMPTY	1
#define MG_ERR_MEM				2

typedef void * 			HANDLE;
typedef UINT32 			MG_BOOL;
#define MG_FALSE		0
#define MG_TRUE			1

#endif

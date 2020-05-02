#ifndef FIBDRV_FIBDRV_H_
#define FIBDRV_FIBDRV_H_

#if defined(__SIZEOF_INT128__) && defined(F_LARGE_NUMBER)
typedef unsigned __int128 f_uint;

#define MAX_LENGTH 100

#else /* __SIZEOF_INT128__ */
/* MAX_LENGTH is set to 92 because
 * ssize_t can't fit the number > 92
 */
typedef unsigned long long f_uint;

#define MAX_LENGTH 92
#endif /* __SIZEOF_INT128__ */


#endif /* FIBDRV_FIBDRV_H_ */

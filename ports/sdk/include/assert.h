/* assert, reporting through libaux. Not guarded: NDEBUG may change between includes. */
#undef assert
#ifdef NDEBUG
#define assert(e)	((void)0)
#else
void __aux_assert(const char *expr, const char *file, int line, const char *func)
	__attribute__((noreturn));
#define assert(e)	((e) ? (void)0 : __aux_assert(#e, __FILE__, __LINE__, __func__))
#endif

#if __STDC_VERSION__ >= 201112L && !defined(__cplusplus) && !defined(static_assert)
#define static_assert	_Static_assert
#endif

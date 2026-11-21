/* WASI lacks realpath. Fixtures use canonical absolute paths only.
   Production Linux builds use libc realpath without this substitution. */
char *fixture_realpath(const char *,char *);
#define realpath fixture_realpath

/* WASI lacks mkstemp. The test directory belongs exclusively to this test. */
int fixture_mkstemp(char *name);
#define mkstemp fixture_mkstemp


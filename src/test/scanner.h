#ifndef TEST_SCANNER_H
#define TEST_SCANNER_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
	const char *src;
	size_t cur, len;
} NojaTestScanner;

bool noMoreCharacters(NojaTestScanner *scanner);
bool followsCharacter(NojaTestScanner *scanner, char c);
bool followsCharacterThat(NojaTestScanner *scanner, int (*testfn)(int c));
void consumeCharactersThat(NojaTestScanner *scanner, int (*testfn)(int c));
bool ifFollowsConsumeCharacter(NojaTestScanner *scanner, char c);
/*
#ifndef NDEBUG
bool noMoreCharacters__wrapped(NojaTestScanner *scanner, const char *file, size_t line);
bool followsCharacter__wrapped(NojaTestScanner *scanner, char c, const char *file, size_t line);
bool followsCharacterThat__wrapped(NojaTestScanner *scanner, int (*testfn)(int c), const char *file, size_t line);
void consumeCharactersThat__wrapped(NojaTestScanner *scanner, int (*testfn)(int c), const char *file, size_t line);
bool ifFollowsConsumeCharacter__wrapped(NojaTestScanner *scanner, char c, const char *file, size_t line);

#define noMoreCharacters(scanner) noMoreCharacters__wrapped(scanner, __FILE__, __LINE__)
#define followsCharacter(scanner, c) followsCharacter__wrapped(scanner, c, __FILE__, __LINE__)
#define followsCharacterThat(scanner, testfn) followsCharacterThat__wrapped(scanner, testfn, __FILE__, __LINE__)
#define consumeCharactersThat(scanner, testfn) consumeCharacterThat__wrapped(scanner, testfn, __FILE__, __LINE__)
#define ifFollowsConsumeCharacter(scanner, c) ifFollowsConsumeCharacter__wrapped(scanner, c, __FILE__, __LINE__)
#endif
*/
#endif
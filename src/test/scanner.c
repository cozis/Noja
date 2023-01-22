#include "scanner.h"

bool followsCharacterThat(NojaTestScanner *scanner, int (*testfn)(int c))
{
	return scanner->cur < scanner->len && testfn(scanner->src[scanner->cur]);
}

bool followsCharacter(NojaTestScanner *scanner, char c)
{
	return scanner->cur < scanner->len && scanner->src[scanner->cur] == c;
}

bool noMoreCharacters(NojaTestScanner *scanner)
{
	return scanner->cur == scanner->len;
}

bool ifFollowsConsumeCharacter(NojaTestScanner *scanner, char c)
{
	bool follows = followsCharacter(scanner, c);
	if (follows)
		scanner->cur++;
	return follows;
}

void consumeCharactersThat(NojaTestScanner *scanner, int (*testfn)(int c))
{
	while (followsCharacterThat(scanner, testfn))
		scanner->cur++;
}
/*
#ifndef NDEBUG

bool noMoreCharacters__wrapped(NojaTestScanner *scanner, const char *file, size_t line)
{
	fprintf(stderr, "noMoreCharacters(%p) - %s:%ld\n", scanner, file, line);
	return noMoreCharacters(scanner);
}

bool followsCharacter__wrapped(NojaTestScanner *scanner, char c, const char *file, size_t line)
{
	fprintf(stderr, "followsCharacter(%p, '%c') - %s:%ld\n", scanner, c, file, line);
	return followsCharacter(scanner, c);
}

bool followsCharacterThat__wrapped(NojaTestScanner *scanner, int (*testfn)(int c), const char *file, size_t line)
{
	fprintf(stderr, "followsCharacterThat(%p, %p) - %s:%ld\n", scanner, testfn, file, line);
	return followsCharacterThat(scanner);
}

void consumeCharactersThat__wrapped(NojaTestScanner *scanner, int (*testfn)(int c), const char *file, size_t line)
{
	fprintf(stderr, "consumeCharactersThat(%p, %p) - %s:%ld\n", scanner, testfn, file, line);
	return consumeCharactersThat(scanner);
}

bool ifFollowsConsumeCharacter__wrapped(NojaTestScanner *scanner, char c, const char *file, size_t line)
{
	fprintf(stderr, "ifFollowsConsumeCharacter(%p, '%c') - %s:%ld\n", scanner, c, file, line);
	return ifFollowsConsumeCharacter(scanner);
}

#endif
*/
/*
 * command interpreter
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

char logLevel = 1;

int cmdIndex = 0;
char cmdBuffer[256];
char historyBuffer[512];

char *historyHead = historyBuffer;
char *historyThis = historyBuffer;
char *historyTail = historyBuffer;

/*
 * insert the most recent command into the history buffer
 */
void insertHistory(void)
{
	char *q = historyHead;

	// copy the command into the history buffer
	for (char *p = cmdBuffer; *p; ) {
		*q++ = *p++;
		if (q == historyBuffer + sizeof(historyBuffer))
			q = historyBuffer;
	}

	// terminate it with a null
	*q++ = '\0';
	if (q == historyBuffer + sizeof(historyBuffer))
		q = historyBuffer;

	// advance the head of the history buffer
	historyHead = q;

	// clear the remains of the oldest command
	while (*q) {
		*q++ = '\0';
		if (q == historyBuffer + sizeof(historyBuffer))
			q = historyBuffer;
	}

	// advance to the oldest complete command
	while (!*q) {
		if (++q == historyBuffer + sizeof(historyBuffer))
			q = historyBuffer;
	}

	// update pointers
	historyThis = historyTail = q;
}

/*
 * show the contents of the history buffer
 */
void historyCommand(const char *s)
{
	putchar('\n');

	for (char *p = historyTail; *p; ) {
		while (*p) {
			putchar(*p++);
			if (p == historyBuffer + sizeof(historyBuffer))
				p = historyBuffer;
		}

		if (++p == historyBuffer + sizeof(historyBuffer))
			p = historyBuffer;

		putchar('\n');
	}
}

/*
 * command descriptor
 */
extern struct Command
{
  void (*handler)(const char *);
  const char *name;
  const char *description;
} commands[];

/*
 * show the date
 */
void dateCommand(const char *s)
{
	char buffer[26];
	time_t g_timestamp = time(0);
	struct tm *tmp = localtime((time_t*)&g_timestamp);
	strftime(buffer, sizeof(buffer), " %c", tmp);
	printf("%s\n", buffer);
}

/*
 * If there is an integer argument, parse it and set the logLevel.
 *
 * If there is some other argument, complain.
 *
 * Otherwise, show the logLevel
 */
void logCommand(const char *s)
{
	if (s) {
		if ('0' <= *s && *s <= '9') {
			int level = 0;
			while ('0' <= *s && *s <= '9')
				level = 10 * level + *s++ - '0';
			if (!*s) {
				logLevel = level;
				return;
			}
		}
		printf("syntax: log [integer]\n");
	} else
		printf("%d\n", logLevel);
}

/*
 * exit this environment
 */
void exitCommand(const char *s)
{
	exit(0);
}

extern void helpCommand(const char *s);

/*
 * SORTED array of command descriptors
 */
struct Command commands[] = {
  { dateCommand,    "date",    "show the date and time" },
  { exitCommand,    "exit",    "exit this environment" },
  { helpCommand,    "help",    "show available commands" },
  { historyCommand, "history", "show command history" },
  { logCommand,     "log",     "get or set log level" },
};

const int numCommands = sizeof(commands)/sizeof(*commands);

/*
 * show a list of available commands
 */
void helpCommand(const char *s)
{
	for (int i = 0; i < numCommands; ++i) {
		printf("\n%s\t%s", commands[i].name, commands[i].description);
	}
	putchar('\n');
}

/*
 * retrieve a command from the history
 */
void historic(void)
{
	int i = cmdIndex;
	char *p = historyThis;

	while (i)
	{
		--i;
		printf("\b \b");
	}

	while (*p)
	{
		putchar(cmdBuffer[i++] = *p++);
		if (p == historyBuffer + sizeof(historyBuffer))
			p = historyBuffer;
	}

	cmdIndex = i;
}

/*
 * move to the previous command
 */
void prev(void)
{
	char *p = historyThis;

	char *q = p;	// q is a wraparound marker

	// find end of previous command (if any)
	do {
		if (p == historyBuffer)
			p = historyBuffer + sizeof(historyBuffer);
	} while (!*--p && p != q);

	// find null before previous command
	do {
		if (p == historyBuffer)
			p = historyBuffer + sizeof(historyBuffer);
	} while (*--p);

	// advance to beginning of previous command
	if (++p == historyBuffer + sizeof(historyBuffer))
		p = historyBuffer;

	historyThis = p;

	historic();
}

/*
 * move to the next command
 */
void next(void)
{
	// next history
	char *p = historyThis;

	// find end of this command
	while (*p)
	{
		if (p++ == historyBuffer + sizeof(historyBuffer))
			p = historyBuffer;
	}

	char *q = p;	// q is a wraparound marker

	// find beginning of next command (if any)
	while (!*p)
	{
		if (++p == historyBuffer + sizeof(historyBuffer))
			p = historyBuffer;
		if (p == q)
			break;
	}

	historyThis = p;

	historic();
}

/*
 * try to execute a command
 */
void enter(void)
{
	int cmdArgs;

	cmdBuffer[cmdIndex] = '\0';

	struct Command *pCommand, *qCommand = commands+sizeof(commands)/sizeof(*commands);

	for (cmdArgs = 0; cmdBuffer[cmdArgs] && ' ' != cmdBuffer[cmdArgs]; ++cmdArgs) {}

	for (pCommand = commands; pCommand != qCommand; ++pCommand) {
		if (cmdArgs && !strncmp(pCommand->name, cmdBuffer, strlen(pCommand->name))) {
			while (' ' == cmdBuffer[cmdArgs])
				++cmdArgs;
			if (cmdBuffer[cmdArgs]) {
				insertHistory();
				putchar('\n');
				(*pCommand->handler)(cmdBuffer + cmdArgs);
			} else {
				insertHistory();
				putchar(' ');
				(*pCommand->handler)(0);
			}
			cmdBuffer[cmdIndex = 0] = '\0';
			return;
		}
	}

	putchar('\n');
	if (cmdIndex)
		printf("invalid command\n");
	cmdBuffer[cmdIndex = 0] = '\0';
}

/*
 * try to complete a command
 */
void complete(void)
{
	char c, d;
	struct Command *pCommand = commands;
	struct Command *qCommand = commands+sizeof(commands)/sizeof(*commands)-1;

	for (;;)
	{
		while (pCommand < qCommand && strncmp(cmdBuffer, pCommand->name, cmdIndex) > 0)
			++pCommand;

		while (qCommand > pCommand && strncmp(cmdBuffer, qCommand->name, cmdIndex) < 0)
			--qCommand;

		if (!strncmp(cmdBuffer, pCommand->name, cmdIndex) && cmdIndex < strlen(pCommand->name) && (c = pCommand->name[cmdIndex]) &&
		    !strncmp(cmdBuffer, qCommand->name, cmdIndex) && cmdIndex < strlen(qCommand->name) && (d = qCommand->name[cmdIndex]) &&
		    (c == d))
			putchar(cmdBuffer[cmdIndex++] = c);
		else
			break;
	}
}

/*
 * erase the last character if any
 */
void bs(void)
{
	if (cmdIndex) {
		printf("\b \b");
		cmdBuffer[--cmdIndex] = '\0';
	}
}

/*
 * accept a character from the console
 *
 * Ctrl-H backspace
 * Ctrl-U line-delete
 * Ctrl-P Up-arrow
 * Ctrl-N Down-arrow
 * Tab
 */
void command(char c)
{
	switch (c) {

	case '\020': // ctrl-P
		prev();
		break;

	case '\016': // ctrl-n
		next();
		break;

	case '\025': // ctrl-U
		// line delete
		while (cmdIndex)
			bs();
		break;

	case '\r': // CR
	case '\n': // LF
		enter();
		break;

	case '\0':
		break;

	case '\t': // tab
		complete();
		break;

	case '\b':   // backspace
	case '\177': // del
		bs();
		break;

	case '\033':
		// do not echo ESC
		cmdBuffer[cmdIndex++] = c;
		break;

	case 'A': // up-arrow is "\033\133A"
	case 'B': // down-arrow is "\033\133B"
		if (cmdIndex > 1 &&
		    '\133' == cmdBuffer[cmdIndex-1] &&
		    '\033' == cmdBuffer[cmdIndex-2]) {
			switch (c) {
			case 'A': prev(); return;
			case 'B': next(); return;
			}
		}
	default:
		putchar(cmdBuffer[cmdIndex++] = c);
	}
}

int main(int argc, char **argv, char **envp)
{
	for (;;)
		command(getchar());

	return 0;
}


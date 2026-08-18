/*
 *  crush/src/console.c
 *  `crush console`: an interactive prompt, and a runner for scripted batches of crush commands
 *
 *  WHY THIS EXISTS. Every crush command was its own process, which meant every command paid for
 *  a full framework boot, a context load off disk, and a render engine started and stopped again
 *  -- and then threw all of it away. A build step that adds four fonts and renders six sets did
 *  that eleven times. Here the boot happens once and the commands run against the state already
 *  in memory, which is also what makes an interactive session possible at all.
 *
 *  WHAT IS ACTUALLY REUSED between lines: the loaded context and its object stores, the default
 *  render engine and its worker thread, and the FreeType and jansson allocator wiring. Each
 *  command still commits its own changes to disk as it always did (see crush_font_commit() and
 *  friends), so a session that ends badly has lost nothing a sequence of separate processes
 *  would have kept.
 *
 *  THE INTERESTING CONSTRAINT is that this loop runs inside light_cli's ONE-SHOT task, which the
 *  framework runs before it starts scheduling periodic tasks -- so blocking here blocks the
 *  periodic task list entirely. That is safe on the host and only on the host: log output is
 *  drained by a background thread (LIGHT_PLATFORM_HAS_C11_THREADS), not by a periodic task, so
 *  it keeps flowing while this loop waits on a human. On a single-core target the drain IS a
 *  periodic task, and a console written this way would print its prompt and then never print
 *  anything again. See light_stream.h.
 */
#include <crush.h>
#include <stdio.h>
#include <errno.h>

//   isatty() decides whether to prompt, and it is spelled differently on the two platforms this
// builds for. mingw has the underscored names in <io.h> and only exposes the POSIX spellings
// when it feels like it, so ask for the ones that are always there
#ifdef _WIN32
#include <io.h>
#define _console_isatty(fd)             _isatty(fd)
#define _console_fileno(file)           _fileno(file)
#else
#include <unistd.h>
#define _console_isatty(fd)             isatty(fd)
#define _console_fileno(file)           fileno(file)
#endif

#define COMMAND_CONSOLE_NAME            "console"
#define COMMAND_CONSOLE_DESCRIPTION     "command used to run crush commands interactively, or in batches from a script"

#define OPTION_CONSOLE_COMMAND_NAME     "command"
#define OPTION_CONSOLE_COMMAND_CODE     'c'
#define OPTION_CONSOLE_COMMAND_DESC     "run a single command line and exit, instead of reading any"
#define SWITCH_CONSOLE_KEEP_GOING_NAME  "keep-going"
#define SWITCH_CONSOLE_KEEP_GOING_CODE  'k'
#define SWITCH_CONSOLE_KEEP_GOING_DESC  "in a script, carry on after a failing command instead of stopping at it"
#define SWITCH_CONSOLE_INTERACTIVE_NAME "interactive"
#define SWITCH_CONSOLE_INTERACTIVE_CODE 'i'
#define SWITCH_CONSOLE_INTERACTIVE_DESC "prompt and survive failures even when input is not a terminal"

//   how many script files one invocation accepts. Bounded by LIGHT_CLI_MAX_ARGS (16) rather than
// by anything here; 8 is simply more than any caller has wanted
#define CONSOLE_SCRIPTS_MAX             8

//   the longest command line the console will read. Generous because the arguments are
// filesystem paths and a Windows path is allowed to be 260 characters on its own, and a
// `render new` line carries two of them
#define CONSOLE_LINE_MAX                1024

//   how deeply a script may invoke `console` again. A script that runs itself is an easy
// accident to have, and without a bound it is a stack overflow rather than a message
#define CONSOLE_DEPTH_MAX               8

#define CONSOLE_PROMPT                  "crush> "

#define BUILTIN_EXIT                    "exit"
#define BUILTIN_QUIT                    "quit"
#define BUILTIN_HELP                    "help"
#define BUILTIN_HELP_ALT                "?"

static struct light_cli_invocation_result do_cmd_console(struct light_cli_invocation *invoke);

Light_Command_Define(cmd_crush_console, &cmd_crush, COMMAND_CONSOLE_NAME, COMMAND_CONSOLE_DESCRIPTION,
                                        do_cmd_console, 0, CONSOLE_SCRIPTS_MAX);
Light_Command_Option_Define(opt_crush_console_command, &cmd_crush_console,
                                        OPTION_CONSOLE_COMMAND_NAME, OPTION_CONSOLE_COMMAND_CODE,
                                        OPTION_CONSOLE_COMMAND_DESC);
Light_Command_Switch_Define(sw_crush_console_keep_going, &cmd_crush_console,
                                        SWITCH_CONSOLE_KEEP_GOING_NAME, SWITCH_CONSOLE_KEEP_GOING_CODE,
                                        SWITCH_CONSOLE_KEEP_GOING_DESC);
//   isatty() is the default answer and not always the right one: a session driven through a pipe
// by an editor or a wrapper still wants prompts, and without a way to ask for them the prompting
// path could not be tested at all -- a terminal is the one input a test harness cannot supply
Light_Command_Switch_Define(sw_crush_console_interactive, &cmd_crush_console,
                                        SWITCH_CONSOLE_INTERACTIVE_NAME, SWITCH_CONSOLE_INTERACTIVE_CODE,
                                        SWITCH_CONSOLE_INTERACTIVE_DESC);

// what _read_line() found, which is three outcomes rather than two -- a line that did not fit
// is neither a line nor the end of the input, and treating it as either loses or truncates it
#define LINE_READ                       0
#define LINE_END                        1
#define LINE_OVERLONG                   2

//   nesting depth, so a script that invokes `console` cannot recurse without limit. File-scope
// because the recursion is through the command dispatcher and there is nowhere to thread it
static uint8_t console_depth;

//   writes the prompt and makes sure it is actually on the screen before anything blocks
// waiting for a reply.
//
//   THREE separate steps, none of which is redundant:
//   - light_stream_flush() drains everything the last command queued, so the prompt appears
//     after that command's output rather than in the middle of it. Logging is asynchronous here.
//   - _f_sync() writes through the stream's own lock, which is the same lock the drain worker
//     holds while it prints. Queueing the prompt instead would leave it behind a worker that
//     has no reason to run before we block on input.
//   - fflush(), because a prompt has no newline on the end and neither platform's stdio will
//     push a partial line to a terminal on its own.
static void _prompt(void)
{
        light_stream_flush();
        light_stream_message_f_sync(light_stream_stdout, (const uint8_t *)"%s", CONSOLE_PROMPT);
        fflush(stdout);
}
//   reads one line, strips the terminator, and reports which of the three things happened.
//
//   CRLF as well as LF: a script file written on Windows and read in text mode has already had
// its \r removed, but the same file read through a pipe, or written on Windows and run on a
// Linux CI runner, has not -- and a trailing \r ends up inside the last argument, where it
// becomes part of a filename
static uint8_t _read_line(FILE *input, uint8_t *buffer, size_t size)
{
        if(!fgets((char *)buffer, (int)size, input))
                return LINE_END;

        size_t length = strlen((char *)buffer);
        //   no terminator means the line did not fit. Swallow the rest of it so the next read
        // starts at a line boundary rather than parsing the tail as a command of its own
        if(length && buffer[length - 1] != '\n') {
                int c;
                while((c = fgetc(input)) != EOF && c != '\n')
                        ;
                return LINE_OVERLONG;
        }
        while(length && (buffer[length - 1] == '\n' || buffer[length - 1] == '\r'))
                buffer[--length] = '\0';
        return LINE_READ;
}
//   prints help for a command path typed at the prompt: "help", or "help font", or
// "help render new". Walks the tree by name rather than parsing, since the whole point is to be
// usable when the user does not yet know what the commands are
static void _print_help(int argc, char *argv[])
{
        struct light_command *command = crush_command_root();

        for(int i = 1; i < argc; i++) {
                struct light_command *child = light_cli_find_command(command, (const uint8_t *)argv[i]);
                if(!child) {
                        light_error("no such command '%s' under '%s'", argv[i],
                                        light_cli_command_get_short_name(command));
                        return;
                }
                command = child;
        }
        light_cli_print_command_help(command);
        if(argc < 2)
                light_stream_message_f_faster(light_stream_stdout,
                                "\ncommands are typed without the leading 'crush'. "
                                "'help <command>' describes one; 'exit' ends the session.\n");
}
//   handles the lines that are the console's own rather than the command tree's, and reports
// whether it did. These are deliberately NOT registered commands: `exit` means nothing to a
// crush invoked from a shell, and `help` has to be answerable before the user knows enough to
// ask for anything else
static bool _run_builtin(int argc, char *argv[], bool *stop)
{
        if(!argc)
                return false;
        if(!strcmp(argv[0], BUILTIN_EXIT) || !strcmp(argv[0], BUILTIN_QUIT)) {
                *stop = true;
                return true;
        }
        if(!strcmp(argv[0], BUILTIN_HELP) || !strcmp(argv[0], BUILTIN_HELP_ALT)) {
                _print_help(argc, argv);
                return true;
        }
        return false;
}
//   the read-eval loop itself, over any FILE*: a terminal, a pipe, or a script file. Returns the
// number of commands that failed.
//
//   `interactive` is not the same question as "is this a terminal" and is passed in rather than
// derived here: a script's lines are echoed (so a transcript shows what produced what) and are
// not prompted for, while a terminal's are prompted for and must not be echoed back at the user
// who just typed them.
static uint32_t _run_stream(FILE *input, bool interactive, bool keep_going)
{
        //   ON THE STACK, one set per nesting level, because a script may invoke `console`
        // again. File-scope buffers would have the nested loop overwrite the line its parent's
        // invocation still holds pointers into -- every argument the parser bound points into
        // that buffer, including the paths of the scripts the parent has not opened yet
        uint8_t line[CONSOLE_LINE_MAX];
        uint8_t scratch[CONSOLE_LINE_MAX];
        char *argv[LIGHT_CLI_MAX_TOKENS];
        uint32_t failures = 0;
        bool stop = false;

        while(!stop) {
                if(interactive)
                        _prompt();

                uint8_t status = _read_line(input, line, sizeof(line));
                if(status == LINE_END)
                        break;
                if(status == LINE_OVERLONG) {
                        light_error("command line longer than %d characters, ignored", CONSOLE_LINE_MAX - 1);
                        failures++;
                        if(!keep_going)
                                break;
                        continue;
                }
                //   builtins are recognised from a COPY. light_cli_tokenize_line() splits its
                // input in place, which would leave nothing for light_cli_run_line() to parse --
                // and re-joining tokens to undo that is exactly the kind of guesswork (whose
                // spaces? whose quotes?) this avoids by spending 1KB of stack
                uint8_t argc = 0;
                snprintf((char *)scratch, sizeof(scratch), "%s", (char *)line);
                uint8_t tokenized = light_cli_tokenize_line(scratch, argv, LIGHT_CLI_MAX_TOKENS, &argc);

                //   ECHOED BEFORE ANYTHING IT PRODUCES, and only for a line that produces
                // something: a transcript has to say which command each result came from, but a
                // script's blank lines and comments are not commands and a prompt in front of
                // them is noise. Through the stream layer rather than printf so it stays in
                // order with the command's own logging instead of racing the drain worker
                if(!interactive && (tokenized || argc))
                        light_stream_message_f_faster(light_stream_stdout, "%s%s\n", CONSOLE_PROMPT, line);

                if(tokenized) {
                        // already logged by the tokenizer, which knows what was wrong with it
                        failures++;
                        if(!keep_going)
                                break;
                        continue;
                }
                if(!argc)
                        continue;
                if(_run_builtin(argc, argv, &stop))
                        continue;

                if(light_cli_run_line(crush_command_root(), line) != LIGHT_OK) {
                        failures++;
                        if(!keep_going) {
                                light_error("stopping at the failed command (--keep-going to carry on)");
                                break;
                        }
                }
        }
        return failures;
}
static uint32_t _run_script(const uint8_t *path, bool keep_going)
{
        FILE *script = fopen((const char *)path, "r");
        if(!script) {
                light_error("could not open script '%s': %s", path, strerror(errno));
                return 1;
        }
        light_info("running script '%s'", path);
        uint32_t failures = _run_stream(script, false, keep_going);
        fclose(script);
        return failures;
}
static struct light_cli_invocation_result do_cmd_console(struct light_cli_invocation *invoke)
{
        if(console_depth >= CONSOLE_DEPTH_MAX) {
                light_error("console nested more than %d deep; a script probably runs itself",
                                CONSOLE_DEPTH_MAX);
                return Result_Error;
        }
        const uint8_t *one_line = light_cli_invocation_get_option_value(invoke, OPTION_CONSOLE_COMMAND_NAME);
        bool keep_going = light_cli_invocation_get_switch_value(invoke, SWITCH_CONSOLE_KEEP_GOING_NAME);
        uint32_t failures = 0;

        console_depth++;

        if(one_line) {
                //   COPIED, because the tokenizer writes into what it is given and this points
                // into the process's own argv. Modifying argv is legal but pointlessly rude, and
                // the copy is also where an over-long -c gets caught rather than truncated
                uint8_t line[CONSOLE_LINE_MAX];
                if(snprintf((char *)line, sizeof(line), "%s", (const char *)one_line) >= (int)sizeof(line)) {
                        light_error("--%s is longer than %d characters",
                                        OPTION_CONSOLE_COMMAND_NAME, CONSOLE_LINE_MAX - 1);
                        console_depth--;
                        return Result_Error;
                }
                if(light_cli_run_line(crush_command_root(), line) != LIGHT_OK)
                        failures++;
        } else if(invoke->args_bound) {
                //   every named script, in order. Each is run to completion (or to its first
                // failure) before the next starts, because a script that sets a context up is
                // entitled to assume the one before it finished
                for(uint8_t i = 0; i < invoke->args_bound; i++) {
                        failures += _run_script(light_cli_invocation_get_arg_value(invoke, i), keep_going);
                        if(failures && !keep_going)
                                break;
                }
        } else {
                //   no script named: read from standard input. A TERMINAL gets a prompt and
                // survives a failing command, because that is what a session is for; a PIPE is a
                // script by another name and behaves like one, so `crush console < build.crush`
                // and `crush console build.crush` do the same thing and fail the same way
                bool interactive = light_cli_invocation_get_switch_value(invoke, SWITCH_CONSOLE_INTERACTIVE_NAME)
                                || _console_isatty(_console_fileno(stdin)) != 0;
                if(interactive) {
                        light_stream_message_f_faster(light_stream_stdout,
                                        "%s\ntype 'help' for commands, 'exit' to leave.\n", LF_INFO_STR);
                        //   a failing command must never end an interactive session, whatever
                        // the switch says -- the whole value of a prompt is that a mistake costs
                        // you the line and not the session
                        keep_going = true;
                }
                failures = _run_stream(stdin, interactive, keep_going);
                if(interactive) {
                        //   an interactive session is not a build step and does not fail: the
                        // human saw every error as it happened and decided to carry on anyway
                        light_info("console session ended, %u command(s) failed", failures);
                        failures = 0;
                }
        }
        console_depth--;

        if(failures) {
                light_error("%u command(s) failed", failures);
                return Result_Error;
        }
        return Result_Success;
}

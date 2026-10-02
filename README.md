# cshell-from-scratch

A custom Unix shell written from scratch in C, with pipelines, I/O redirection, full job control, and a set of original builtins, including a `ptrace`-based syscall tracer and a frecency-based `cd`.

No readline, no external libraries. Just libc and POSIX: `fork`, `exec`, `pipe`, `dup2`, signals, and process groups.

## Features

- **Pipelines, redirection, sequencing:** `|`, `<`, `>`, `>>`, `;`, `&`
- **Quote-aware lexer:** state machine handling single and double quotes
- **Syntax validation:** a small parser state machine rejects malformed input before anything runs
- **Job control:** background jobs, process groups, terminal handoff with `tcsetpgrp`, stopped-job tracking
- **Signal handling:** Ctrl+C returns to the prompt, Ctrl+D is handled cleanly (warns about stopped jobs), SIGCHLD reaping with completion notices for background jobs

## Builtins

| Command | What it does |
|---|---|
| `hop [path]` | `cd` with `~`, `-`, `.`, `..` support, plus a **frecency database** (`.cshell_frecency`) that falls back to your most-visited matching directory |
| `reveal [-a] [-t] [path]` | Directory listing (`ls`-style). `-a` shows hidden files, `-t` recurses |
| `peek [-n] [-r] [files...]` | File viewer. `-n` numbers lines continuously across files, `-r` prints in reverse (chunked backward `lseek`, so large files aren't loaded fully). Reads stdin with `-` |
| `locate <name>...` | Finds executables across `$PATH` |
| `spy [pid]` | Inspects a process via `/proc`: cwd, exe, mapped libraries, open file descriptors with types |
| `ping <pid \| %job> <signal>` | Sends a signal to a process, or to a whole job's process group |
| `snoop <cmd>` / `snoop -p <pid>` | Traces syscalls with `ptrace`, then prints a summary sorted by count with timing (a mini `strace -c`) |
| `activities` | Lists tracked jobs and the state of each process |
| `resume %<id> [bg \| fg [--timeout <sec>]]` | Continues a stopped job in the background or foreground, with an optional foreground timeout |

## Build & run

```bash
git clone https://github.com/gavy0037/cshell-from-scratch.git
cd cshell-from-scratch
make
./shell.out
```

Or just `make run`. Clean up with `make clean`.

**Requirements:** Linux, `gcc` with `-std=c23` support (GCC 14+), `make`.

> `snoop` and `spy` use `ptrace` and `/proc`, so they're Linux-only. The syscall name table is x86_64.

## Project structure

```
custom-shell-c/
├── include/          # headers
├── src/
│   ├── main.c        # REPL loop, signal setup
│   ├── lexer.c       # tokenizer (state machine)
│   ├── parser.c      # syntax validation
│   ├── execute.c     # pipelines, forking, job tracking
│   ├── redirect.c    # <, >, >> handling
│   ├── hop.c  reveal.c  peek.c  locate.c
│   ├── spy.c  snoop.c   ping.c  activities.c  resume.c
│   └── prompt.c
└── Makefile
```

## How it works

1. `main.c` reads a line and `lexer.c` turns it into a linked list of tokens.
2. `parser.c` validates the token stream.
3. `execute.c` splits it into pipeline stages, forks each into the same process group, wires up pipes and redirections, then either waits (foreground) or tracks the job (background).
4. Builtins that must affect the shell itself (`hop`, `resume`, `ping`, `exit`) run in the parent. The rest can run in children, so they work inside pipes.

## Example session

```text
<user@host:~> reveal -a
<user@host:~> peek -n notes.txt | grep todo
<user@host:~> sleep 100 &
<user@host:~> activities
<user@host:~> ping %1 19
<user@host:~> resume %1 fg --timeout 5
<user@host:~> snoop ls
```

## Known limitations

- No tab completion, command history, or line editing
- `snoop` is Linux / x86_64 only
- Edit this list so it matches what's actually true for your shell

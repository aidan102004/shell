# BirdShell

# About The Project

BirdShell is a custom command-line shell written to explore how real shells like bash and zsh work under the hood. It handles process creation and execution, path navigation, quoting rules, I/O redirection, and both filename and command completion, with programmable completion for extending behaviour per-command.

I started this project to practice system-level programming; however, it is also a solid base for extending, as I can continue adding more features to replicate a POSIX shell. I plan on implementing pipelines, job control, and history in the near future.

This is very much a work in progress, and features are being added incrementally. 

# Built With
C++
CMake

# Getting Started

Prerequisites: 
You'll need a C compiler and cmake installed.

1. Clone the Repo
2. Move into the project directory
3. Build the project with CMake
4. Run it

# Usage

BirdShell currently supports:

- Command execution — runs external programs via fork/exec, resolving executables against PATH.
- pwd — prints the current working directory.
- cd — changes directory, including handling cd -, cd ~, and relative/absolute paths.
- Quote parsing — correctly tokenises single-quoted, double-quoted, and escaped arguments.
- Redirection — supports >, >>, and < to redirect stdout/stdin to and from files.
- Command completion — tab-completes built-in and PATH-resolved executable names.
- Filename completion — tab-completes file and directory paths as arguments.
- Programmable completion — allows per-command completion rules to be registered (similar in spirit to bash's complete builtin).

# Roadmap
 - Pipelines (cmd1 | cmd2 | cmd3)
 - Background jobs (&, jobs, fg, bg)
 - Command history (readline history + history builtin)

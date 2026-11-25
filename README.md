# bare-bones

A repository of a curious programmer who wants to see how syscalls work by making bare rewritings of common shell utils. Also includes other interesting little C programs from *"what if"* and *"how to"* categories

## List

All programs compile without libraries, just `gcc <program.c>`

| name | description |
|------|-------------|
| ansi.c | tiny program to print a table with ansi colors, pass `--shell` to print escape codes for bash scripts | 
| append.c | sort of inside kitchen of `>>` operator, takes input from stdin and outputs it to given file. e.g: `cat file.txt | append other.txt` |
| cat.c | bare rewriting of `cat` program. |
| chmod.c | bare version of `chmod` |
| cp.c | bare version of `cp` |
| dstat.c | kind of `stat` program, but instead stats content of given directory |
| env.c | bare rewrite of a shell `env` command, optionally returns value of given envitonment variable |
| kill.c | bare rewrite of `kill` program |
| ln.c | bare `ln` proram |
| ls.c | bare version of `ls` |
| paccache.c | rewrite of [paccache](https://wiki.archlinux.org/title/Pacman#Cleaning_the_package_cache) utility in c, does almost same things, includes a little hashmap, pass `-h` argument to see usage |
| pfiles.c | shows what files are opened for given process |
| prlimits.c | same as `prlimit`, shows different resource limits for a program |
| pwd.c | simplest `pwd` |
| rln.c | returns random line from given file |
| rm.c | bare `rm` | 
| sasha.c | a freaking sha256 hash bruteforcer but CPU only version. Also can be used as benchmark, calculates hashes and compares them. [here is an nvidia GPU version](https://github.com/cebem1nt/cusha) |
| shell.c | really simple shell |
| spawn.c | spawns given background process in a new session, redirects stdin/stdout/stderr to /dev/null. e.g: `spawn some process --arg1 --arg2` |
| stat.c | a bare rewrite of stat |
| time.c | bare rewrite of time, measures program execution time |
| to.c | same as append, but rewrite of shell `>` operator |
| toggle_rofi.c | I was bored, so I rewrote my shell script that adds toggle behaviour to rofi from bash to c, now it is freaking fast |
| ulimit.c | kind of `ulimit` but tells you little more things |
| umask.c | shows how `umask` shell command works, but this one won't work properly, it should affect shell environment |
| uname.c | sort of `uname` program recreation, but shows everything at once |
| whoami.c | `whoami`, but also user id and group id |
| xore.c | xor file encrytption |

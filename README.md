# picalc
π Calculus Engine
===========
Some info:
In this implementation, all channels are global. Thus, it is not possible to send a channel through another channel.

Possible π commands:
new(a,b,c...) - creates channel
print(x,y,z,...) or print("...", '...', ...) - prints variable content or string literal
a?x or a?(x,y,z,...) - reads from channel into variable(s)
a!"..." or a!x or a!("...", '...', ...) or a!(x,y,z,...) - writes variable(s) to channel
<command1> . <command2> - runs command1 then command2
<command1> | <command2> - runs command1 and command2 at once
<command1> + <command2> - runs command1 or command2 (random 50/50 chance)

In addition to π commands, the interpreter can accept additional commands, listed below.
clear - reset interpreter to initial state (deletes channels and declared methods)
status - display interpreter state (lists channels, declared methods, and number of halted commands)
exit or quit - closes interpreter

===========
Compilation:
g++ -o picalc picalc.cpp

===========
Running unit tests:
./test.sh
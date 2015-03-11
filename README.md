# picalc
π Calculus Engine
===========
Some info:  <br/>
In this implementation, all channels are global. Thus, it is not possible to send a channel through another channel.

Possible π commands: <br/>
new(a,b,c...) - creates channel <br/>
print(x,y,z,...) or print("...", '...', ...) - prints variable content or string literal <br/>
a?x or a?(x,y,z,...) - reads from channel into variable(s) <br/>
a!"..." or a!x or a!("...", '...', ...) or a!(x,y,z,...) - writes variable(s) to channel <br/>
Command1 . Command2 - runs Command1 then Command2 <br/>
Command1 | Command2 - runs Command1 and Command2 at once <br/>
Command1 + Command2 - runs Command1 or Command2 (random 50/50 chance) <br/>

In addition to π commands, the interpreter can accept additional commands, listed below. <br/>
clear - reset interpreter to initial state (deletes channels and declared methods) <br/>
status - display interpreter state (lists channels, declared methods, and number of halted commands) <br/>
exit or quit - closes interpreter <br/>

===========
Compilation: <br/>
g++ -o picalc picalc.cpp

===========
Running unit tests: <br/>
./test.sh

===========
Code explanation: <br/>
When a string is entered, an expression tree is created and then run according to the rules listed above.
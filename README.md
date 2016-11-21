#SHELL CHAT

Chat application that works purely on the command-line :)

Application is provided as a REPL shell with commands
facilitating chat

Shell is invoked by the command:

 *python shell-chat-client.py configFile\(OPTIONAL\)*

Chatroom session parameters can be passed in via **config
files which are in JSON format**. This allows you to switch
between multiple personas for different chatrooms easily.
This parameter is optional - **default.json** is used as
the config file by default.

The source code includes another config file named
**config.json** in the same chatroom as described by
**default.json**. Conversations can be simulated in
localhost by spawning 2 chat clients as:

 *python shell-chat-client.py*

 *python shell-chat-client.py config.json*

**Third party dependencies:**
* cJSON - https://github.com/DaveGamble/cJSON

  Place cJSON folder on the project root for Makefile to work

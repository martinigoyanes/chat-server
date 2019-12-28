# chat-server
Implementation of a chat server in C, client was given.

**Protocol:**

Clients allow users to communicate with one another by connecting to the server and interacting with it in accordance to
an application protocol. Through this protocol, the server allows clients to engage in group chats
in chat rooms and send private messages to one another


To run **client**:

``client -u``

1. -u = Run with limited ncurses support for a slightly better chat experience.
While running, the client takes commands directly from the user. 

All commands are preceded by a backslash. Not every command is available in every context. The client supports the following
commands:

1. \connect <IP Address>:<Port> = Instruct the client to connect to a new chat server,
specified by the IP address and port.
  
2. \disconnect = If connected to a server, disconnect from that server.

3. \join <Room> <Password> = Join the specified chatroom, creating it if it does not
already exist. The Password is optional, with the default being the empty string. Users may
only join rooms for which they know the password. Both Room and Password must be less
than 256 characters in length.
  
4. \leave = If in a room, this exits the room. Otherwise, it leaves the server.

5. \list users = List all users. If in a room, it lists all users in that room. Otherwise, it lists
all users connected to the server.

6. \list rooms = List all rooms that currently exist on the server.

7. \msg <User> <Message> = Send a private message to the specified user. User must be
less than 256 characters in length, and the Message must be less than 65536 characters in
length.

8. \nick <Name> = Set your nickname to the specified name. Name must be less than 256
characters in length.
  
9. \quit = Exits out of the client.

All other input is interpreted as a message being sent to the room the user is currently in.

To run **server**:

``server -p <number>``

-p <Number> = The port that the server will listen on. Represented as a base-10 integer.
Must be specified.

Name:   Swathi Janapala
Roll Num: 23CS60R80

->server.c contains server part code, and i have implemented all 4-tasks in one server code file.
->client.c contains clinet code.
->chat_history.txt stores the chat between clients in the format of:
<sender_id>,<recipient_id>,<message>

Compilation and Running:

->for compile the server.c use the command : gcc server.c -luuid -o server
->for client.c : gcc client.c
# Peer-to-Peer Chat Application with Chat History

This project involves implementing a basic peer-to-peer chat functionality between clients, with the ability to maintain chat history for each client on the server.

## Task 1: Connection Establishment and Chatting

### Connection Establishment:
- Clients should send a connection request to the server.
- The server assigns unique identifiers (IDs) to clients and sends a welcome message along with their ID upon successful connection.
- The server maintains a table of connected clients' details, including their socket ID and unique ID.

### Chatting:
- Clients can use the following commands:
  - `/active`: Retrieve a list of active clients.
  - `/send <dest_id> <message>`: Send messages to other clients using their unique IDs.
  - `/logout`: Request to exit the application. The server sends a "Bye!! Have a nice day" message as an acknowledgment.

### Error Handling:
- The server should handle message sharing between clients and notify the sender if the recipient goes offline.
- Appropriate error handling should be implemented.

`Compile using "gcc -o server server.c -luuid -pthread" the server.c ant then "./server"
for client .c -> "gcc -o client client.c" and then "./client"`

## Task 2: Chat History Management (Improved)

### Features for the Client:
- `/history <recipient_id>`: Retrieve the conversation history between the requesting client and the specified recipient.
- `/history_delete <recipient_id>`: Delete chats of the specified recipient from the requesting client's chat history.
- `/delete_all`: Delete the complete chat history of the requesting client.

### Implementation Details:
- Maintain a chat history for each client, storing their previous messages and conversations in a log file on the server.
- Whenever a client sends a message to another client, the server stores the message in the chat history for both clients.

`run using "gcc -o server server.c -luuid -pthread" the server.c ant then "./server"
for client .c -> "gcc -o client client.c" and then "./client"`

Feel free to reach out if you have any questions or need further assistance!

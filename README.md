# UDP-video-transfer

Open Terminal on your Linux system
Write the following line to start the server:

g++ server.c -o serv

This will create a file named "serv" in the same directory, then write

./serv 9898

Now the server will start listening on the port 9898
Next, we complie the client by using:

g++ client.c -o cli

This will create a file named "cli" in the same directory, then write

./cli 127.0.0.1 9898

127.0.0.1 is for localhost.

Sender:
The sender works as follows: "Sender listenport" (Command line Arguments).

Receiver:
The receiver should run as follows: "Receiver serevr-addr listenport" (Command line Arguments).

# GoBackNProtocol
An implementation of the go-back-N protocol for CS 485 Networking at the University of Kentucky
Sean Karlage

## Instructions for execution
1. unzip /path/to/zipfile.zip
2. cd /path/to/unzipped/file
3. make
4. (in terminal 1): ./receiver <port> [<loss_rate>]
5. (in terminal 2): ./sender <ip> <port> <chunk_size> <window_size>

## General Architecture
There are multiple files that comprise this project:
* sender.c: the main procedure for the sender process
* receiver.c: main procedure for the receiver process
* packet.c: contains helpful functions for constructing, receiving, sending, and serializing packets
* net.c: contains helpful networking code to set up any network connections
* data.h: contains the data buffer to send over the network
* timer.c: contains function for setting timer on the socket

The sender implements the go-back-N protocol, and the receiver will respond to any packet with the ACK number that it expects to receive next. Once the process of transferring the entire data buffer is complete, the sender will send a "tear-down" message, which the receiver will respond with an appropriate "tear-down" ACK.

## Notes
* To the best of my knowledge, this is a complete solution to the problem.

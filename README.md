# Tiny Social Network Service (SNS) using gRPC

Tiny Social Network Service (SNS) is a simple social networking application implemented using gRPC, a high-performance, language-agnostic remote procedure call (RPC) framework. This project demonstrates how to create a basic social networking service where users can log in, follow/unfollow others, post/display messages, and retrieve user lists.

## Features

- User Authentication: Users can log in using a unique username.
- Follow/Unfollow: Users can follow or unfollow other users.
- Posting Messages: Users can post messages on their timeline.
- Real-time Updates: Messages are displayed in real-time for followers.
- User Lists: Users can retrieve lists of all users and followers.

## Prerequisites

Before running the SNS application, ensure you have the following dependencies installed:

- Protocol Buffers (protoc) version 3.0.0 or newer
- gRPC C++ protobuf plugin
- C++ compiler (g++)
- pkg-config

## Getting Started

Follow these steps to set up and run the SNS application:

### Step 1: Compile Protobuf Files

Compile the `.proto` files to generate the necessary C++ code for the client and server.

```shell
make
```

### Step 2: Build Client and Server

Build the client and server executables using the Makefile:

```shell
make tsc
make tsd
```

### Step 3: Run Client and Server

Start the server before the client. Run the server using:

```shell
./tsd
```

Then, run the client using:

```shell
./tsc
```

### Step 4: Interact with the SNS

The client and server will interact based on RPC methods. Use the client to log in, follow/unfollow users, post messages, and retrieve user lists.

## Code Structure

- `sns.proto`: Protobuf definition file containing service definitions and message structures.
- `sns_client.cc`: Client code for the SNS application.
- `sns_server.cc`: Server code for the SNS application.
- `Makefile`: Makefile for compiling and building the project.

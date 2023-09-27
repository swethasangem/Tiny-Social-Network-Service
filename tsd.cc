/*
 *
 * Copyright 2015, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <ctime>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>
#include<glog/logging.h>
#define log(severity, msg) LOG(severity) << msg; google::FlushLogFiles(google::severity); 

#include "sns.grpc.pb.h"


using google::protobuf::Timestamp;
using google::protobuf::Duration;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using csce662::Message;
using csce662::ListReply;
using csce662::Request;
using csce662::Reply;
using csce662::SNSService;


struct Client {
  std::string username;
  bool connected = true;
  int following_file_size = 0;
  std::vector<Client*> client_followers;
  std::vector<Client*> client_following;
  ServerReaderWriter<Message, Message>* stream = 0;
  bool operator==(const Client& c1) const{
    return (username == c1.username);
  }
};

//Vector that stores every client that has been created
std::vector<Client*> client_db;


//utility method to find index of user with given username
int findUserIndex(std::string targetUsername) {
    for (int index = 0; index < client_db.size(); ++index) {
        if (client_db[index]->username == targetUsername) {
        //returns the index of the found user
            return index; 
        }
    }
    //returns -1 if the user is not found in the database
    return -1; 
}


class SNSServiceImpl final : public SNSService::Service {
  
  Status List(ServerContext* context, const Request* request, ListReply* list_reply) override {
    /*********
    YOUR CODE HERE
    **********/
    Client* user = client_db[findUserIndex(request->username())];
    
    //adding users in client_db to list_reply;
    for(Client* c: client_db){
    	list_reply->add_all_users(c->username);
    }
    
    //adding follow of c to list_reply
    std::vector<Client*>::const_iterator it;
    for (it = user->client_followers.begin(); it != user->client_followers.end(); it++) {
	    int followerIndex = findUserIndex((*it)->username);
	    if (followerIndex != -1) {
		list_reply->add_followers(client_db[followerIndex]->username);
	    }
    }

    
    return Status::OK;
  }

  Status Follow(ServerContext* context, const Request* request, Reply* reply) override {

    /*********
    YOUR CODE HERE
    **********/
    //extracting the usernames from request
    std::string u1 = request->username();
    std::string u2 = request->arguments(0);
    
    int index1 = findUserIndex(u1);
    int index2 = findUserIndex(u2);
    
    //If the user is not found
    if(index1 < 0 || index2 < 0  )
    {
    	reply->set_msg("Join Failed -- Invalid Username");
    	
    }
    //If the username is same as current user
    else if(u1 == u2)
    {
    	reply->set_msg("Join Failed -- Already Following User");
    	
    }
    else
    {
     // Checking if already following the user
    	if (std::find(client_db[index1]->client_following.begin(), client_db[index1]->client_following.end(), client_db[index2]) != client_db[index1]->client_following.end()) {
            reply->set_msg("Join Failed -- Already Following User");
            return Status::OK;
        }

        // updating the following and followers lists for both users
        client_db[index1]->client_following.push_back(client_db[index2]);
        client_db[index2]->client_followers.push_back(client_db[index1]);

        // Set a success message in the reply
        reply->set_msg("Join Successful");
    }
    
    return Status::OK; 
  }

  Status UnFollow(ServerContext* context, const Request* request, Reply* reply) override {

    /*********
    YOUR CODE HERE
    **********/
    std::string u1 = request->username();
    std::string u2 = request->arguments(0);
    
    int index1 = findUserIndex(u1);
    int index2 = findUserIndex(u2);
    //If the user is not found or same as current
    if(index1 < 0 || index2 < 0 || u1 == u2 )
    {
    	reply->set_msg("Leave Failed -- Invalid Username");
    	
    }
    else{
    //Checking if the user is not in the follower's list
    if(std::find(client_db[index1]->client_following.begin(), client_db[index1]->client_following.end(), client_db[index2]) == client_db[index1]->client_following.end()){
	      reply->set_msg("Leave Failed -- Not Following User");
        return Status::OK;
      }
      client_db[index1]->client_following.erase(find(client_db[index1]->client_following.begin(), client_db[index1]->client_following.end(), client_db[index2])); 
      client_db[index2]->client_followers.erase(find(client_db[index2]->client_followers.begin(), client_db[index2]->client_followers.end(), client_db[index1]));
      reply->set_msg("Leave Successful");
    }
    
    
    return Status::OK;
  }

  // RPC Login
  Status Login(ServerContext* context, const Request* request, Reply* reply) override {

    /*********
    YOUR CODE HERE
    **********/
    
      // Extract the username from the incoming request
    std::string username = request->username();
    
    // Log that a client is attempting to connect with the server
    std::cout << username + " is connecting to the server" << std::endl;
    
    // Check if the user already exists by searching for their index
    int user_index = findUserIndex(username);
    
    // If the user is not found, create a new Client object and add it to the client database
    if(user_index < 0){
        Client* c = new Client();
        c->username = username;
        client_db.push_back(c);
        // Set the reply message to indicate a successful login
        reply->set_msg("Login Successful!");
        
        // Print the usernames of all connected clients
        // for (auto user : client_db) {
        //     std::cout << user->username << std::endl;
        // }
    }
    else{ 
        // If the user is already connected, set a message indicating that
        if(client_db[user_index]->connected){
            reply->set_msg("you have already joined");//already existing
        }
        else{
            // If the user is not connected, set a message indicating their successful login
            std::string msg = client_db[user_index]->username;
            reply->set_msg(msg);
            client_db[user_index]->connected = true;
        }
    }
    
    // Return a status of OK to indicate successful handling of the login request
    return Status::OK;
  }

  Status Timeline(ServerContext* context, 
		ServerReaderWriter<Message, Message>* stream) override {

    /*********
    YOUR CODE HERE
    **********/
    // Define variables for message processing
    Message message;
    int userIndex;

    // Loop to continuously read messages from the client
    while (stream->Read(&message)) {
        std::string username = message.username();
        userIndex = findUserIndex(username);
        
        // Prepare the formatted message to store in the user-specific file
        std::string filename = username + ".txt";
        std::ofstream user_file(filename, std::ios::app | std::ios::out | std::ios::in);
        google::protobuf::Timestamp temptime = message.timestamp();
        std::string time = google::protobuf::util::TimeUtil::ToString(temptime);
        std::string fileinput = "T " + time + "\n" + "U " + message.username() + "\n" +  "W " + message.msg() + "\n";

        // Check if the received message is not the "FirstStream" initialization message
        if (message.msg() != "FirstStream") {
            user_file << fileinput;
        } 
        else 
        {
            // Handle the "FirstStream" message

            // Assign the client's stream to the server for future communication
            client_db[userIndex]->stream = stream;
            Message newMsg;   
            int count = 0;
            // Read the latest 20 messages from the user's "following.txt" file
            std::string line;
            std::vector<std::string> latest_twenty;
            std::ifstream in(username + "following.txt");
            
            
            // Read and collect the last 20 messages
            while (getline(in, line)) {
                if (client_db[userIndex]->following_file_size > 20) {
                    
                    if (count < client_db[userIndex]->following_file_size - 20) {
                        count++;
                        continue; // Skipping older messages
                    }
                }
                latest_twenty.push_back(line);
            }
            
            // Send the latest messages to the client to be displayed
            for (int i = 0; i < latest_twenty.size(); i++) {
                newMsg.set_msg(latest_twenty[i]);
                stream->Write(newMsg);
            }
            
            // Continue listening for more messages from the client
            continue;
        }
        
        // Send the received message to each follower's stream
        std::vector<Client*>::const_iterator it;
        for (it = client_db[userIndex]->client_followers.begin(); it != client_db[userIndex]->client_followers.end(); ++it) {
            Client* follower = *it;
            
            if (follower->stream != nullptr && follower->connected) {
                follower->stream->Write(message);
            }
            
            // Put the message of the current user's followers in their "following.txt" file
            std::string temp_username = follower->username;
            std::string temp_file = temp_username + "following.txt";
            std::ofstream following_file(temp_file, std::ios::app | std::ios::out | std::ios::in);
            following_file << fileinput;
            
            client_db[userIndex]->following_file_size++;
        }
    }
    
    // If the client disconnected from Chat Mode, set connected to false
    client_db[userIndex]->connected = false;
    return Status::OK;
    }
};



void RunServer(std::string port_no) {
  std::string server_address = "0.0.0.0:"+port_no;
  SNSServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  log(INFO, "Server listening on "+server_address);

  server->Wait();
}

int main(int argc, char** argv) {

  std::string port = "3010";
  
  int opt = 0;
  while ((opt = getopt(argc, argv, "p:")) != -1){
    switch(opt) {
      case 'p':
          port = optarg;break;
      default:
	  std::cerr << "Invalid Command Line Argument\n";
    }
  }
  
  std::string log_file_name = std::string("server-") + port;
  google::InitGoogleLogging(log_file_name.c_str());
  log(INFO, "Logging Initialized. Server starting...");
  RunServer(port);

  return 0;
}

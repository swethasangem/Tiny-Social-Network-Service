#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <string>
#include <unistd.h>
#include <csignal>
#include <grpc++/grpc++.h>
#include "client.h"
#include <google/protobuf/util/time_util.h>
#include <ctime>
#include "sns.grpc.pb.h"
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using csce662::Message;
using csce662::ListReply;
using csce662::Request;
using csce662::Reply;
using csce662::SNSService;


void sig_ignore(int sig) {
  std::cout << "Signal caught " + sig;
}

Message MakeMessage(const std::string& username, const std::string& msg) {
    Message m;
    m.set_username(username);
    m.set_msg(msg);
    google::protobuf::Timestamp* timestamp = new google::protobuf::Timestamp();
    timestamp->set_seconds(time(NULL));
    timestamp->set_nanos(0);
    m.set_allocated_timestamp(timestamp);
    return m;
}


class Client : public IClient
{
public:
  Client(const std::string& hname,
	 const std::string& uname,
	 const std::string& p)
    :hostname(hname), username(uname), port(p) {}

  
protected:
  virtual int connectTo();
  virtual IReply processCommand(std::string& input);
  virtual void processTimeline();

private:
  std::string hostname;
  std::string username;
  std::string port;
  
  // You can have an instance of the client stub
  // as a member variable.
  std::unique_ptr<SNSService::Stub> stub_;
  
  IReply Login();
  IReply List();
  IReply Follow(const std::string &username);
  IReply UnFollow(const std::string &username);
  void   Timeline(const std::string &username);
};


///////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////
int Client::connectTo()
{
  // ------------------------------------------------------------
  // In this function, you are supposed to create a stub so that
  // you call service methods in the processCommand/porcessTimeline
  // functions. That is, the stub should be accessible when you want
  // to call any service methods in those functions.
  // Please refer to gRpc tutorial how to create a stub.
  // ------------------------------------------------------------
    
///////////////////////////////////////////////////////////
// YOUR CODE HERE
//////////////////////////////////////////////////////////
   std::string server_address = hostname + ":" + port;
    stub_ = std::unique_ptr<SNSService::Stub>(SNSService::NewStub(
        grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials())));

    // Prepare a login request with the client's username
    Request request;
    request.set_username(username);

    // Sendinf the login request to the server
    Reply reply;
    ClientContext context;
    Status status = stub_->Login(&context, request, &reply);

    // Processing the server's response
    IReply ire;
    ire.grpc_status = status;

    if (reply.msg() == "you have already joined") {
        ire.comm_status = FAILURE_ALREADY_EXISTS;
        return -1;
    } else {
        ire.comm_status = SUCCESS;
    }

    // Checking if the connection was successful
    if (status.ok()) {
        std::cout << "Connected to the server " + server_address << std::endl;
        return 1; 
    } else {
        std::cerr << "Failed to connect to the server" << std::endl;
        return -1;
    }
}

IReply Client::processCommand(std::string& input)
{
  // ------------------------------------------------------------
  // GUIDE 1:
  // In this function, you are supposed to parse the given input
  // command and create your own message so that you call an 
  // appropriate service method. The input command will be one
  // of the followings:
  //
  // FOLLOW <username>
  // UNFOLLOW <username>
  // LIST
  // TIMELINE
  // ------------------------------------------------------------
  
  // ------------------------------------------------------------
  // GUIDE 2:
  // Then, you should create a variable of IReply structure
  // provided by the client.h and initialize it according to
  // the result. Finally you can finish this function by returning
  // the IReply.
  // ------------------------------------------------------------
  
  
  // ------------------------------------------------------------
  // HINT: How to set the IReply?
  // Suppose you have "FOLLOW" service method for FOLLOW command,
  // IReply can be set as follow:
  // 
  //     // some codes for creating/initializing parameters for
  //     // service method
  //     IReply ire;
  //     grpc::Status status = stub_->FOLLOW(&context, /* some parameters */);
  //     ire.grpc_status = status;
  //     if (status.ok()) {
  //         ire.comm_status = SUCCESS;
  //     } else {
  //         ire.comm_status = FAILURE_NOT_EXISTS;
  //     }
  //      
  //      return ire;
  // 
  // IMPORTANT: 
  // For the command "LIST", you should set both "all_users" and 
  // "following_users" member variable of IReply.
  // ------------------------------------------------------------

    IReply ire;
    
    /*********
    YOUR CODE HERE
    **********/
    
    	std::size_t index = input.find_first_of(" "); // Find the first space character in the input string

	// Check if a space character was found
	if (index != std::string::npos) {
	    std::string cmd = input.substr(0, index); // Extract the command part before the space
	    std::string arg = input.substr(index + 1, (input.length() - index)); // Extract the argument part after the space

	    // Check the command type and call the corresponding function
	    if (cmd == "FOLLOW") {
		return Follow(arg); // Call the Follow function with the argument
	    } else if (cmd == "UNFOLLOW") {
		return UnFollow(arg); // Call the UnFollow function with the argument
	    }
	} else {
	    // If no space character was found, check for specific input commands
	    if (input == "LIST") {
		return List(); // Call the List function
	    } else if (input == "TIMELINE") {
		ire.comm_status = SUCCESS; // Set communication status to success for TIMELINE command
		return ire; // Return the IReply with SUCCESS status
	    }
	}

	// If none of the above conditions match, set communication status to FAILURE_INVALID
	ire.comm_status = FAILURE_INVALID;

	return ire; // Return the IReply with FAILURE_INVALID status

}


void Client::processTimeline()
{
    Timeline(username);
}

// List Command
IReply Client::List() {

    IReply ire; // Create an instance of IReply to store the response

    // Create a Request to send to the server with the user's username
    Request request;
    request.set_username(username);

    // Create a container for the data from the server
    ListReply list_reply;

    // Create a ClientContext for the client
    ClientContext context;

    // Call the server's List method and store the status and response
    Status status = stub_->List(&context, request, &list_reply);

    // Store the gRPC status in the IReply
    ire.grpc_status = status;

    // Check if the gRPC call was successful
    if (status.ok()) {
        ire.comm_status = SUCCESS; // Set the communication status to SUCCESS

        // Loop through the list of all users and followers received from the server
        for (std::string s : list_reply.all_users()) {
            ire.all_users.push_back(s); // Add each user to the all_users vector in IReply
        }

        for (std::string s : list_reply.followers()) {
            ire.followers.push_back(s); // Add each follower to the followers vector in IReply
        }
    }

    return ire; // Return the IReply with the retrieved data and status
}

// Follow Command        
IReply Client::Follow(const std::string& username2) {

    IReply ire; 

    // Create a request to follow another user
    Request request;
    request.set_username(username);
    request.add_arguments(username2);

    // Initialize a reply object to store the server's response
    Reply reply;
    ClientContext context;

    // Make an RPC call to the server's Follow method
    Status status = stub_->Follow(&context, request, &reply);
    
    // Store the gRPC status in the IReply object
    ire.grpc_status = status;

    // Determine the communication status based on the server's reply message
    if (reply.msg() == "Join Failed -- Invalid Username" || reply.msg() == "unknown follower username") {
        ire.comm_status = FAILURE_INVALID_USERNAME;
    } 
    else if (reply.msg() == "Join Failed -- Already Following User" || reply.msg() == "Join Failed -- Username already exists") {
        ire.comm_status = FAILURE_ALREADY_EXISTS;
    }
    else if (reply.msg() == "Join Successful") {
        ire.comm_status = SUCCESS;
    } 
    else {
        ire.comm_status = FAILURE_UNKNOWN;
    }
    
    // Return the IReply object with the communication status
    return ire;
}

// UNFollow Command  
IReply Client::UnFollow(const std::string& username2) {

    IReply ire;

    // Create a request to unfollow another user
    Request request;
    request.set_username(username);
    request.add_arguments(username2);

    // Initialize a reply object to store the server's response
    Reply reply;

    // Create a context for the client
    ClientContext context;

    // Make an RPC call to the server's UnFollow method
    Status status = stub_->UnFollow(&context, request, &reply);
 
    // Store the gRPC status in the IReply object
    ire.grpc_status = status;

    // Determine the communication status based on the server's reply message
    if (reply.msg() == "Leave Failed -- Invalid Username") {
        ire.comm_status = FAILURE_INVALID_USERNAME;
    } 
    else if (reply.msg() == "Leave Successful") {
        ire.comm_status = SUCCESS;
    } 
    else {
        ire.comm_status = FAILURE_UNKNOWN;
    }

    // Return the IReply object with the communication status
    return ire;
}

// Login Command  
IReply Client::Login() {

    IReply ire;
  
    /***
     YOUR CODE HERE
    ***/
    Request request;
    request.set_username(username);
    ClientContext context;
    Reply reply;
    
    //making an RPC call to the servers Login method
    Status status = stub_->Login(&context, request, &reply);
    
    //recording the gRPC status
    ire.grpc_status = status;
    
    //determining communication status based on reply message
    if (reply.msg() == "you have already joined") {
        ire.comm_status = FAILURE_ALREADY_EXISTS;
    } 
    else 
    {
        ire.comm_status = SUCCESS;
    }
    return ire;
}

// Timeline Command
void Client::Timeline(const std::string& username) {

    // ------------------------------------------------------------
    // In this function, you are supposed to get into timeline mode.
    // You may need to call a service method to communicate with
    // the server. Use getPostMessage/displayPostMessage functions 
    // in client.cc file for both getting and displaying messages 
    // in timeline mode.
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // IMPORTANT NOTICE:
    //
    // Once a user enter to timeline mode , there is no way
    // to command mode. You don't have to worry about this situation,
    // and you can terminate the client program by pressing
    // CTRL-C (SIGINT)
    // ------------------------------------------------------------
  
    /***
    YOUR CODE HERE
    ***/
    ClientContext context;

    std::shared_ptr<ClientReaderWriter<Message, Message>> stream(stub_->Timeline(&context));

    //Thread to read messages and send to the server
    std::thread writer([username, stream]() {
        std::string input = "FirstStream";
        Message m = MakeMessage(username, input);
        stream->Write(m);
        while (1) {
            input = getPostMessage();
            m = MakeMessage(username, input);
            stream->Write(m);
        }
        //stream->WritesDone();
    });
    //Thread for reading the posts
    std::thread reader([username, stream]() {
    std::string line;
         std::string time;
    	 std::string username;
    	 std::string msg;
    int count = 0;
    while (true) {
    	//terminating if count>=20
    	if(count>=20)
    	break;
        Message m;
        stream->Read(&m);
            
        // Extract the message components
        std::string line = m.msg();
        char type = line[0]; // The first character indicates the type (T, U, W)
	google::protobuf::Timestamp timestamp;
	std::time_t t_time;
        switch (type) {
            case 'T':
                // Extract and display time
                time = line.substr(2); // Remove the type character
                
		google::protobuf::util::TimeUtil::FromString(time, &timestamp);
		t_time = google::protobuf::util::TimeUtil::TimestampToTimeT(timestamp);
                // Display the time as needed
                break;
            case 'U':
                // Extract and display username
                username = line.substr(2);
                // Display the username as needed
                break;
            case 'W':
                // Extract and display message
                msg = line.substr(2);
                // Display the message as needed
                count++;
                displayPostMessage(username, msg, t_time);
                break;
            default:
                //std::cerr << "Invalid line format: " << line << std::endl;
                break;
        }
    }
    });

    //Wait for the threads to finish
    writer.join();
    reader.join();

}



//////////////////////////////////////////////
// Main Function
/////////////////////////////////////////////
int main(int argc, char** argv) {

  std::string hostname = "localhost";
  std::string username = "default";
  std::string port = "3010";
    
  int opt = 0;
  while ((opt = getopt(argc, argv, "h:u:p:")) != -1){
    switch(opt) {
    case 'h':
      hostname = optarg;break;
    case 'u':
      username = optarg;break;
    case 'p':
      port = optarg;break;
    default:
      std::cout << "Invalid Command Line Argument\n";
    }
  }
      
  std::cout << "Logging Initialized. Client starting...";
  
  Client myc(hostname, username, port);
  
  myc.run();
  
  return 0;
}

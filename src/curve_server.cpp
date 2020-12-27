
#include <iostream>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <thread>
#include <boost/format.hpp>

#define public_key_req "\001"

static constexpr uint32_t public_key_server_port = 50000;
static constexpr uint32_t server_port = public_key_server_port + 1;

//Server thread to send the server public key to client
void start_public_key_server(const std::string &public_key)
{
    zmq::context_t ctx(1);
    zmq::socket_t public_key_sendback_socket(ctx, zmq::socket_type::dealer);

    boost::format fmt("tcp://%1%:%2%");
    std::string bind_str = ((fmt) % ("*") % (public_key_server_port)).str();

    public_key_sendback_socket.bind(bind_str);

    while (1)
    {
        zmq::multipart_t msgs;
        msgs.recv(public_key_sendback_socket);

        //Recve message and check the request is public_key_req
        if (msgs.size() == 1 and msgs.back().to_string_view().compare(public_key_req) == 0)
        {
            msgs.clear();
            msgs.pushstr(public_key);
            msgs.send(public_key_sendback_socket);
        }
    }
}

void start_with_curve()
{
    char secretkey[128] = {0};
    char publickey[128] = {0};

    //Generate the curve key pair
    auto rc = zmq_curve_keypair(&publickey[0], &secretkey[0]);
    assert(rc == 0);

    //Print out the public key and private key
    std::cout << "secret key: " << secretkey << " -  " << strlen(secretkey) << std::endl;
    std::cout << "public key: " << publickey << " -  " << strlen(secretkey) << std::endl;

    //Start the public key server
    std::thread public_key_server_thread(start_public_key_server, std::move(std::string(publickey)));
    public_key_server_thread.detach();

    zmq::context_t ctx(1);
    zmq::socket_t server_socket(ctx, zmq::socket_type::dealer);

    //Set sockopt to server
    server_socket.set(zmq::sockopt::curve_server, 1);
    //Set sockopt secretkey
    server_socket.set(zmq::sockopt::curve_secretkey, secretkey);

    boost::format fmt("tcp://%1%:%2%");
    std::string bind_str = ((fmt) % ("*") % (server_port)).str();

    std::cout << "start secret comunication\n";

    server_socket.bind(bind_str);

    while (1)
    {
        zmq::multipart_t msgs;
        msgs.recv(server_socket); //Recv the message
        std::cout << "Recv msgs: " << msgs.back().to_string_view() << std::endl;
        std::string send_back_msg{msgs.back().to_string_view()};
        send_back_msg.append(", too"); //Create the new msg, base on the recived message.
        msgs.clear();
        msgs.pushstr(send_back_msg);
        msgs.send(server_socket); //Send the message back
    }
}

int main(int argc, char **argv)
{
    start_with_curve();
}
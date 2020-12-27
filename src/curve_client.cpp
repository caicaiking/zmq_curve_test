#include <iostream>
#include <boost/format.hpp>
#include <zmq.hpp>
#include <zmq_addon.hpp>

#define public_key_req "\001"

static constexpr uint32_t public_key_server_port = 50000;
static constexpr uint32_t server_port = public_key_server_port + 1;

std::string get_public_key()
{
    zmq::context_t ctx(1);
    zmq::socket_t public_socket(ctx, zmq::socket_type::dealer);

    boost::format fmt("tcp://%1%:%2%");
    std::string connect_string = (fmt % ("localhost") % (public_key_server_port)).str();

    public_socket.connect(connect_string);

    std::string pub_key{};

    while (true)
    {
        zmq::multipart_t msgs;
        msgs.pushstr(public_key_req);
        msgs.send(public_socket);
        msgs.recv(public_socket);

        if (msgs.size() == 1 and msgs.back().size() >= 40)
        {
            pub_key = msgs.back().to_string();
            break;
        }

        zmq_sleep(1);
    }

    public_socket.close();
    ctx.close();

    return pub_key;
}

int main(int argc, char **argv)
{
    auto public_key_str = get_public_key();
    zmq::context_t ctx(1);
    zmq::socket_t client_socket(ctx, zmq::socket_type::dealer);
    client_socket.set(zmq::sockopt::curve_serverkey, public_key_str);

    char client_pub_key[64] = {0};
    char client_pri_key[64] = {0};
    zmq_curve_keypair(client_pub_key, client_pri_key);
    client_socket.set(zmq::sockopt::curve_publickey, client_pub_key);
    client_socket.set(zmq::sockopt::curve_secretkey, client_pri_key);

    boost::format fmt("tcp://%1%:%2%");
    std::string connect_string = (fmt % ("localhost") % (server_port)).str();
    client_socket.connect(connect_string);

    int count{};
    while (1)
    {
        zmq::multipart_t msgs;
        boost::format fmt("I say %1%");
        msgs.pushstr((fmt % (++count)).str());
        std::cout << "send msg: " << msgs.back().to_string_view() << std::endl;
        msgs.send(client_socket);

        msgs.recv(client_socket);
        std::cout << "recv msg: " << msgs.back().to_string_view() << std::endl;
        getchar();
    }
}
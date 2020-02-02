#include <iostream>
#include <csignal>

#include "TcpIp.h"

bool keepRunning = true;

using byte = uint8_t;
void serverReadCallBack( ::std::string clientAddr, ::std::vector< byte > data )
{
    ::std::string strData( reinterpret_cast< char* >( data.data() ), data.size() ); //bad but tolerable
    ::std::cout << __func__ << " : "
                << "Server received data : \n\t" 
                << strData << '\n'
                << "From client : " << clientAddr << ::std::endl;
}

void serverSendCallBack( ::std::size_t sent_bytes )
{
    ::std::cout << __func__ << " : "
                << "Server sent " 
                << sent_bytes << "bytes." 
                << ::std::endl;
}

/*                       Unification : */
void clientReadCallBack( ::std::string , ::std::vector< byte > data )
{
    ::std::string strData( reinterpret_cast< char* >( data.data() ), data.size() ); //bad but tolerable
    ::std::cout << __func__ << " : "
                << "Client received data : \n\t" 
                << strData << ::std::endl;
}

void clientSendCallBack( ::std::size_t sent_bytes )
{
    ::std::cout << __func__ << " : " 
                << "Client sent " 
                << sent_bytes << "bytes." 
                << ::std::endl;
}


void sigIngHandler ( int signal )
{
    keepRunning = false;
}

#define LOCAL_HOST_IP       "127.0.0.1"
/* On unix systems, 
 * the first 1024 port are restricted to the root user only, 
 * so if serverPort < 1024 you should try something > 1024 */
#define PORT_TO_USE         2000
#define TRANSACTION_DELAY   ::std::chrono::milliseconds(1000)

int main( int, char** )
{

    ::TcpIp::Server server;
    ::TcpIp::Server::Config serverConfig = 
    {
        .m_recv_cb = serverReadCallBack,
        .m_send_cb = serverSendCallBack,
        .m_address      = LOCAL_HOST_IP,
        .m_port_num      = PORT_TO_USE,
    };
    server.setConfig( ::std::move( serverConfig ) );
    server.start();

    ::TcpIp::Client client;
    ::TcpIp::Client::Config clientConfig = 
    {
        .m_recv_cb = clientReadCallBack,
        .m_send_cb = clientSendCallBack,
        .m_address      = LOCAL_HOST_IP,
        .m_port_num      = PORT_TO_USE,
    };
    client.setConfig( ::std::move( clientConfig ) );
    client.start();

    std::signal(SIGINT, sigIngHandler);

    while (keepRunning)
    {
        ::std::this_thread::sleep_for( TRANSACTION_DELAY );
        client.send( ::std::string{ "DATA_FROM_CLIENT" } );
        ::std::cout << "--" << ::std::endl;

        /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
        /* !!!Make human readable authentication as in UnixServer!!! */
        /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

        ::std::this_thread::sleep_for( TRANSACTION_DELAY );
        server.send< ::std::string >( 
                    ::std::string{ LOCAL_HOST_IP },
                    ::std::string{ "DATA_TO_CLIENT" } );
        ::std::cout << "--" << ::std::endl;
    }
}
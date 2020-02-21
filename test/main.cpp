#include <iostream>
#include <csignal>

#include "TcpIp.h"

bool keepRunning = true;

/*--------------------------*/
/*--- Server's callbacks ---*/
/*--------------------------*/
void serverReadCallBack( ::std::string data )
{
    ::std::cout << __func__ << " : "
                << "Server received data : \n\t" 
                << data << ::std::endl;
}

void serverSendCallBack( ::std::size_t sent_bytes )
{
    ::std::cout << __func__ << " : "
                << "Server sent " 
                << sent_bytes << "bytes." 
                << ::std::endl;
}

void serverErrorCallBack( const ::std::string& error )
{
    ::std::cout << "Received error : '" << error << "'"<< ::std::endl;
}

/*--------------------------*/
/*--- Client's callbacks ---*/
/*--------------------------*/
void clientReadCallBack( ::std::string data )
{
    ::std::cout << __func__ << " : "
                << "Client received data : \n\t" 
                << data << ::std::endl;
}

void clientSendCallBack( ::std::size_t sent_bytes )
{
    ::std::cout << __func__ << " : " 
                << "Client sent " 
                << sent_bytes << "bytes." 
                << ::std::endl;
}

void clientErrorCallBack( const ::std::string& error)
{
    ::std::cout << "Received error from server : '" << error << ::std::endl;
}


void sigIntHandler ( int signal )
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
        .m_recv_cb  = serverReadCallBack,
        .m_send_cb  = serverSendCallBack,
        .m_error_cb = serverErrorCallBack,
        .m_address  = LOCAL_HOST_IP,
        .m_port_num = PORT_TO_USE,
    };
    server.setConfig( ::std::move( serverConfig ) );
    server.start();

    ::TcpIp::Client client;
    ::TcpIp::Client::Config clientConfig = 
    {
        .m_recv_cb  = clientReadCallBack,
        .m_send_cb  = clientSendCallBack,
        .m_error_cb = clientErrorCallBack,
        .m_address  = LOCAL_HOST_IP,
        .m_port_num = PORT_TO_USE,
    };
    client.setConfig( ::std::move( clientConfig ) );
    client.start();

    std::signal(SIGINT, sigIntHandler);

    while (keepRunning)
    {
        ::std::this_thread::sleep_for( TRANSACTION_DELAY );
        client.send( ::std::string{ "DATA_FROM_CLIENT" } );
        ::std::cout << "--" << ::std::endl

        ::std::this_thread::sleep_for( TRANSACTION_DELAY );
        server.broadCast< ::std::string >( ::std::string{ "DATA_TO_CLIENTs" } );
        ::std::cout << "--" << ::std::endl;
    }
}
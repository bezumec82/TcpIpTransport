#ifndef TCP_IP_SESSION_HPP
#define TCP_IP_SESSION_HPP

#include "TcpIp.h"

namespace TcpIp
{

template< typename Data >
Result Server::Session::send( Data&& data )
{
    ::boost::asio::async_write( m_socket, 
    ::boost::asio::buffer(
        ::std::forward<Data>(data).data(), 
        ::std::forward<Data>(data).size() ),
    [&]( const boost::system::error_code& error, ::std::size_t bytes_transferred )
    {
        if (!error)
        {
            m_parent_ptr->getConfig().m_send_cb( bytes_transferred );
            PRINTF( GRN, "%lu bytes is sent.\n", bytes_transferred );
        }
        else
        {
            PRINT_ERR( "Error when writing : %s\n", error.message().c_str());
            if( m_socket.is_open() )
            {
                m_socket.shutdown( Socket::shutdown_send );
            }
        }
    } );
}

} //end namespace TcpIp

#endif /* TCP_IP_SESSION_HPP */
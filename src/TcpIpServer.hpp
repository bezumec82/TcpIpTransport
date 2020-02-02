#ifndef TCP_IP_SERVER_HPP
#define TCP_IP_SERVER_HPP

#include "TcpIp.h"

namespace TcpIp
{
/* Wrapper around 'Session.send' */
template< typename Data >
Result Server::send( const ::std::string& address, Data&& data )
{
    if( ! m_is_started.load() )
    {
        PRINT_ERR( "Trying to use not started server.\n" );
        throw ::std::runtime_error( "Trying to use not started server.\n" );
        return Result::SEND_ERROR;
    }
    /* Hash will be better here : use 'unordered_map' */
    for( auto & it : m_sessions )
    {
        if ( it.getIp() == address )
        {
            it.send( ::std::forward<Data>(data) ); //perfect forwarding
            return Result::SEND_SUCCESS; 
        }
    } //end for
    return Result::NO_SUCH_CLIENT;
}//end send

} //end namespace TcpIp

#endif /* TCP_IP_HPP */
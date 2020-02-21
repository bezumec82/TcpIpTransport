#ifndef TCP_IP_SERVER_HPP
#define TCP_IP_SERVER_HPP

#include "TcpIp.h"

namespace TcpIp
{

template< typename Data >
Result Server::send( SessionView& session, Data&& data )
{
    if( session.getValid() )
    {
        session.session()->send( ::std::forward<Data>(data) );
        return Result::SEND_SUCCESS;
    }
    else
    {
        return Result::SEND_ERROR;
    }
}

template< typename SessionViewList, typename Data >
Result Server::multiCast( SessionViewList&& sessions, Data&& data )
{
    for( auto& it : sessions )
    {
        this->send( it, ::std::forward<Data>(data) );
    }
}

template< typename Data >
Result Server::broadCast( Data&& data )
{
    ::std::lock_guard< ::std::mutex > lock( m_sessions_mtx ); //access to sessions
    for( auto& it : m_sessions )
    {
        if( it.getValid() )
            it.send( ::std::forward<Data>(data) );
    }
    return Result::SEND_SUCCESS;
}

template< typename SessionType >
void Server::removeSession( SessionType&& session )
{
    PRINTF( RED, "Removing session.\n" );
    ::std::lock_guard< ::std::mutex > lock( m_sessions_mtx );
    m_sessions.erase( session.session() );
}


} //end namespace TcpIp

#endif /* TCP_IP_HPP */
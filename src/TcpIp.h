#ifndef TCP_IP_H
#define TCP_IP_H

#include <iostream>
#include <functional>
#include <memory>
#include <list>
#include <type_traits>

#include <boost/asio.hpp>
#include <boost/tti/has_member_function.hpp>

#include "Tools.h"

namespace TcpIp
{
    class Server; //forward
    using byte = uint8_t;

    using IoService = ::boost::asio::io_service;
    using ErrCode = boost::system::error_code;

    using Socket = ::boost::asio::ip::tcp::socket;
    using SocketUptr = ::std::unique_ptr< Socket >;
    using IpAddress = ::boost::asio::ip::address;
    using EndPoint = ::boost::asio::ip::tcp::endpoint;
    using EndPointUptr = ::std::unique_ptr<EndPoint>;

    using Acceptor = ::boost::asio::ip::tcp::acceptor;
    using AcceptorUptr = ::std::unique_ptr<Acceptor>;

    using Buffer = ::std::string;
    using BufferShPtr = ::std::shared_ptr< Buffer >;
    using ErrorDescription = ::std::string;
}


namespace TcpIp
{
    /*--- Definitions ---*/
    #define RECV_BUF_SIZE    1024

    /* Used by both : Server and Client */
    enum class Result // : int8_t
    {
        SEND_ERROR          = -4,
        NO_SUCH_CLIENT      = -3,
        CFG_ERROR           = -2,
        WRONG_IP_ADDRESS    = -1,
        ALL_GOOD            = 0,
        SEND_SUCCESS        = 1,
    };

    class Server /* Default constructable */
    {
    private : /*--- Forward declarations ---*/
        class Session;
    public :
        class SessionView;

    public : /*--- Aliases ---*/
        using SessionShptr  = ::std::shared_ptr< Session >;
        using Sessions      = ::std::list< Session >;
        using SessionHandle = Sessions::iterator;

        using RecvCallBack  = void( const SessionView&, ::std::string& );
        using SendCallBack  = void( const SessionView&, ::std::size_t );
        using ErrorCallBack = void( const SessionView&, const ErrorDescription& );

    public : /*--- Structures/Classes/Enums ---*/
        struct Config
        {
            /* Can be used directly in async calls */
            ::std::function< RecvCallBack >  m_recv_cb;
            ::std::function< SendCallBack >  m_send_cb;
            ::std::function< ErrorCallBack > m_error_cb;
            ::std::string   m_address; /* IP address to use */
            uint16_t        m_port_num;
            ::std::string   m_delimiter;
                /* Each message should have some kind of start-end sequence :
                <body>
                    ...
                </body>
                */
        }; //end struct Config

    private : /* No access to the Session from outside */
        class Session
        {
            friend class Server; //access to the flags
            friend class SessionView;
        public : /*--- Methods ---*/
            /* Session knows about its parent - class Server */
            Session(IoService& io_service, Server * parent)
                : m_io_service_ref( io_service ), //to post events
                m_socket( io_service ),
                m_parent_ptr( parent )
            { }

            Socket& socket( void )
            {
                return m_socket;
            }
            ::std::string getRemoteIp( void )
            {
                /* TO DO : find better way. Copy ellision not work here.
                 * m_socket.remote_endpoint().address().to_string() IS DEPRECATED */
                ::std::ostringstream address;
                address << m_socket.remote_endpoint().address();
                return address.str();
            }
            void saveHandle( SessionHandle& self )
            {
                m_self = ::std::make_unique< SessionView >( self );
            }
            void setValid( bool state )
            {
                m_is_valid.store( state );
            }
            bool isValid() const
            {
                return m_is_valid.load();
            }
            /* Taking self to prevent destruction shared ptr */
            void recv( void );
            void recv( ::std::string& );
            template< typename Data >
            Result send( Data&& );
            ~Session();
        private : /*--- Variables ---*/
            IoService& m_io_service_ref;
            Socket m_socket;
            Server * m_parent_ptr;
            ::std::unique_ptr< SessionView > m_self;
        private : /*--- Constants ---*/
            const int READ_BUF_SIZE = 4096;
        private : /*--- Flags ---*/
            ::std::atomic< bool > m_is_valid{ false };
        }; //end class Session

    public :
        class SessionView
        {
            friend class Server;
            friend class Session;
        public : /*--- Constructor ---*/
            SessionView( SessionHandle& session )
            : m_session( session )
            { }
        private : /*--- Getters/Setters ---*/
            SessionHandle& session()
            {
                return m_session;
            }
        public :
            bool isValid() const //provides view to the state from the outside
            {
                m_session->isValid();
            }
        private : /*--- Variables ---*/
            SessionHandle& m_session;
        }; //end class SessionView

    public : /*--- Methods ---*/
        Result setConfig( Config&& );
        Config& getConfig( void )
        {
            return m_config;
        }
        Result start( void );

        /* Send function is callable, recv is event. */;
        template< typename Data >
            Result send( SessionView& , Data&& );
        template< typename SessionViewList , typename Data >
            Result multiCast( SessionViewList&& , Data&& );
        template< typename Data >
            Result broadCast( Data&& );

        BOOST_TTI_HAS_MEMBER_FUNCTION(session)
        template< typename SessionType >
        void removeSession( SessionType&& session )
        {
            
            ::std::lock_guard< ::std::mutex > lock( m_sessions_mtx );
            if constexpr ( has_member_function_session< SessionType, SessionHandle& >::value )
            {
                PRINTF( RED, "Removing session.\n" );
                m_sessions.erase( session.session() );
            }
            else if constexpr ( ::std::is_same< SessionType, Sessions::iterator >::value )
            {
                m_sessions.erase( session );
            }
        }
        
        ~Server();
    private :
        void accept( void );

    private : /*--- Variables ---*/
        Config m_config;
        AcceptorUptr m_acceptor_uptr;
        IpAddress m_address;
        EndPointUptr m_endpoint_uptr;

        Sessions m_sessions;
        ::std::mutex m_sessions_mtx; //protect access to the sessions data

        IoService m_io_service; //Server has its own io_service
        ::std::thread m_worker;
        ::std::future<void> m_future;

        /*--- Flags ---*/
        ::std::atomic< bool > m_is_configured{ false };
        ::std::atomic< bool > m_is_started{ false };
    }; //end class Server



    class Client
    {
    public : /*--- Aliases ---*/
        using RecvCallBack  = void( ::std::string& );
        using SendCallBack  = void( ::std::size_t );
        using ErrorCallBack = void( const ErrorDescription& );

    public : /*--- Structures/Classes/enums ---*/
        struct Config
        {
            ::std::function< RecvCallBack >   m_recv_cb;
            ::std::function< SendCallBack >   m_send_cb;
            ::std::function< ErrorCallBack >  m_error_cb;

            ::std::string   m_address;
            uint16_t        m_port_num;
            ::std::string   m_delimiter; //look 'Server::Config'
        }; //end struct Config

    public : /*--- Methods ---*/
        Result setConfig( Config&& );
        Result start( void );
        template< typename Data >
        void send( Data&& data );
        ~Client()
        {
            if( m_socket_uptr!= nullptr )
                m_socket_uptr->close();
        }
    private :
        void connect( void );
        void recv( void );
        void recv( ::std::string& );
    private : /*--- Variables ---*/
        Config m_config;
        SocketUptr m_socket_uptr;
        IpAddress m_address;
        EndPointUptr m_endpoint_uptr;
        IoService m_io_service;
        ::std::thread m_worker;
        /*--- Constants ---*/
        const int READ_BUF_SIZE = 4096;
        /*--- Flags ---*/
        ::std::atomic< bool > m_is_configured{ false };
        ::std::atomic< bool > m_is_connected{ false };
    }; //class Client

}; //end namespace TcpIp

#include "TcpIpServer.hpp"
#include "TcpIpSession.hpp"
#include "TcpIpClient.hpp"

#endif /* TCP_IP_H */
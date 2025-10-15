#ifndef CONNECT_H
#define CONNECT_H

#include "macros.h"
#include <boost/asio.hpp>
#include <list>
#include <memory>
#include <mutex>
#include <string>




class CConnect : public std::enable_shared_from_this<CConnect>
{
public:
    class IConEvent
    {
    public:
        virtual int OnConnected(int64_t id, std::shared_ptr<CConnect> pConnect)    = 0;
        virtual int OnHandlePacket(int64_t id, std::shared_ptr<CConnect> pConnect) = 0;
        virtual int OnClosed(int64_t id)                                           = 0;
    };

    CConnect(boost::asio::ip::tcp::socket&& _socket);
    CConnect(boost::asio::io_context& ioc);

    virtual ~CConnect();

    virtual void Start();

    virtual void Close();

    void SendData(std::shared_ptr<std::string> pdata);

    void ReadCb(boost::system::error_code er, size_t length);

    void SendCb(boost::system::error_code er, size_t length);

protected:
    template<typename T>
    std::shared_ptr<T> shared_from_base()
    {
        return std::dynamic_pointer_cast<T>(shared_from_this());
    }


    virtual void DoRead();

    virtual void DoSend();



    void NotifyClose();

    void HandlePacket();

    char m_recv_buffer[RECV_BUFF_LEN];

    std::string m_recv_data;

    std::list<std::shared_ptr<std::string>> m_list_send;


    boost::asio::ip::tcp::socket m_socket;

    bool m_need_close{false};
    int  m_is_client{-1};

    int64_t m_id{0};

    std::shared_ptr<IConEvent> m_pEvent;
};


#endif
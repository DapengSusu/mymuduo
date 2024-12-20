#include <mymuduo/tcpserver.h>
#include <mymuduo/logger.h>

#include <string>

using namespace std::placeholders;

class EchoServer
{
public:
    EchoServer(EventLoop* loop, const InetAddress& addr, const std::string& name)
        : loop_(loop)
        , server_(loop_, addr, name)
    {
        // 注册回调函数
        server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, _1));
        server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, _1, _2, _3));

        // 设置合适的loop线程数量
        server_.setThreadNum(3);
    }

    void start()
    {
        server_.start();
    }

private:
    // 连接建立或断开的回调
    void onConnection(const TcpConnectionPtr& conn)
    {
        if (conn->connected()) {
            LOG_INFO("New connection [%s] up", conn->peerAddr().toIpPort().c_str());
        } else {
            LOG_INFO("Connection [%s] down", conn->peerAddr().toIpPort().c_str());
        }
    }

    // 可读写事件的回调
    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
    {
        conn->send(buf->retrieveAllAsString());
        conn->shutdown(); // 关闭写端
    }

    EventLoop* loop_;
    TcpServer server_;
};

int main()
{
    EventLoop loop;
    InetAddress addr(8000);
    EchoServer server(&loop, addr, "EchoServer-01");

    server.start();
    loop.loop(); // 启动mainLoop的底层poller

    return 0;
}

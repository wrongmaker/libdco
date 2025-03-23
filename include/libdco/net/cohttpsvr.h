#pragma once

#include "boost/asio/ip/detail/endpoint.hpp"
#include "boost/asio/ssl/context.hpp"
#include "boost/beast/http/message.hpp"
#include "libdco/all.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/core.hpp>
#include <iostream>
#include <memory.h>
#include <string>
#include <unordered_map>

namespace dco
{
  namespace beast = boost::beast;   // from <boost/beast.hpp>
  namespace http = beast::http;     // from <boost/beast/http.hpp>
  namespace net = boost::asio;      // from <boost/asio.hpp>
  namespace ssl = boost::asio::ssl; // from <boost/asio/ssl.hpp>
  using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

  template <class cohttp_steam>
  class cohttpsvr_base
  {
  public:
    struct cohttp_response
    {
      cohttp_steam &stream_;
      bool &close_;
      beast::error_code &ec_;

      explicit cohttp_response(cohttp_steam &stream, bool &close,
                               beast::error_code &ec)
          : stream_(stream), close_(close), ec_(ec) {}

      template <bool isRequest, class Body, class Fields>
      void operator()(http::message<isRequest, Body, Fields> &&msg) const
      {
        // Determine if we should close the connection after
        close_ = msg.need_eof();

        // We need the serializer here because the serializer requires
        // a non-const file_body, and the message oriented version of
        // http::write only works with const messages.
        http::serializer<isRequest, Body, Fields> sr{msg};
        http::write(stream_, sr, ec_);
      }
    };

  public:
    typedef std::function<void(const http::request<http::string_body> &,
                               cohttp_response &)>
        http_session;
    typedef std::unordered_map<std::string, http_session> http_dispachter;

  private:
    bool run_;
    net::io_context ioc_;
    std::shared_ptr<tcp::acceptor> acceptor_;
    std::shared_ptr<http_dispachter> get_session_dispatcher_;
    std::shared_ptr<http_dispachter> post_session_dispatcher_;
    std::shared_ptr<http_dispachter> put_session_dispatcher_;

  protected:
    std::shared_ptr<http_dispachter> default_session_dispatcher_;

  public:
    cohttpsvr_base(const std::string &host, const unsigned short port)
        : run_(false)
    {
      auto const address = net::ip::make_address(host);
      acceptor_ =
          std::make_shared<tcp::acceptor>(ioc_, tcp::endpoint{address, port});
      get_session_dispatcher_ = std::make_shared<http_dispachter>();
      post_session_dispatcher_ = std::make_shared<http_dispachter>();
      put_session_dispatcher_ = std::make_shared<http_dispachter>();
      default_session_dispatcher_ = std::make_shared<http_dispachter>();
    }
    virtual ~cohttpsvr_base() {}

  protected:
    // create a coroutine and launch
    virtual void go_session(std::shared_ptr<tcp::socket> socket) = 0;

    std::shared_ptr<http_dispachter> get_dispacher(beast::http::verb method)
    {
      std::shared_ptr<http_dispachter> dispatcher = default_session_dispatcher_;
      if (method == beast::http::verb::get)
      {
        dispatcher = get_session_dispatcher_;
      }
      else if (method == beast::http::verb::post)
      {
        dispatcher = post_session_dispatcher_;
      }
      else if (method == beast::http::verb::put)
      {
        dispatcher = put_session_dispatcher_;
      }
      return dispatcher;
    }

  protected:
    static void do_dispatcher(const http::request<http::string_body> &req,
                              std::shared_ptr<http_dispachter> dispatcher,
                              std::shared_ptr<http_dispachter> default_dispatcher,
                              cohttp_response &response)
    {
    auto url = std::string(req.target());
    auto query_pos = url.find('?');
    if (query_pos != std::string::npos) url = url.substr(0, query_pos);
    auto session_call = dispatcher->find(url);
      if (session_call == dispatcher->end())
      {
        session_call = default_dispatcher->find("404");
        BOOST_ASSERT(session_call != default_dispatcher->end());
      }
      session_call->second(req, response);
    }

  public:
    void register_404(const http_session &session)
    {
      (*default_session_dispatcher_)["404"] = session;
    }

    void register_uri(beast::http::verb method, const std::string &uri, const http_session &session)
    {
      std::shared_ptr<http_dispachter> dispatcher = get_dispacher(method);
      (*dispatcher)[uri] = session;
    }

    void accept()
    {
      if (run_)
        return;
      run_ = true;
      dco_go[this]
      {
        for (;;)
        {
          // This will receive the new connection
          std::shared_ptr<tcp::socket> socket =
              std::make_shared<tcp::socket>(ioc_);

          // Block until we get a connection
          acceptor_->accept(*socket);

          // Launch the session, transferring ownership of the socket
          go_session(socket);
        }
      };
    }
  };

  class cohttpsvr : public cohttpsvr_base<tcp::socket>
  {
  private:
  public:
    cohttpsvr(const std::string &host, const unsigned short port)
        : cohttpsvr_base<tcp::socket>(host, port) {}

  private:
    virtual void go_session(std::shared_ptr<tcp::socket> socket);

    void do_session(std::shared_ptr<tcp::socket> socket);

  public:
  };

  class cohttpsvr_ssl : public cohttpsvr_base<beast::ssl_stream<tcp::socket &>>
  {
  private:
    std::shared_ptr<ssl::context> ssl_ctx_;

  public:
    cohttpsvr_ssl(const std::string &host, const unsigned short port)
        : cohttpsvr_base<beast::ssl_stream<tcp::socket &>>(host, port) {}

  private:
    virtual void go_session(std::shared_ptr<tcp::socket> socket);

    void do_session(std::shared_ptr<tcp::socket> socket,
                    std::shared_ptr<ssl::context> ctx);

  public:
    bool load_certificate(const ssl::context::method &mod,
                          const ssl::context::options &op,
                          const std::string &cert, const std::string &key,
                          const std::string &dh);
  };
} // namespace dco

// resp和req直接裸着来吧,这样自由度最高

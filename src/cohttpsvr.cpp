#include "libdco/net/cohttpsvr.h"
#include "boost/asio/ip/detail/endpoint.hpp"
#include "boost/asio/ssl/context.hpp"
#include <memory>

using namespace dco;

void cohttpsvr::go_session(std::shared_ptr<tcp::socket> socket,
                           std::shared_ptr<http_dispachter> dispatcher) {
  // create coroutine task and launch
  std::function<void()> task =
      std::bind(&cohttpsvr::do_session, this, socket, dispatcher);
  dco_go task;
}

void cohttpsvr::do_session(std::shared_ptr<tcp::socket> socket,
                           std::shared_ptr<http_dispachter> dispatcher) {
  bool close = false;
  beast::error_code ec;

  // This buffer is required to persist across reads
  beast::flat_buffer buffer;

  // This lambda is used to send messages
  cohttp_response lambda{*socket, close, ec};

  for (;;) {
    // Read a request
    http::request<http::string_body> req;
    http::read(*socket, buffer, req, ec);
    if (ec == http::error::end_of_stream)
      break;
    if (ec) {
      dco_error("do_session recv fail ec:{}", ec.what());
      return;
    }

    // Get target session call
    do_dispatcher(req, dispatcher, lambda);

    if (ec) {
      dco_error("do_session resp fail ec:{}", ec.what());
      return;
    }
    if (close) {
      // This means we should close the connection, usually because
      // the response indicated the "Connection: close" semantic.
      break;
    }
  }

  // Send a TCP shutdown
  socket->shutdown(tcp::socket::shutdown_send, ec);

  // At this point the connection is closed gracefully
}

void cohttpsvr_ssl::go_session(std::shared_ptr<tcp::socket> socket,
                               std::shared_ptr<http_dispachter> dispatcher) {
  BOOST_ASSERT(ssl_ctx_ != nullptr);
  // create coroutine task and launch
  std::function<void()> task =
      std::bind(&cohttpsvr_ssl::do_session, this, socket, dispatcher, ssl_ctx_);
  dco_go task;
}

void cohttpsvr_ssl::do_session(std::shared_ptr<tcp::socket> socket,
                               std::shared_ptr<http_dispachter> dispatcher,
                               std::shared_ptr<ssl::context> ctx) {
  bool close = false;
  beast::error_code ec;

  // Construct the stream around the socket
  beast::ssl_stream<tcp::socket &> stream{*socket, *ctx};

  // Perform the SSL handshake
  stream.handshake(ssl::stream_base::server, ec);
  if (ec)
    return;

  // This buffer is required to persist across reads
  beast::flat_buffer buffer;

  // This lambda is used to send messages
  cohttp_response lambda{stream, close, ec};

  for (;;) {
    // Read a request
    http::request<http::string_body> req;
    http::read(stream, buffer, req, ec);
    if (ec == http::error::end_of_stream)
      break;
    if (ec) {
      dco_error("do_session recv fail ec:{}", ec.what());
      return;
    }

    // Get target session call
    do_dispatcher(req, dispatcher, lambda);

    if (ec) {
      dco_error("do_session resp fail ec:{}", ec.what());
      return;
    }
    if (close) {
      // This means we should close the connection, usually because
      // the response indicated the "Connection: close" semantic.
      break;
    }
  }

  // Perform the SSL shutdown
  stream.shutdown(ec);
  if (ec)
    return;

  // At this point the connection is closed gracefully
}

bool cohttpsvr_ssl::load_certificate(const ssl::context::method &mod,
                                     const ssl::context::options &op,
                                     const std::string &cert,
                                     const std::string &key,
                                     const std::string &dh) {
  if (ssl_ctx_ == nullptr)
    ssl_ctx_ = std::make_shared<ssl::context>(mod);
  ssl_ctx_->set_options(op);
  ssl_ctx_->use_certificate_chain(
      boost::asio::buffer(cert.data(), cert.size()));
  ssl_ctx_->use_private_key(boost::asio::buffer(key.data(), key.size()),
                            boost::asio::ssl::context::file_format::pem);
  ssl_ctx_->use_tmp_dh(boost::asio::buffer(dh.data(), dh.size()));
  return true;
}

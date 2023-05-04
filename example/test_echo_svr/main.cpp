#include "libdco/all.h"
#include <netinet/in.h>

void client_chat(dco::coctx* ctx, int cli_fd) {
  char msg[1500];
  while (1) {
    memset(&msg, 0, sizeof(msg)); // clear the buffer
    int ret = recv(cli_fd, (char *)&msg, sizeof(msg), 0);
    if (ret <= 0 || !strcmp(msg, "exit")) {
      dco_info("client has quit the session fd:{}", cli_fd);
      close(cli_fd);
      break;
    }
    msg[ret] = 0;
    dco_info("client fd:{} get:{} ret:{}", cli_fd, msg, ret);
    memset(&msg, 0, sizeof(msg));
    sprintf(msg, "hi client, my ctx:%p id:%d", (void *)ctx, ctx->id());
    ret = send(cli_fd, (char *)&msg, strlen(msg), 0);
    dco_info("server send fd:{} back:{} ret:{}", cli_fd, msg, ret);
    // sleep(1);
    if (ret <= 0) {
      dco_info("client has quit the session fd:{}", cli_fd);
      close(cli_fd);
      break;
    }
  }
}

int main() {
  dco_init();

  // 设置日志等级为debug
  // spdlog::set_level(spdlog::level::debug);
  spdlog::set_level(spdlog::level::info);

  // echo server
  dco_go[] {
    int port = 39307;
    sockaddr_in svr_addr;
    bzero((char *)&svr_addr, sizeof(svr_addr));
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    svr_addr.sin_port = htons(port);

    int svr_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_fd < 0) {
      dco_error("error establishing the server socket");
      exit(0);
    }

    int ret = bind(svr_fd, (struct sockaddr *)&svr_addr, sizeof(svr_addr));
    if (ret < 0) {
      dco_error("error binding socket to local address");
      exit(0);
    }

    listen(svr_fd, 5);
    sockaddr_in cli_addr;
    socklen_t cli_addrSize = sizeof(cli_addr);

    for (;;) {
      dco_info("waiting for a client to connect...");
      int cli_fd = accept(svr_fd, (sockaddr *)&cli_addr, &cli_addrSize);
      if (cli_fd < 0) {
        dco_error("error accepting request from client!");
        exit(1);
      }
      dco_info("accept a client connect fd:{}", cli_fd);
      // 走个协程去处理
      dco_go[cli_fd](dco::coctx* ctx) { client_chat(ctx, cli_fd); };
      // 继续去监听
    }
  };

  dco_epoll_run();
  return 0;
}

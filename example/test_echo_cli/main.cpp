#include "libdco/all.h"
#include <cstdio>
#include <cstring>
#include <netinet/in.h>

void make_client(dco::coctx *ctx) {
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

  int ret = connect(svr_fd, (struct sockaddr *)&svr_addr, sizeof(svr_addr));
  if (ret < 0) {
    dco_error("error connect socket to local address");
    exit(0);
  }

  char msg[1500];
  while (1) {
    memset(&msg, 0, sizeof(msg)); // clear the buffer
    sprintf(msg, "hello server, my ctx:%p id:%d", (void *)ctx, ctx->id());
    ret = send(svr_fd, (char *)&msg, strlen(msg), 0);
    if (ret <= 0) {
      dco_info("server has quit the session fd:{}", svr_fd);
      close(svr_fd);
      break;
    }
    dco_info("server fd:{} send:{} ret:{}", svr_fd, msg, ret);
    memset(&msg, 0, sizeof(msg));
    ret = recv(svr_fd, (char *)&msg, sizeof(msg), 0);
    dco_info("server fd:{} recv:{} ret:{}", svr_fd, msg, ret);
    if (ret <= 0) {
      dco_info("server has quit the session fd:{}", svr_fd);
      close(svr_fd);
      break;
    }
    sleep(1);
  }
}

int main() {
  dco_init();

  // 设置日志等级为debug
  // spdlog::set_level(spdlog::level::debug);
  spdlog::set_level(spdlog::level::info);

  dco_go[](dco::coctx * ctx) { make_client(ctx); };
  dco_go[](dco::coctx * ctx) {
    dco_sleep(300);
    make_client(ctx);
  };
  dco_go[](dco::coctx * ctx) {
    dco_sleep(600);
    make_client(ctx);
  };

  dco_epoll_run();
}
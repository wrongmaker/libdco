#include "boost/beast/http/field.hpp"
#include "boost/beast/http/message.hpp"
#include "boost/beast/http/status.hpp"
#include "boost/beast/http/string_body.hpp"
#include "libdco/all.h"
#include "libdco/net/cohttpsvr.h"
#include <cstdio>
#include <cstring>
#include <netinet/in.h>

using namespace dco;

int main() {
  dco_init();
  // dco_init_worker(1);

  // 设置日志等级为debug
  spdlog::set_level(spdlog::level::info);
  // spdlog::set_level(spdlog::level::debug);

  dco::cohttpsvr svr("0.0.0.0", 9307);

  svr.register_404([](const http::request<http::string_body> &req,
                      cohttpsvr::cohttp_response &send) {
    // Returns a bad request response
    auto const bad_request = [&req](beast::string_view why) {
      http::response<http::string_body> res{http::status::bad_request,
                                            req.version()};
      res.set(http::field::server, "co_test_svr");
      res.set(http::field::content_type, "text/html");
      res.keep_alive(req.keep_alive());
      res.body() = std::string(why);
      res.prepare_payload();
      return res;
    };

    send(bad_request("bad target"));
  });

  svr.register_uri("/", [](const http::request<http::string_body> &req,
                           cohttpsvr::cohttp_response &send) {
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "co_test_svr");
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    // res.body() =
    //     "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 "
    //     "Transitional//EN\"><html><head><meta http-equiv=\"content-type\" "
    //     "content=\"text/html; "
    //     "charset=Big5\"><title></title><style>body{background-image: "
    //     "url(bg.png);background-position: center;background-repeat: "
    //     "repeat-y;background-color:#6F963C;margin:0px;}#all{text-align:center;}"
    //     ".page{width: 660px;margin: 0px auto;background-position: center "
    //     "bottom;background-repeat: no-repeat;text-align:left;}h1{color: "
    //     "gray;background-color: #ECECEC;padding:4px "
    //     "20px;margin-bottom:50px;}p{line-height: "
    //     "2.2em;color:#404040;text-align:justify;text-justify:inter-ideograph;}"
    //     "span{border-bottom: 1px dotted "
    //     "#99CC00;font-size:0.8em;padding-left:2em;padding-bottom:4px;}.header{"
    //     "height:62px;background-image: url(header.png);background-position: "
    //     "center;background-repeat: "
    //     "none;   }.foot{height:88px;background-image: "
    //     "url(foot.png);background-position: center;background-repeat: "
    //     "none;   }.pic {float: right;margin:10px;background-color: "
    //     "#000000;padding:4px;}.pic_txt {color: yellow;font-size: "
    //     "12px;margin-top:6px;}   img {border: 1px solid "
    //     "gray;}</style></head><body><div id=\"all\"><div "
    //     "class=\"header\"></div><div class=\"page\"><h1>CSS的歷史</h1><div "
    //     "class=\"pic\"><img src=\"Bert_Bos.jpg\" width=\"250\" height=\"375\" "
    //     "border=\"0\" alt=\"Bert_Bos\"><div class=\"pic_txt\">伯特·波斯（Bert "
    //     "Bos）</div>       "
    //     "</"
    //     "div><p><span>"
    //     "從1990年代初HTML被發明開始樣式表就以各種形式出現了，不同的瀏覽器結合了"
    //     "它們各自的樣式語言，讀者可以使用這些樣式語言來調節網頁的顯示方式。一開"
    //     "始樣式表是給讀者用的，最初的HTML版本只含有很少的顯示屬性，讀者來決定網"
    //     "頁應該怎樣被顯示。</span></"
    //     "p><p><span>"
    //     "但隨著HTML的成長，為了滿足設計師的要求，HTML獲得了很多顯示功能。隨著這"
    //     "些功能的增加外來定義樣式的語言越來越沒有意義了。</span></"
    //     "p><p><span>1994年哈坤·利提出了CSS的最初建議。伯特·波斯（Bert "
    //     "Bos）當時正在設計一個叫做Argo的瀏覽器，他們決定一起合作設計CSS。</"
    //     "span></"
    //     "p><p><span>"
    //     "當時已經有過一些樣式表語言的建議了，但CSS是第一個含有「層疊」的主意的"
    //     "。在CSS中，一個文件的樣式可以從其他的樣式表中繼承下來。讀者在有些地方"
    //     "可以使用他自己更喜歡的樣式，在其他地方則繼承，或「層疊」作者的樣式。這"
    //     "種層疊的方式使作者和讀者都可以靈活地加入自己的設計，混合各人的愛好。</"
    //     "span></"
    //     "p><p><span>"
    //     "哈坤于1994年在芝加哥的一次會議上第一次展示了CSS的建議，1995年他與波斯"
    //     "一起再次展示這個建議。當時W3C剛剛建立，W3C對CSS的發展很感興趣，它為此"
    //     "組織了一次討論會。哈坤、波斯和其他一些人（比如微軟的托馬斯·雷爾登）是"
    //     "這個項目的主要技術負責人。1996年底，CSS已經完成。1996年12月CSS要求的第"
    //     "一版本被出版。</span></"
    //     "p><p><span>"
    //     "1997年初，W3C內組織了專門管CSS的工作組，其負責人是克里斯·里雷。這個工"
    //     "作組開始討論第一版中沒有涉及到的問題，其結果是1998年5月出版的第二版要"
    //     "求。到2004年為止，第三版還未出版。</span></p></div><div "
    //     "class=\"foot\"></div></div></body></html>";
    res.body() = "Hello, World!";
    res.prepare_payload();

    send(std::move(res));
  });

  svr.accept();

  dco_epoll_run();
}

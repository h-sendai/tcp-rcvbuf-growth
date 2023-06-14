# tcp-rcvbuf-growth

起動方法:
```
./tcp-rcvbuf-growth 
[-d (debug)] [-b bufsize (4k)] [-n n_read (1000)] [-p port (1234)] [-r rcvbuf_size (auto tune)] remote_addr

-b bufsize  read(sockfd, buf, bufsize)のbufsizeの指定
-n n_read   read()する回数-2を指定(-2の分については下記参照)
-p port     remote port
-r rcvbuf   手動でrcvbufサイズを指定(自動調節との比較に使う)
remote_addr 接続先ホスト名あるいはIPアドレス
```

Linuxではソケットレシーブバッファの大きさが自動調節
されるが、実際にどのくらいの大きさになっているかテストする
プログラム。

``read(sockfd, buf, bufsize)``でread()した直後に``SO_RCVBUF``サイズ
を取得する。

取得したデータはメモリに保存することにして、終了時に結果を出力する。
出力フォーマットは

```
TCPソケットを作ってからの経過時間 read()したバイト数 RCVBUFサイズ（単位はバイト）
```

1行目のデータはTCPソケットを作った直後のデータ、
2行目のデータはconnect()直後のデータになっているのでこれらは
read()したバイト数は0になっている。

# chttp

Van's first http server in C. A learning result.

Primitive and limited support for HTTP/1.1 and __HTTP/2__.

```sh
# generate self-signed certification: conf/server.key, conf/server.crt
# http://www.akadia.com/services/ssh_test_certificate.html

make -s OPTFLAGS=-DDEBUG chttp && bin/chttp_test.bin

# add local.test.com to /etc/hosts
echo '127.0.0.1 local.test.com' >> /etc/hosts

# visit: https://local.test.com:8443/
```

## Helps

* [Hypertext Transfer Protocol Version 2 (HTTP/2)](http://httpwg.org/specs/rfc7540.html)
* [HPACK: Header Compression for HTTP/2](http://httpwg.org/specs/rfc7541.html)
* [Tutorial: HTTP/2 server](https://nghttp2.org/documentation/tutorial-server.html)
* [Tools for debugging, testing and using HTTP/2](https://blog.cloudflare.com/tools-for-debugging-testing-and-using-http-2/)
* https://github.com/ppelleti/https-example/blob/master/https-server.c


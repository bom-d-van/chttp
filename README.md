# chttp

Van's first http server in C. A learning result.

Primitive and limited support HTTP/1.1 and HTTP/2.

```sh
# generate self-signed certification: conf/server.key, conf/server.crt
# http://www.akadia.com/services/ssh_test_certificate.html

make -s OPTFLAGS=-DDEBUG chttp && bin/chttp_test.bin

# add local.test.com to /etc/hosts
echo '127.0.0.1 local.test.com' >> /etc/hosts

# visit: https://local.test.com:8443/
```

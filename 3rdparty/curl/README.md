# libcurl

Pre-built libcurl with only HTTP/HTTPS (& gzip) enabled.

For scripts and updates go [here](https://github.com/den-mentiei/workbench-libcurl).

## SSL

The library is compiled with only 1 default SSL-backend:
* _TODO_ Windows - native WinSSL (SChannel).
* _TODO_ macOS/iOS - native Secure Transport.
* Linux - built-in [mbedtls](https://github.com/ARMmbed/mbedtls).
* _TODO_ Android - built-in [mbedtls](https://github.com/ARMmbed/mbedtls).

## Used commits
 202c1cc22fc3b83dff93b7181f8eb3cc1a8e4a3f curl (curl-7_55_1-142-g202c1cc)
 72ea31b026e1fc61b01662474aa5125817b968bc mbedtls (mbedtls-2.6.0)
 cacf7f1d4e3d44d871b605da3b647f07d718623f zlib (v1.2.11)

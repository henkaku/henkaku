# HENkaku Offline

You need to host two things: the first stage ROP and the second stage dynamic ROP. The first stage is just static HTML/JS and can be hosted using any means. The second stage must run our custom server. We provided a PHP and Go implementation (Go is recommended as it can handle ~1000x more requests per second).

## Requirements

* Python 3 (and the `python3` command)
* (Optional) PHP
* (Optional) Go

## Building

`YOUR_STAGE_2_URL` will point to where you are hosting stage 2 (must end with a / if you're using the Go server and must point to stage2.php if you're using PHP) and `YOUR_PKG_PREFIX_URL` will point to where you host the molecularShell package (we put it on the same static server as the first stage exploit files). Both URLs have a 255 character limit. Then call

```shell
./build.sh YOUR_STAGE_2_URL YOUR_PKG_PREFIX_URL
```

For example, if my server is at `192.168.1.100` and I run a web server on port 8888 serving the exploit files along with `pkg` and I run the stage 2 Go server on port 4000, then I would run

```shell
./build.sh http://192.168.1.100:4000/ http://192.168.1.100:8888/pkg
```

The built files will be in the `./host` directory.

## Running

Your stage 1 server can be any web server (Apache for example). For simplicity, you can run the following Python command to start a server at the current directory on port 8888

```shell
python -m SimpleHTTPServer 8888
```

Your stage 2 server can either be run as the PHP file (in that case `YOUR_STAGE_2_URL` would point to the PHP script) or as the Go server (your `YOUR_STAGE_2_URL` would point to the right port number). As an example, to run the Go server on port 4000, you can run

```shell
go run stage2.go -payload stage2.bin -port 4000
```

You can then visit `http://192.168.1.100:8888/exploit.html` to start the installation.

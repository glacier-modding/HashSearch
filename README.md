# HashSearch
Backend powering [https://hitmandb.glaciermodding.org/](https://hitmandb.glaciermodding.org/)

Originally created by [2kpr](https://github.com/2kpr), uploaded on behalf of them.

## Building

### Linux
`g++ hashsearch.cpp -lfcgi++ -lfcgi -O2 -o hashsearch`

## Running

### Linux
`spawn-fcgi -p 8000 -n hashsearch`

#### Systemd service
```conf
[Unit]
Description=hashsearch
[Service]
Type=simple
ExecStart=spawn-fcgi -p 8000 -n /usr/bin/hashsearch
Restart=on-failure
RestartSec=3s
[Install]
WantedBy=multi-user.target
```

#### Nginx FastCGI
```nginx
server {
        location /search {
                fastcgi_pass 127.0.0.1:8000;

                fastcgi_param   GATEWAY_INTERFACE       CGI/1.1;
                fastcgi_param   SERVER_SOFTWARE         nginx;
                fastcgi_param   QUERY_STRING            $query_string;
                fastcgi_param   REQUEST_METHOD          $request_method;
                fastcgi_param   CONTENT_TYPE            $content_type;
                fastcgi_param   CONTENT_LENGTH          $content_length;
                fastcgi_param   SCRIPT_FILENAME         $document_root$fastcgi_script_name;
                fastcgi_param   SCRIPT_NAME             $fastcgi_script_name;
                fastcgi_param   REQUEST_URI             $request_uri;
                fastcgi_param   DOCUMENT_URI            $document_uri;
                fastcgi_param   DOCUMENT_ROOT           $document_root;
                fastcgi_param   SERVER_PROTOCOL         $server_protocol;
                fastcgi_param   REMOTE_ADDR             $remote_addr;
                fastcgi_param   REMOTE_PORT             $remote_port;
                fastcgi_param   SERVER_ADDR             $server_addr;
                fastcgi_param   SERVER_PORT             $server_port;
                fastcgi_param   SERVER_NAME             $server_name;

        if ($request_method = OPTIONS) {
                return 204;
        }

        add_header Access-Control-Allow-Origin *;
        add_header Access-Control-Max-Age 3600;
        add_header Access-Control-Expose-Headers Content-Length;
        add_header Access-Control-Allow-Headers "Range, Content-Type";
        }
}
```
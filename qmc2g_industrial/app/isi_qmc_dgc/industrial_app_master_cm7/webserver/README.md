# Core IoT HTTP server

A modular HTTP server for IoT



## Dependencies

* lwip application layer tcp interface.
* newlib-nano /newlib  or a different C library  implementing *funopen(3)* and *fmemopen(3)* and *strsep(3)*.
* for extending the parser the *ragel* state machine compiler is needed.
* coreJSON for plug_json.

##  

## Sources

| Important Source files  | Description              | Usage                             |
| ----------------------- | ------------------------ | --------------------------------- |
| cit_httpd.h             | server include file      | To be included by the application |
| cit_httpd_opt.h         | server configuration     |                                   |
| constants.h             | defined constants        |                                   |
| httpd.c                 | server implementation    |                                   |
| plug.h                  | plugin system header     | The plugin API                    |
| parser/http11_parser.rl | ragel parser source code |                                   |

## Server Configuration

The server is started by `void cit_httpd_init(struct cit_httpd_config *conf)` the configuration has the following fields:

| Field    | Type               | Notes                                              |
| -------- | ------------------ | -------------------------------------------------- |
| tls      | altcp_tls_config   | optional TLS configuration                         |
| rule_cnt | u16_t              | number of rules                                    |
| rule_vec | const plug_rule_t* | pointer to static ruleset                          |
| ip_addr  | ip_addr_t*         | optional lwip ip address                           |
| port     | u16_t              | port, if 0 defaults to 80 or 443 for HTTP or HTTPS |
| u8_t     | ip_type            | lwip ip_type                                       |

## Plugins provided by the webserver

a request plugin (short plug) is used to handle request that match a rule, you can create your own for your application.

| Plug              | Description                                                  | Flags                                                        | Options              |
| ----------------- | ------------------------------------------------------------ | ------------------------------------------------------------ | -------------------- |
| plug_status       | returns negative status code responses, gets called as default error handler. | HTTP status code                                             | NULL                 |
| plug_redirect     | create a redirection response                                | redirect status code                                         | URL, redirect target |
| plug_rewrite_path | replace the path (and optionaly method) of the request       | 0 / new http_method                                          | target path          |
| plug_zip_fs       | serve files from a zipfile                                   | 0:continue<br />404:  use the error handler to create a 404 response. | zipfile reference    |
| plug_json         | simple JSON API callbacks <br />(uses coreJSON by default)   | requst body size limit                                       | json_callback_fn     |

The plugin api is defined in `plug.h` (see also below).



## Rule Based Request Processing

The server handles request by matching rules against the URL from an request.
Rules are passed as an array of  `const plug_rule_t` structs.

```c
// add a zipfile to the firmware image 
PLUG_ZIP_FS_FILE(webroot, "web_root.zip");

// request handling rules:
static const plug_rule_t s_webservice_rules[] = {
       // redirect references to index.html to '/'
    PLUG_RULE("/index.html?$", HTTP_GET, plug_redirect, 301, "/"),

    // rewrite '/' so it will be served from index.html
    PLUG_RULE("/$", HTTP_GET, plug_rewrite_path, 0, "/index.html"),
    
    // api calls are nested under /api
    PLUG_PATTERN("/api", HTTP_ANY),
    
    // status requests go to a json request handler
    // due to the initial '.' this matches /api/status and would be skiped if the 
    // previous rule fails. more ./ rules can follow to match against 
    PLUG_RULE("./status", HTTP_GET, plug_json, 0, json_status_handler),
    
    // anything that starts with a / can be served from the web_root.zip included above
    // return 404 if not found, (flags of '0' would continue rule processing)
    // this is a builtin behavior of plug_zip_fs (calls the error handler, plug_status by default).
    PLUG_RULE("/", HTTP_GET, plug_zip_fs, 404, &webroot),
    
    // unsupported method for everything else
    PLUG_RULE("", HTTP_ANY, plug_status, 405, NULL),
}
```



`PLUG_RULE(pattern, methods, plug, int_flags, options_pointer)` constructs a plug_rule_t initializer to call the plugin if methods and the pattern match.

int_flags and the options_pointer are defined by the plug that is been called.

`PLUG_PATTERN(pattern,methods)` constructs a plug_rule_t that only matches a pattern.




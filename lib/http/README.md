# HTTP server

* Support **GET**, **POST** url-encoded-data

## Basic scripting

### Markup

Substitution of variables can be achieved by placing them into html comment. `<!-- [var] -->`

* Only uppercase, lowercase and numbers are allowed. (c: isalnum)
* Only one variable by tag
* only `html` and `json` files are parsed

### Implementation

The user has to declare it's own `ts_web_content_handlers` structure.

* `mParsingStart` will be called on `get` or `post` requests as soon as parsing starts. It can be used to implement mutex protection.
* `mParsingDone` will be called after `get` or `post` requests. It can be used to implement mutex protection.
* `mUserData` is passed to all functions to be able to store implementation dependant data.
* `mHandler` is a set of user-handlers to be called if data is requested from variable substitution or is received via `get` and `post`.
* `mHandlerCount` the number of registered handlers. It must match the array size of `mHandler`.

```C
/*! Web parser structure */
typedef struct {

    /*! Number of web content handlers*/
    size_t                      mHandlerCount;

    /*! Parsing start handler */
    void                     (* mParsingStart)(void * inUserData);

    /*! Parsing done handler */
    void                     (* mParsingDone)(void * inUserData);

    /*! User data */
    void                      * mUserData;

    /*! Web content handlers */
    ts_web_content_handler      mHandler[];

} ts_web_content_handlers;
```


## Supported content types

| File Extension | MIME type                |
| -------------- | ------------------------ |
| html           | text/html                |
| css            | text/css                 |
| js             | text/javascript          |
| png            | image/png                |
| jpg            | image/jpeg               |
| gif            | image/gif                |
| txt            | text/plain               |
| bin            | application/octet-stream |
| json           | application/json         |

other content types can be enabled in `web_content_handler.c`.

## Web content

To test the web content, change to the `web` directory and run:

```
python -m http.server 8000
```

Use a web-browser and navigate to http://localhost:8000

## Convert web content to c file

Go to the `tools` folder and run:

```
gcc -o txt_to_c txt_to_c.c
```

Go to the `web` directory and run:

```
../tools/txt_to_c ../src/web_content.c *
```

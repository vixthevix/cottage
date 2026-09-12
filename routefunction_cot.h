/*
Struct definitions for HTTP requests and routes.

Code is part of the cottage framework (https://github.com/vixthevix/cottage)
*/

#ifndef ROUTEFUNCTION_COT
#define ROUTEFUNCTION_COT

#include "sitevar_cot.h"
#include "stringmap_cot.h"
#include "dependencies_cot.h"
#include "init_cot.h"
#include "error_cot.h"
#include "datavector_cot.h"

/*
Enum for each valid HTTP request type.
*/
typedef enum HttpRequest_Code {
    UNKNOWN = -1,
    GET,
    PUT,
    POST,
    DELETE,
    PATCH,
    HEAD,
    OPTIONS,
    TRACE,
    CONNECT,
} HttpRequest_Code;

/*
Struct that encapsulates a HTTP request.
@param type -> HTTP request type.
@param target -> resource the request is acting upon.
@param version -> HTTP version number of the request.
@param options -> Labelled information about the request, in hashmap form.
@param payload -> Extra data attached to the end of the request.
*/
typedef struct HttpRequest {
    HttpRequest_Code type;
    char* target;
    float version;
    stringMap* options;
    char* payload;
} HttpRequest;


/*
Enum for each valid HTTP response type.
Credits to https://github.com/j-ulrich/http-status-codes-cpp/blob/main/HttpStatusCodes_C.h
*/
typedef enum HttpResponse_Code
{
	HttpStatus_Invalid = -1, //!< An invalid status code.

	/*####### 1xx - Informational #######*/
	/* Indicates an interim response for communicating connection status
	 * or request progress prior to completing the requested action and
	 * sending a final response.
	 */
	HttpStatus_Continue           , /*!< Indicates that the initial part of a request has been received and has not yet been rejected by the server. */
	HttpStatus_SwitchingProtocols , /*!< Indicates that the server understands and is willing to comply with the client's request, via the Upgrade header field, for a change in the application protocol being used on this connection. */
	HttpStatus_Processing         , /*!< Is an interim response used to inform the client that the server has accepted the complete request, but has not yet completed it. */
	HttpStatus_EarlyHints         , /*!< Indicates to the client that the server is likely to send a final response with the header fields included in the informational response. */

	/*####### 2xx - Successful #######*/
	/* Indicates that the client's request was successfully received,
	 * understood, and accepted.
	 */
	HttpStatus_OK                          , /*!< Indicates that the request has succeeded. */
	HttpStatus_Created                     , /*!< Indicates that the request has been fulfilled and has resulted in one or more new resources being created. */
	HttpStatus_Accepted                    , /*!< Indicates that the request has been accepted for processing, but the processing has not been completed. */
	HttpStatus_NonAuthoritativeInformation , /*!< Indicates that the request was successful but the enclosed payload has been modified from that of the origin server's 200 (OK) response by a transforming proxy. */
	HttpStatus_NoContent                   , /*!< Indicates that the server has successfully fulfilled the request and that there is no additional content to send in the response payload body. */
	HttpStatus_ResetContent                , /*!< Indicates that the server has fulfilled the request and desires that the user agent reset the \"document view\", which caused the request to be sent, to its original state as received from the origin server. */
	HttpStatus_PartialContent              , /*!< Indicates that the server is successfully fulfilling a range request for the target resource by transferring one or more parts of the selected representation that correspond to the satisfiable ranges found in the requests's Range header field. */
	HttpStatus_MultiStatus                 , /*!< Provides status for multiple independent operations. */
	HttpStatus_AlreadyReported             , /*!< Used inside a DAV:propstat response element to avoid enumerating the internal members of multiple bindings to the same collection repeatedly. [RFC 5842] */
	HttpStatus_IMUsed                      , /*!< The server has fulfilled a GET request for the resource, and the response is a representation of the result of one or more instance-manipulations applied to the current instance. */

	/*####### 3xx - Redirection #######*/
	/* Indicates that further action needs to be taken by the user agent
	 * in order to fulfill the request.
	 */
	HttpStatus_MultipleChoices   , /*!< Indicates that the target resource has more than one representation, each with its own more specific identifier, and information about the alternatives is being provided so that the user (or user agent) can select a preferred representation by redirecting its request to one or more of those identifiers. */
	HttpStatus_MovedPermanently  , /*!< Indicates that the target resource has been assigned a new permanent URI and any future references to this resource ought to use one of the enclosed URIs. */
	HttpStatus_Found             , /*!< Indicates that the target resource resides temporarily under a different URI. */
	HttpStatus_SeeOther          , /*!< Indicates that the server is redirecting the user agent to a different resource, as indicated by a URI in the Location header field, that is intended to provide an indirect response to the original request. */
	HttpStatus_NotModified       , /*!< Indicates that a conditional GET request has been received and would have resulted in a 200 (OK) response if it were not for the fact that the condition has evaluated to false. */
	HttpStatus_UseProxy          , /*!< \deprecated \parblock Due to security concerns regarding in-band configuration of a proxy. \endparblock
	                                    The requested resource MUST be accessed through the proxy given by the Location field. */
	HttpStatus_TemporaryRedirect , /*!< Indicates that the target resource resides temporarily under a different URI and the user agent MUST NOT change the request method if it performs an automatic redirection to that URI. */
	HttpStatus_PermanentRedirect , /*!< The target resource has been assigned a new permanent URI and any future references to this resource ought to use one of the enclosed URIs. [...] This status code is similar to 301 Moved Permanently (Section 7.3.2 of rfc7231), except that it does not allow rewriting the request method from POST to GET. */

	/*####### 4xx - Client Error #######*/
	/* Indicates that the client seems to have erred.
	 */
	HttpStatus_BadRequest                  , /*!< Indicates that the server cannot or will not process the request because the received syntax is invalid, nonsensical, or exceeds some limitation on what the server is willing to process. */
	HttpStatus_Unauthorized                , /*!< Indicates that the request has not been applied because it lacks valid authentication credentials for the target resource. */
	HttpStatus_PaymentRequired             , /*!< *Reserved* */
	HttpStatus_Forbidden                   , /*!< Indicates that the server understood the request but refuses to authorize it. */
	HttpStatus_NotFound                    , /*!< Indicates that the origin server did not find a current representation for the target resource or is not willing to disclose that one exists. */
	HttpStatus_MethodNotAllowed            , /*!< Indicates that the method specified in the request-line is known by the origin server but not supported by the target resource. */
	HttpStatus_NotAcceptable               , /*!< Indicates that the target resource does not have a current representation that would be acceptable to the user agent, according to the proactive negotiation header fields received in the request, and the server is unwilling to supply a default representation. */
	HttpStatus_ProxyAuthenticationRequired , /*!< Is similar to 401 (Unauthorized), but indicates that the client needs to authenticate itself in order to use a proxy. */
	HttpStatus_RequestTimeout              , /*!< Indicates that the server did not receive a complete request message within the time that it was prepared to wait. */
	HttpStatus_Conflict                    , /*!< Indicates that the request could not be completed due to a conflict with the current state of the resource. */
	HttpStatus_Gone                        , /*!< Indicates that access to the target resource is no longer available at the origin server and that this condition is likely to be permanent. */
	HttpStatus_LengthRequired              , /*!< Indicates that the server refuses to accept the request without a defined Content-Length. */
	HttpStatus_PreconditionFailed          , /*!< Indicates that one or more preconditions given in the request header fields evaluated to false when tested on the server. */
	HttpStatus_ContentTooLarge             , /*!< Indicates that the server is refusing to process a request because the request payload is larger than the server is willing or able to process. */
	HttpStatus_PayloadTooLarge             , /*!< Alias for HttpStatus_ContentTooLarge for backward compatibility. */
	HttpStatus_URITooLong                  , /*!< Indicates that the server is refusing to service the request because the request-target is longer than the server is willing to interpret. */
	HttpStatus_UnsupportedMediaType        , /*!< Indicates that the origin server is refusing to service the request because the payload is in a format not supported by the target resource for this method. */
	HttpStatus_RangeNotSatisfiable         , /*!< Indicates that none of the ranges in the request's Range header field overlap the current extent of the selected resource or that the set of ranges requested has been rejected due to invalid ranges or an excessive request of small or overlapping ranges. */
	HttpStatus_ExpectationFailed           , /*!< Indicates that the expectation given in the request's Expect header field could not be met by at least one of the inbound servers. */
	HttpStatus_ImATeapot                   , /*!< Any attempt to brew coffee with a teapot should result in the error code 418 I'm a teapot. */
	HttpStatus_MisdirectedRequest          , /*!< Indicates that the request was directed at a server that is unable or unwilling to produce an authoritative response for the target URI. */
	HttpStatus_UnprocessableContent        , /*!< Means the server understands the content type of the request entity (hence a 415(Unsupported Media Type) status code is inappropriate), and the syntax of the request entity is correct (thus a 400 (Bad Request) status code is inappropriate) but was unable to process the contained instructions. */
	HttpStatus_UnprocessableEntity         , /*!< Alias for HttpStatus_UnprocessableContent for backward compatibility. */
	HttpStatus_Locked                      , /*!< Means the source or destination resource of a method is locked. */
	HttpStatus_FailedDependency            , /*!< Means that the method could not be performed on the resource because the requested action depended on another action and that action failed. */
	HttpStatus_TooEarly                    , /*!< Indicates that the server is unwilling to risk processing a request that might be replayed. */
	HttpStatus_UpgradeRequired             , /*!< Indicates that the server refuses to perform the request using the current protocol but might be willing to do so after the client upgrades to a different protocol. */
	HttpStatus_PreconditionRequired        , /*!< Indicates that the origin server requires the request to be conditional. */
	HttpStatus_TooManyRequests             , /*!< Indicates that the user has sent too many requests in a given amount of time (\"rate limiting\"). */
	HttpStatus_RequestHeaderFieldsTooLarge , /*!< Indicates that the server is unwilling to process the request because its header fields are too large. */
	HttpStatus_UnavailableForLegalReasons  , /*!< This status code indicates that the server is denying access to the resource in response to a legal demand. */

	/*####### 5xx - Server Error #######*/
	/* Indicates that the server is aware that it has erred
	 * or is incapable of performing the requested method.
	 */
	HttpStatus_InternalServerError           , /*!< Indicates that the server encountered an unexpected condition that prevented it from fulfilling the request. */
	HttpStatus_NotImplemented                , /*!< Indicates that the server does not support the functionality required to fulfill the request. */
	HttpStatus_BadGateway                    , /*!< Indicates that the server, while acting as a gateway or proxy, received an invalid response from an inbound server it accessed while attempting to fulfill the request. */
	HttpStatus_ServiceUnavailable            , /*!< Indicates that the server is currently unable to handle the request due to a temporary overload or scheduled maintenance, which will likely be alleviated after some delay. */
	HttpStatus_GatewayTimeout                , /*!< Indicates that the server, while acting as a gateway or proxy, did not receive a timely response from an upstream server it needed to access in order to complete the request. */
	HttpStatus_HTTPVersionNotSupported       , /*!< Indicates that the server does not support, or refuses to support, the protocol version that was used in the request message. */
	HttpStatus_VariantAlsoNegotiates         , /*!< Indicates that the server has an internal configuration error: the chosen variant resource is configured to engage in transparent content negotiation itself, and is therefore not a proper end point in the negotiation process. */
	HttpStatus_InsufficientStorage           , /*!< Means the method could not be performed on the resource because the server is unable to store the representation needed to successfully complete the request. */
	HttpStatus_LoopDetected                  , /*!< Indicates that the server terminated an operation because it encountered an infinite loop while processing a request with "Depth: infinity". [RFC 5842] */
	HttpStatus_NotExtended                   , /*!< \deprecated \parblock Obsoleted as the experiment has ended and there is no evidence of widespread use. \endparblock
	                                                     The policy for accessing the resource has not been met in the request. [RFC 2774] */
	HttpStatus_NetworkAuthenticationRequired , /*!< Indicates that the client needs to authenticate to gain network access. */

	HttpStatus_xxx_max
} HttpResponse_Code;

/*
String array to convert HttpResponse_Code enum to string.
*/
const char* HttpResponse_Code_List[] = {
    /* 1xx - Informational */
    "100 Continue",
    "101 Switching Protocols",
    "102 Processing",
    "103 Early Hints",

    /* 2xx - Successful */
    "200 OK",
    "201 Created",
    "202 Accepted",
    "203 Non-Authoritative Information",
    "204 No Content",
    "205 Reset Content",
    "206 Partial Content",
    "207 Multi-Status",
    "208 Already Reported",
    "226 IM Used",

    /* 3xx - Redirection */
    "300 Multiple Choices",
    "301 Moved Permanently",
    "302 Found",
    "303 See Other",
    "304 Not Modified",
    "305 Use Proxy",
    "307 Temporary Redirect",
    "308 Permanent Redirect",

    /* 4xx - Client Error */
    "400 Bad Request",
    "401 Unauthorized",
    "402 Payment Required",
    "403 Forbidden",
    "404 Not Found",
    "405 Method Not Allowed",
    "406 Not Acceptable",
    "407 Proxy Authentication Required",
    "408 Request Timeout",
    "409 Conflict",
    "410 Gone",
    "411 Length Required",
    "412 Precondition Failed",
    "413 Content Too Large",
    "413 Payload Too Large", /* Alias for Content Too Large */
    "414 URI Too Long",
    "415 Unsupported Media Type",
    "416 Range Not Satisfiable",
    "417 Expectation Failed",
    "418 I'm a teapot",
    "421 Misdirected Request",
    "422 Unprocessable Content",
    "422 Unprocessable Entity", /* Alias for Unprocessable Content */
    "423 Locked",
    "424 Failed Dependency",
    "425 Too Early",
    "426 Upgrade Required",
    "428 Precondition Required",
    "429 Too Many Requests",
    "431 Request Header Fields Too Large",
    "451 Unavailable For Legal Reasons",

    /* 5xx - Server Error */
    "500 Internal Server Error",
    "501 Not Implemented",
    "502 Bad Gateway",
    "503 Service Unavailable",
    "504 Gateway Timeout",
    "505 HTTP Version Not Supported",
    "506 Variant Also Negotiates",
    "507 Insufficient Storage",
    "508 Loop Detected",
    "510 Not Extended",
    "511 Network Authentication Required"
};

/*
Struct that encapsulates a HTTP responset.
@param version -> HTTP version number of the request.
@param type -> HTTP response code.
@param options -> Labelled information about the request, in hashmap form.
@param payload -> Extra data attached to the end of the request.
*/
typedef struct HttpResponse {
    float version;
    HttpResponse_Code type;
    stringMap* options;
    char* payload;
    size_t payload_size;
} HttpResponse;

/*
Function prototype for a route function, which is code that executes
depending on the request type acted upon a defined route.
@arg request -> the HTTP request to get information from.
@arg clientfd -> the client that sent the HTTP request.
@arg extraData -> external siteVars to be used.
@return status of route success.
*/
typedef bool (*RouteFunction)(HttpRequest request, int clientfd, siteVar* extraData);

/*
Macro for reducing boilerplate when writing a new RouteFunction definition.
*/
#define NewRouteFunction(functionName) bool functionName(HttpRequest request, int clientfd, siteVar* extraData)

/*
Struct that holds all of the possible route functions for a given route.
Should hold a RouteFunction for each individual HTTP request type.
(cottage v1 has only the main GET, POST, PUT and DELETE types. More to be added in the future.)
*/
typedef struct RouteEntry {
    RouteFunction routeGet;
    RouteFunction routePost;
    RouteFunction routePut;
    RouteFunction routeDelete;
} RouteEntry;

// Function prototypes
void debugHttpRequest(HttpRequest request);
HttpRequest_Code StrToHttpRequest_Code(char* data);
float StrToHttpVersion(char* data);
char* HttpRequest_CodeToStr(HttpRequest_Code type);
char* HttpVersionToStr(float version);
void HttpRequestFree(HttpRequest request);
bool HttpRequestValid(HttpRequest request);
cotResult HttpResponseInit(HttpResponse* input, float version, HttpResponse_Code type);
void HttpResponseFree(HttpResponse response);
bool HttpResponseAddOption(HttpResponse* response, const char* key, const char* value);
bool HttpResponseAddPayload(HttpResponse* response, char* payload, size_t size);
bool sendCustom(HttpResponse response, int client);
char* buildHttpResponse(HttpResponse response);
size_t HttpResponseTotalSize(HttpResponse response);

#if defined(COTTAGE_START)

/*
Debug display for a HTTP request.
@arg request -> request to display.
*/
void debugHttpRequest(HttpRequest request) {
    cottageCheck();
    fprintf(stderr,
        "REQUEST DEBUG\n"
        "TARGET:%s\n"
        "PAYLOAD:%s\n"
        "VERSION:%f\n"
        "TYPE:%i\n",
        request.target, request.payload, request.version, request.type
    );

}

/*
Converts HTTP request type string into enum value.
@arg data -> HTTP request type in string form.
@return HTTP request type in enum form.
*/
HttpRequest_Code StrToHttpRequest_Code(char* data) {
    cottageCheck(UNKNOWN);
    if (!data) return UNKNOWN;

    if (!strcmp(data, "GET")) return GET;
    if (!strcmp(data, "PUT")) return PUT;
    if (!strcmp(data, "POST")) return POST;
    if (!strcmp(data, "DELETE")) return DELETE;
    if (!strcmp(data, "PATCH")) return PATCH;
    if (!strcmp(data, "HEAD")) return HEAD;
    if (!strcmp(data, "OPTIONS")) return OPTIONS;
    if (!strcmp(data, "TRACE")) return TRACE;
    if (!strcmp(data, "CONNECT")) return CONNECT;

    return UNKNOWN;
}

/*
Converts HTTP version string into float value.
@arg data -> HTTP version in string form.
@return HTTP version in float form.
*/
float StrToHttpVersion(char* data) {
    cottageCheck(0);
    if (!data) return UNKNOWN;

    if (!strcmp(data, "HTTP/0.9")) return 0.9;
    if (!strcmp(data, "HTTP/1.0")) return 1.0;
    if (!strcmp(data, "HTTP/1.1")) return 1.1;
    if (!strcmp(data, "HTTP/2")) return 2;
    if (!strcmp(data, "HTTP/3")) return 3;

    return -1;

}

/*
Converts HTTP request type enum into string equivalent.
@arg data -> HTTP request type in enum form.
@return HTTP request type in string form.
*/
char* HttpRequest_CodeToStr(HttpRequest_Code type) {
    cottageCheck(NULL);
    
    const int bufferMax = 20;
    char* buffer = calloc(bufferMax, sizeof(char));

    switch (type) {
        case GET:
            strncpy(buffer, "GET", bufferMax);
            break;
        case PUT:
            strncpy(buffer, "PUT", bufferMax);
            break;
        case POST:
            strncpy(buffer, "POST", bufferMax);
            break;
        case DELETE:
            strncpy(buffer, "DELETE", bufferMax);
            break;
        case PATCH:
            strncpy(buffer, "PATCH", bufferMax);
            break;
        case HEAD:
            strncpy(buffer, "HEAD", bufferMax);
            break;
        case OPTIONS:
            strncpy(buffer, "OPTIONS", bufferMax);
            break;
        case TRACE:
            strncpy(buffer, "TRACE", bufferMax);
            break;
        case CONNECT:
            strncpy(buffer, "CONNECT", bufferMax);
            break;
        default: {
            if (buffer) free(buffer);
            return NULL;
        }
    }

    buffer = realloc(buffer, strlen(buffer) + 1);

    return buffer;
}

/*
Converts HTTP version to string equivalent
@arg data -> HTTP version in float form.
@return HTTP version in string form.
*/
char* HttpVersionToStr(float version) {
    cottageCheck(NULL);
    
    const int bufferMax = 20;
    char* buffer = calloc(bufferMax, sizeof(char));

	if      (version == 0.9) strncpy(buffer, "HTTP/0.9", bufferMax);
	else if (version == 1.0) strncpy(buffer, "HTTP/1.0", bufferMax);
	else if (version == 1.1) strncpy(buffer, "HTTP/1.1", bufferMax);
	else if (version == 2.0) strncpy(buffer, "HTTP/2", bufferMax);
	else if (version == 3.0) strncpy(buffer, "HTTP/3", bufferMax);
	else {
		if (buffer) free(buffer);
		return NULL;
	}

    buffer = realloc(buffer, strlen(buffer) + 1);

    return buffer;
}

/*
Frees HttpRequest from memory.
@arg request -> target to free.
*/
void HttpRequestFree(HttpRequest request) {
    cottageCheck();
    if (request.target) free(request.target);
    if (request.options) strMapFree(request.options);
    if (request.payload) free(request.payload);
}

/*
Checks if a HttpRequest is valid for interpreting.
@arg request -> target to analyze.
@return if valid.
*/
bool HttpRequestValid(HttpRequest request) {
    cottageCheck(false);
    return (request.target && request.options && (request.type > UNKNOWN) && (request.version > -1));
}

/*
Creates an empty HttpResponse container.
@arg input -> stores container.
@arg version -> HTTP version number.
@arg type -> HTTP response code.
@return error status of creation.
*/
cotResult HttpResponseInit(HttpResponse* input, float version, HttpResponse_Code type) {
	HttpResponse response;
	if (
		version != 0.9 &&
		version != 1.0 &&
		version != 1.1 &&
		version != 2   &&
		version != 3
	) {
		return newResultError("HttpResponseInit: invalid version.");
	}

	if (type ==  HttpStatus_Invalid || type >= HttpStatus_xxx_max) {
		return newResultError("HttpResponseInit: invalid type.");
	}

	response.version = version;
	response.type = type;
	response.options = strMapInit();
	response.payload = NULL;

	*input = response;
	return newResultOK();
}

/*
Frees a HttpResponse object from memory.
@arg response -> target to free.
*/
void HttpResponseFree(HttpResponse response) {
	strMapFree(response.options);
	if (response.payload) free(response.payload);
}

/*
Wrapper to insert option into HttpResponse.
@arg response -> HttpResponse to edit.
@arg key -> key of option.
@arg value -> value of option.
@return status of insert.
*/
bool HttpResponseAddOption(HttpResponse* response, const char* key, const char* value) {
    return strMapInsert(&(response->options), key, value);
}

/*
Adds a copy of a payload into HttpResponse.
@arg response -> HttpResponse to edit.
@arg payload -> data to insert.
@arg size -> size of payload in bytes.
@return status of insert.
*/
bool HttpResponseAddPayload(HttpResponse* response, char* payload, size_t size) {
    if (!payload) {
        newResultError("HttpResponseAddPayload: payload invalid.");
        return false;
    }
    if (response->payload) free(response->payload);
    response->payload = (char*) calloc(size, sizeof(char));
    if (!response->payload) {
        newResultError("HttpResponseAddPayload: out of memory.");
        return false;
    }
    memcpy(response->payload, payload, size);
    response->payload_size = size;

    return true;
}

/*
Sends a custom HTTP response to a client.
@arg response -> HttpResponse container.
@arg client -> fd to send data to.
@return status of send.
*/
bool sendCustom(HttpResponse response, int client) {
    cottageCheck(false);
    char* buffer = buildHttpResponse(response); 
    if (!buffer) {
        newResultError("sendCustom: buffer is invalid.");
        return false;
    }

    const size_t size = strlen(buffer);
    int bytes = send(client, buffer, size, 0);
    if (bytes <= 0) {
        newResultError("sendCustom: could not send bytes to client.");
        return false;
    }

    return true;
}

/*
Reads a HttpResponse object, and converts into HTTP response string.
@arg response -> HttpResponse data.
@return response in string form.
*/
char* buildHttpResponse(HttpResponse response) {
    //We'll need dataVector.
    dataVector vector = dataVectorInit(256);
    if (!vector.data) return NULL;

    //We will need some data
    char* version = HttpVersionToStr(response.version);
    if (!version) return NULL;

    //We need to get the response type in string form.
    //DO THIS NEXT MAKE A HUGE ARRAY OF STRINGS AND
    //CHANGE RESPONSE ENUM FOR INDEXING.
    if (response.type ==  HttpStatus_Invalid || response.type >= HttpStatus_xxx_max) {
        if (version) free(version);
        return NULL;
    }
    char* type = HttpResponse_Code_List[response.type];

    char header_buffer[256] = {0};
    snprintf(header_buffer, 256, "%s %s\r\n", version, type);
    dataVectorPushString(&vector, header_buffer);

    //now we have to look through our options and sprintf them.
    if (!response.options || !response.options->items) goto payload_jump;
    const unsigned int option_max = response.options->capacity;
    for (unsigned int i = 0; i < option_max; i++) {
        if (response.options->items[i] != NULL) {
            //Valid string pair
            stringPair* pair = response.options->items[i];
            //Validate strings
            if (!pair->key || !pair->value) continue;
            
            char option_buffer[512] = {0};
            snprintf(option_buffer, 512, "%s: %s\r\n", pair->key, pair->value);
            dataVectorPushString(&vector, option_buffer);
        }
    }

    payload_jump:
    dataVectorPushString(&vector, "\r\n");
    if (!response.payload) goto end_jump;
    //Just push the payload directly
    dataVectorPushBytes(&vector, response.payload, response.payload_size);

    end_jump:
    vector.data[vector.index] = 0; //null terminate it
    return vector.data;
}

/*
Counts up total size of HttpResponse raw data.
@arg response -> HttpResponse to analyze.
@return total size of raw data.
*/
size_t HttpResponseTotalSize(HttpResponse response) {

    size_t count = 0;

    //We will need some data
    char* version = HttpVersionToStr(response.version);
    if (!version) return 0;

    //We need to get the response type in string form.
    //DO THIS NEXT MAKE A HUGE ARRAY OF STRINGS AND
    //CHANGE RESPONSE ENUM FOR INDEXING.
    if (response.type ==  HttpStatus_Invalid || response.type >= HttpStatus_xxx_max) {
        if (version) free(version);
        return 0;
    }
    char* type = HttpResponse_Code_List[response.type];

    char header_buffer[256] = {0};
    snprintf(header_buffer, 256, "%s %s\r\n", version, type);
    count += strlen(header_buffer);

    //now we have to look through our options and sprintf them.
    if (!response.options || !response.options->items) goto payload_jump;
    const unsigned int option_max = response.options->capacity;
    for (unsigned int i = 0; i < option_max; i++) {
        if (response.options->items[i] != NULL) {
            //Valid string pair
            stringPair* pair = response.options->items[i];
            //Validate strings
            if (!pair->key || !pair->value) continue;
            
            char option_buffer[512] = {0};
            snprintf(option_buffer, 512, "%s: %s\r\n", pair->key, pair->value);
            count += strlen(option_buffer);
        }
    }

    payload_jump:
    count += strlen("\r\n");
    if (!response.payload) goto end_jump;
    //Just push the payload directly
    count += response.payload_size;

    end_jump:
    return count;
}

#endif
#endif
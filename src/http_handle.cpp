/*
 * http_handle.cpp
 * CraftOS-PC 2
 * 
 * This file implements the methods for HTTP handles.
 * 
 * This code is licensed under the MIT license.
 * Copyright (c) 2019 JackMacWindows.
 */

#include "http_handle.hpp"
#include "lib.hpp"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/HTTPServerRequest.h>

using namespace Poco::Net;

typedef struct {
    bool closed;
    char * url;
    HTTPClientSession * session;
    HTTPResponse * handle;
    std::istream& stream;
} http_handle_t;

struct http_res {
    std::string body;
    HTTPServerResponse * res;
};

int http_handle_free(lua_State *L) {
    delete (http_handle_t*)lua_touserdata(L, lua_upvalueindex(1));
    return 0;
}

int http_handle_close(lua_State *L) {
    http_handle_t* handle = (http_handle_t*)lua_touserdata(L, lua_upvalueindex(1));
    if (handle->closed) return 0;
    handle->closed = true;
    delete handle->handle;
    delete handle->session;
    free(handle->url);
    return 0;
}

extern char checkChar(char c);

int http_handle_readAll(lua_State *L) {
    http_handle_t * handle = (http_handle_t*)lua_touserdata(L, lua_upvalueindex(1));
    if (handle->closed || !handle->stream.good()) return 0;
    std::string ret;
    char buffer[4096];
    while (handle->stream.read(buffer, sizeof(buffer)))
        ret.append(buffer, sizeof(buffer));
    ret.append(buffer, handle->stream.gcount());
    if (!lua_toboolean(L, lua_upvalueindex(2))) ret.erase(std::remove(ret.begin(), ret.end(), '\r'), ret.end());
    lua_pushlstring(L, ret.c_str(), ret.length());
    return 1;
}

int http_handle_readLine(lua_State *L) {
    http_handle_t * handle = (http_handle_t*)lua_touserdata(L, lua_upvalueindex(1));
    if (handle->closed || !handle->stream.good()) return 0;
    std::string line;
    std::getline(handle->stream, line, '\n');
    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
    lua_pushstring(L, line.c_str());
    return 1;
}

int http_handle_readChar(lua_State *L) {
    http_handle_t * handle = (http_handle_t*)lua_touserdata(L, lua_upvalueindex(1));
    if (handle->closed || !handle->stream.good()) return 0;
    char retval[2];
    retval[0] = checkChar(handle->stream.get());
    lua_pushstring(L, retval);
    return 1;
}

int http_handle_readByte(lua_State *L) {
    http_handle_t * handle = (http_handle_t*)lua_touserdata(L, lua_upvalueindex(1));
    if (handle->closed || !handle->stream.good()) return 0;
    if (!lua_isnumber(L, 1)) {
        lua_pushinteger(L, handle->stream.get());
    } else {
        int c = lua_tointeger(L, 1);
        char * retval = new char[c+1];
        handle->stream.read(retval, c);
        lua_pushstring(L, retval);
    }
    return 1;
}

int http_handle_getResponseCode(lua_State *L) {
    http_handle_t * handle = (http_handle_t*)lua_touserdata(L, lua_upvalueindex(1));
    if (handle->closed) return 0;
    lua_pushinteger(L, handle->handle->getStatus());
    return 1;
}

int http_handle_getResponseHeaders(lua_State *L) {
    http_handle_t * handle = (http_handle_t*)lua_touserdata(L, lua_upvalueindex(1));
    if (handle->closed) return 0;
    lua_newtable(L);
    for (auto it = handle->handle->begin(); it != handle->handle->end(); it++) {
        lua_pushstring(L, it->first.c_str());
        lua_pushstring(L, it->second.c_str());
        lua_settable(L, -3);
    }
    return 1;
}

int req_read(lua_State *L) {
    HTTPServerRequest * req = (HTTPServerRequest*)lua_touserdata(L, lua_upvalueindex(1));
    if (*(bool*)lua_touserdata(L, lua_upvalueindex(2)) || !req->stream().good()) return 0;
    char tmp[2];
    tmp[0] = req->stream().get();
    lua_pushstring(L, tmp);
    return 1;
}

int req_readLine(lua_State *L) {
    HTTPServerRequest * req = (HTTPServerRequest*)lua_touserdata(L, lua_upvalueindex(1));
    if (*(bool*)lua_touserdata(L, lua_upvalueindex(2)) || !req->stream().good()) return 0;
    std::string line;
    std::getline(req->stream(), line);
    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
    lua_pushstring(L, line.c_str());
    return 1;
}

int req_readAll(lua_State *L) {
    HTTPServerRequest * req = (HTTPServerRequest*)lua_touserdata(L, lua_upvalueindex(1));
    if (*(bool*)lua_touserdata(L, lua_upvalueindex(2)) || !req->stream().good()) return 0;
    std::string ret;
    char buffer[4096];
    while (req->stream().read(buffer, sizeof(buffer)))
        ret.append(buffer, sizeof(buffer));
    ret.append(buffer, req->stream().gcount());
    ret.erase(std::remove(ret.begin(), ret.end(), '\r'), ret.end());
    lua_pushstring(L, ret.c_str());
    return 1;
}

int req_close(lua_State *L) {
    return 0;
}

int req_free(lua_State *L) {
    delete (bool*)lua_touserdata(L, lua_upvalueindex(1));
    return 0;
}

int req_getURL(lua_State *L) {
    HTTPServerRequest * req = (HTTPServerRequest*)lua_touserdata(L, lua_upvalueindex(1));
    if (*(bool*)lua_touserdata(L, lua_upvalueindex(2))) return 0;
    lua_pushstring(L, req->getURI().c_str());
    return 1;
}

int req_getMethod(lua_State *L) {
    HTTPServerRequest * req = (HTTPServerRequest*)lua_touserdata(L, lua_upvalueindex(1));
    if (*(bool*)lua_touserdata(L, lua_upvalueindex(2))) return 0;
    lua_pushstring(L, req->getMethod().c_str());
    return 1;
}

int req_getRequestHeaders(lua_State *L) {
    HTTPServerRequest * req = (HTTPServerRequest*)lua_touserdata(L, lua_upvalueindex(1));
    if (*(bool*)lua_touserdata(L, lua_upvalueindex(2))) return 0;
    lua_newtable(L);
    for (auto h = req->begin(); h != req->end(); h++) {
        lua_pushstring(L, h->first.c_str());
        lua_pushstring(L, h->second.c_str());
        lua_settable(L, -3);
    }
    return 1;
}

int res_write(lua_State *L) {
    if (!lua_isstring(L, 1)) bad_argument(L, "string", 1);
    struct http_res * res = (struct http_res*)lua_touserdata(L, lua_upvalueindex(1));
    if (*(bool*)lua_touserdata(L, lua_upvalueindex(2)) || res->res->sent()) return 0;
    size_t len = 0;
    const char * buf = lua_tolstring(L, 1, &len);
    res->body += std::string(buf, len);
    return 0;
}

int res_writeLine(lua_State *L) {
    if (!lua_isstring(L, 1)) bad_argument(L, "string", 1);
    struct http_res * res = (struct http_res*)lua_touserdata(L, lua_upvalueindex(1));
    if (*(bool*)lua_touserdata(L, lua_upvalueindex(2)) || res->res->sent()) return 0;
    size_t len = 0;
    const char * buf = lua_tolstring(L, 1, &len);
    res->body += std::string(buf, len);
    res->body += "\n";
    return 0;
}

int res_close(lua_State *L) {
    struct http_res * res = (struct http_res*)lua_touserdata(L, lua_upvalueindex(1));
    if (*(bool*)lua_touserdata(L, lua_upvalueindex(2)) || res->res->sent()) return 0;
    std::string body((const std::string)res->body);
    try {
        res->res->setContentLength(body.size());
        res->res->send().write(body.c_str(), body.size());
    } catch (std::exception &e) {
        *(bool*)lua_touserdata(L, lua_upvalueindex(2)) = true;
        lua_pushfstring(L, "Could not send data: %s", e.what());
        lua_error(L);
    }
    *(bool*)lua_touserdata(L, lua_upvalueindex(2)) = true;
    return 0;
}

int res_setStatusCode(lua_State *L) {
    if (!lua_isnumber(L, 1)) bad_argument(L, "number", 1);
    struct http_res * res = (struct http_res*)lua_touserdata(L, lua_upvalueindex(1));
    if (*(bool*)lua_touserdata(L, lua_upvalueindex(2)) || res->res->sent()) return 0;
    res->res->setStatus((HTTPResponse::HTTPStatus)lua_tointeger(L, 1));
    return 0;
}

int res_setResponseHeader(lua_State *L) {
    if (!lua_isstring(L, 1)) bad_argument(L, "string", 1);
    if (!lua_isstring(L, 2)) bad_argument(L, "string", 2);
    struct http_res * res = (struct http_res*)lua_touserdata(L, lua_upvalueindex(1));
    if (*(bool*)lua_touserdata(L, lua_upvalueindex(2)) || res->res->sent()) return 0;
    res->res->set(lua_tostring(L, 1), lua_tostring(L, 2));
    return 0;
}
#include "JavaScriptObject.h"

JavaScriptObject::JavaScriptObject(napi_env env) {
    this->env = env;
    status = napi_create_object(env, &self);
    assert(status == napi_ok);
}

void JavaScriptObject::addDouble(char* fieldName, double fieldValue) {
    napi_value doubleNAPIValue;
    napi_status status;
    status = napi_create_double(env, fieldValue, &doubleNAPIValue);
    assert(status == napi_ok);

    status = napi_set_named_property(env, self, fieldName, doubleNAPIValue);
    assert(status == napi_ok);
}

void JavaScriptObject::addString(char* fieldName, char* fieldValue) {
    napi_value charNAPIValue;
    napi_status status;
    status = napi_create_string_utf8(env, fieldValue, strlen(fieldValue), &charNAPIValue);
    assert(status == napi_ok);

    status = napi_set_named_property(env, self, fieldName, charNAPIValue);
    assert(status == napi_ok);
}

void JavaScriptObject::addBool(char* fieldName, bool fieldValue) {
    napi_value boolNAPIValue;
    napi_status status;
    status = napi_get_boolean(env, fieldValue, &boolNAPIValue);
    assert(status == napi_ok);

    status = napi_set_named_property(env, self, fieldName, boolNAPIValue);
    assert(status == napi_ok);
}

napi_value JavaScriptObject::getSelf() {
    return this->self;
}

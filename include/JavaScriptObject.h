#ifndef JAVASCRIPTOBJECT_H
#define JAVASCRIPTOBJECT_H

#include "node_api.h"
#include "assert.h"
#include <stdlib.h>
#include <string>
using namespace std;

class JavaScriptObject 
{
    public:
        napi_value self;
        JavaScriptObject(napi_env env);
        void addDouble(char* fieldName, double fieldValue);
        void addString(char* fieldName, char* fieldValue);
        void addBool(char* fieldName, bool fieldValue);
        napi_value getSelf();
        
    private:
        napi_env env;
        napi_status status;
};

#endif // JAVASCRIPTOBJECT_H

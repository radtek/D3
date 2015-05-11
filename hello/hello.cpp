#include "types.h"

using namespace v8;
using namespace D3;

void Method(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
	Settings&	d3Settings = Settings::Singleton();
	// Cannot resolve symbol "Singleton"
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, d3Settings.AdminPWD().c_str()));
}

void init(Handle<v8::Object> exports) {
  NODE_SET_METHOD(exports, "hello", Method);
}

NODE_MODULE(addon, init)
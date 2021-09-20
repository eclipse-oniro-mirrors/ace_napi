/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FOUNDATION_ACE_NAPI_NATIVE_ENGINE_NATIVE_ENGINE_H
#define FOUNDATION_ACE_NAPI_NATIVE_ENGINE_NATIVE_ENGINE_H

#include <string>
#include <vector>
#include "native_engine/native_async_work.h"
#include "native_engine/native_deferred.h"
#include "native_engine/native_reference.h"
#include "native_engine/native_value.h"
#include "native_property.h"

#include "module_manager/native_module_manager.h"
#include "scope_manager/native_scope_manager.h"

typedef struct uv_loop_s uv_loop_t;

struct NativeErrorExtendedInfo {
    const char* message = nullptr;
    void* reserved = nullptr;
    uint32_t engineErrorCode = 0;
    int errorCode = 0;
};

struct ExceptionInfo {
    const char* message_ = nullptr;
    int32_t lineno_ = 0;
    int32_t colno_ = 0;

    ~ExceptionInfo()
    {
        if (message_ != nullptr) {
            delete[] message_;
        }
    }
};

enum LoopMode {
    LOOP_DEFAULT, LOOP_ONCE, LOOP_NOWAIT
};

using PostTask = std::function<void(bool needSync)>;

class NativeEngine {
public:
    NativeEngine(void *jsEngine);
    virtual ~NativeEngine();

    virtual NativeScopeManager* GetScopeManager();
    virtual NativeModuleManager* GetModuleManager();
    virtual uv_loop_t* GetUVLoop() const;

    virtual void Loop(LoopMode mode, bool needSync = false);
    virtual void SetPostTask(PostTask postTask);
    virtual void TriggerPostTask();
    virtual void CheckUVLoop();
    virtual void CancelCheckUVLoop();
    virtual void* GetJsEngine() const;

    virtual NativeValue* GetGlobal() = 0;

    virtual NativeValue* CreateNull() = 0;
    virtual NativeValue* CreateUndefined() = 0;
    virtual NativeValue* CreateBoolean(bool value) = 0;
    virtual NativeValue* CreateNumber(int32_t value) = 0;
    virtual NativeValue* CreateNumber(uint32_t value) = 0;
    virtual NativeValue* CreateNumber(int64_t value) = 0;
    virtual NativeValue* CreateNumber(double value) = 0;
    virtual NativeValue* CreateString(const char* value, size_t length) = 0;

    virtual NativeValue* CreateSymbol(NativeValue* value) = 0;
    virtual NativeValue* CreateExternal(void* value, NativeFinalize callback, void* hint) = 0;

    virtual NativeValue* CreateObject() = 0;
    virtual NativeValue* CreateFunction(const char* name, size_t length, NativeCallback cb, void* value) = 0;
    virtual NativeValue* CreateArray(size_t length) = 0;

    virtual NativeValue* CreateArrayBuffer(void** value, size_t length) = 0;
    virtual NativeValue* CreateArrayBufferExternal(void* value, size_t length, NativeFinalize cb, void* hint) = 0;

    virtual NativeValue* CreateTypedArray(NativeTypedArrayType type,
                                          NativeValue* value,
                                          size_t length,
                                          size_t offset) = 0;
    virtual NativeValue* CreateDataView(NativeValue* value, size_t length, size_t offset) = 0;
    virtual NativeValue* CreatePromise(NativeDeferred** deferred) = 0;
    virtual NativeValue* CreateError(NativeValue* code, NativeValue* message) = 0;

    virtual NativeValue* CallFunction(NativeValue* thisVar,
                                      NativeValue* function,
                                      NativeValue* const* argv,
                                      size_t argc) = 0;
    virtual NativeValue* RunScript(NativeValue* script) = 0;
    virtual NativeValue* RunBufferScript(std::vector<uint8_t>& buffer) = 0;
    virtual NativeValue* DefineClass(const char* name,
                                     NativeCallback callback,
                                     void* data,
                                     const NativePropertyDescriptor* properties,
                                     size_t length) = 0;

    virtual NativeValue* CreateInstance(NativeValue* constructor, NativeValue* const* argv, size_t argc) = 0;

    virtual NativeAsyncWork* CreateAsyncWork(NativeValue* asyncResource,
                                             NativeValue* asyncResourceName,
                                             NativeAsyncExecuteCallback execute,
                                             NativeAsyncCompleteCallback complete,
                                             void* data);

    virtual NativeAsyncWork* CreateAsyncWork(NativeAsyncExecuteCallback execute,
                                             NativeAsyncCompleteCallback complete,
                                             void* data);

    virtual NativeReference* CreateReference(NativeValue* value, uint32_t initialRefcount) = 0;

    virtual bool Throw(NativeValue* error) = 0;
    virtual bool Throw(NativeErrorType type, const char* code, const char* message) = 0;

    virtual void* CreateRuntime() = 0;
    virtual NativeValue* Serialize(NativeEngine* context, NativeValue* value, NativeValue* transfer) = 0;
    virtual NativeValue* Deserialize(NativeEngine* context, NativeValue* recorder) = 0;
    virtual ExceptionInfo* GetExceptionForWorker() const = 0;
    virtual void DeleteSerializationData(NativeValue* value) const = 0;
    virtual NativeValue* LoadModule(NativeValue* str, const std::string& fileName) = 0;

    NativeErrorExtendedInfo* GetLastError();
    void SetLastError(int errorCode, uint32_t engineErrorCode = 0, void* engineReserved = nullptr);
    void ClearLastError();
    bool IsExceptionPending() const;
    NativeValue* GetAndClearLastException();
    void EncodeToUtf8(NativeValue* nativeValue, char* buffer, int32_t* written, size_t bufferSize, int32_t* nchars);
    NativeEngine(NativeEngine&) = delete;
    virtual NativeEngine& operator=(NativeEngine&) = delete;

    virtual NativeValue* ValueToNativeValue(JSValueWrapper& value) = 0;

    void MarkSubThread()
    {
        isMainThread_ = false;
    }

    bool IsMainThread() const
    {
        return isMainThread_;
    }

    void *jsEngine_;

protected:
    NativeModuleManager* moduleManager_ = nullptr;
    NativeScopeManager* scopeManager_ = nullptr;

    NativeErrorExtendedInfo lastError_;
    NativeValue* lastException_ = nullptr;

    uv_loop_t* loop_;

private:
    bool isMainThread_ { true };
    static void UVThreadRunner(void* nativeEngine);

    void Init();
    void PostLoopTask();

    bool checkUVLoop_ = false;
    PostTask postTask_ = nullptr;
    uv_thread_t uvThread_;
    uv_sem_t uvSem_;
    uv_async_t uvAsync_;
};

#endif /* FOUNDATION_ACE_NAPI_NATIVE_ENGINE_NATIVE_ENGINE_H */

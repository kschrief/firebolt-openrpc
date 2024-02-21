/*
 * Copyright 2023 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "Accessor/Accessor.h"

namespace FireboltSDK {

    class Async {
    private:
        Async();
    public:
        virtual ~Async();
        Async(const Async&) = delete;
        Async& operator= (const Async&) = delete;

    public:
        typedef std::function<Firebolt::Error(void*)> DispatchFunction;

        using CallbackMap = std::map<void*, DispatchFunction>;
        using MethodMap = std::map<string, CallbackMap>;

    private: 
        class Job : public WPEFramework::Core::IDispatch {
        protected:
            Job(Async& parent, const string& method, const DispatchFunction lambda, void* usercb)
                : _parent(parent)
                , _method(method)
                , _lambda(lambda)
                , _usercb(usercb)
            {
            }

       public:
            Job() = delete;
            Job(const Job&) = delete;
            Job& operator=(const Job&) = delete;

            ~Job() = default;

        public:
            static WPEFramework::Core::ProxyType<WPEFramework::Core::IDispatch> Create(Async& parent, const string& method, const DispatchFunction lambda, void* usercb);

            void Dispatch() override
            {
                if (_parent.IsActive(_method, _usercb) == true) {
                    _lambda(_usercb);
                }
            }

        private:
            Async& _parent;
            string _method;
            DispatchFunction _lambda;
            void* _usercb;
        };
    public:
        static Async& Instance();
        static void Dispose();
 
    public:
        template <typename RESPONSE, typename PARAMETERS, typename CALLBACK>
        Firebolt::Error Invoke(const string& method, const PARAMETERS& parameters, const CALLBACK& callback, void* usercb)
        {
            Firebolt::Error status = Firebolt::Error::General;
            Transport<WPEFramework::Core::JSON::IElement>* transport = Accessor::Instance().GetTransport();
            if (transport != nullptr) {
                std::function<void(void* usercb, void* response, Firebolt::Error status)> actualCallback = callback;
                DispatchFunction lambda = [actualCallback, transport, method, parameters](void* usercb) -> Firebolt::Error {
                    RESPONSE response;
                    Firebolt::Error status = transport->Invoke(method, parameters, response);
                    if (status == Firebolt::Error::None) {
                        WPEFramework::Core::ProxyType<RESPONSE>* jsonResponse = new WPEFramework::Core::ProxyType<RESPONSE>();
                        *jsonResponse = WPEFramework::Core::ProxyType<RESPONSE>::Create();
                        (*jsonResponse)->FromString(response);
                        actualCallback(usercb, jsonResponse, status);
                    }
                    return (status);
                };

                _adminLock.Lock();
                MethodMap::iterator index = _methodMap.find(method);
                if (index != _methodMap.end()) {
                    CallbackMap::iterator callbackIndex = index->second.find(usercb);
                    if (callbackIndex == index->second.end()) {
                        index->second.emplace(std::piecewise_construct, std::forward_as_tuple(usercb), std::forward_as_tuple(lambda));
                    }
                } else {

                    CallbackMap callbackMap;
                    callbackMap.emplace(std::piecewise_construct, std::forward_as_tuple(usercb), std::forward_as_tuple(lambda));
                    _methodMap.emplace(std::piecewise_construct, std::forward_as_tuple(method), std::forward_as_tuple(callbackMap));
                }
                _adminLock.Unlock();

                WPEFramework::Core::ProxyType<WPEFramework::Core::IDispatch> job = WPEFramework::Core::ProxyType<WPEFramework::Core::IDispatch>(WPEFramework::Core::ProxyType<Async::Job>::Create(*this, method, lambda, usercb));
                WPEFramework::Core::IWorkerPool::Instance().Submit(job);
            } else {
                FIREBOLT_LOG_ERROR(Logger::Category::OpenRPC, Logger::Module<Accessor>(), "Error in getting Transport err = %d", status);
            }

            return status;
        }

        Firebolt::Error Abort(const string& method, void* usercb)
        {
            Firebolt::Error status = Firebolt::Error::None;
            _adminLock.Lock();
            MethodMap::iterator index = _methodMap.find(method);
            if (index != _methodMap.end()) {
                CallbackMap::iterator callbackIndex = index->second.find(usercb);
                if (callbackIndex != index->second.end()) {
                    index->second.erase(callbackIndex);
                    if (index->second.size() == 0) {
                        _methodMap.erase(index);
                    }
                }
            }
            _adminLock.Unlock();

            return status;
        }


        bool IsActive(const string& method, void* usercb)
        {
            bool valid = false;
            MethodMap::iterator index = _methodMap.find(method);
            if (index != _methodMap.end()) {
                CallbackMap::iterator callbackIndex = index->second.find(usercb);
                if (callbackIndex != index->second.end()) {
                    valid = true;
                }
            }
            return valid;
        }

    private:
        MethodMap _methodMap;
        WPEFramework::Core::CriticalSection _adminLock;
        static Async* _singleton;
    };
}

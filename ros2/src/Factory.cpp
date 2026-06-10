/*
 * Copyright (C) 2018 Open Source Robotics Foundation
 * Copyright (C) 2020 - present Proyectos y Sistemas de Mantenimiento SL (eProsima).
 *
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
 *
 */

#include <is/sh/ros2/Factory.hpp>

#include "IsTypeName.hpp"

#include <is/utils/Log.hpp>

#include <unordered_map>
#include <utility>

namespace eprosima {
namespace is {
namespace sh {
namespace ros2 {

//==============================================================================
class Factory::Implementation
{
public:

    Implementation()
        : logger_("is::sh::ROS2::Factory")
    {
    }

    void register_type_factory(
            const std::string& type_name,
            RegisterTypeToFactory register_type_func)
    {
        _type_factories[type_name] = std::move(register_type_func);
    }

    xtypes::DynamicType create_type(
            const std::string& type_name)
    {
        auto it = _type_factories.find(type_name);
        if (it == _type_factories.end())
        {
            logger_ << utils::Logger::Level::ERROR
                    << "'create_type' could not find a factory type named '"
                    << type_name << "' to create!" << std::endl;

            return nullptr;
        }

        return it->second();
    }

    void register_serialiser_factory(
            const std::string& type_name,
            SerialiseToROS2Function register_func)
    {
      _serialiser_factories[type_name] = std::move(register_func);
    }

    SerialiseToROS2Function* get_serialise_function(
            const xtypes::DynamicType& topic_type)
    {
      auto it = _serialiser_factories.find(fastdds_type_to_is_name(std::string(topic_type->get_name())));
      if (it == _serialiser_factories.end())
      {
        logger_ << utils::Logger::Level::ERROR
                << "get_serialise_function' could not find a message type named '"
                << topic_type->get_name() << "' to load!" << std::endl;

        return nullptr;
      }

      return &it->second;
    }

    void register_deserialiser_factory(
            const std::string& type_name,
            DeserialiseToXtypeFunction register_func)
    {
      _deserialiser_factories[type_name] = std::move(register_func);
    }

    DeserialiseToXtypeFunction* get_deserialise_function(
            const xtypes::DynamicType& topic_type)
    {
      auto it = _deserialiser_factories.find(fastdds_type_to_is_name(std::string(topic_type->get_name())));
      if (it == _deserialiser_factories.end())
      {
        logger_ << utils::Logger::Level::ERROR
                << "get_deserialise_function' could not find a message type named '"
                << topic_type->get_name() << "' to load!" << std::endl;

        return nullptr;
      }

      return &it->second;
    }

    void register_subscription_factory(
            const std::string& topic_type,
            RegisterSubscriptionToFactory register_sub_func)
    {
        _subscription_factories[topic_type] = std::move(register_sub_func);
    }

    std::shared_ptr<void> create_subscription(
            const xtypes::DynamicType& topic_type,
            rclcpp::Node& node,
            const std::string& topic_name,
            TopicSubscriberSystem::SubscriptionCallback* callback,
            const rclcpp::QoS& qos_profile)
    {
        auto it = _subscription_factories.find(fastdds_type_to_is_name(std::string(topic_type->get_name())));
        if (it == _subscription_factories.end())
        {
            logger_ << utils::Logger::Level::ERROR
                    << "create_subscription' could not find a message type named '"
                    << topic_type->get_name() << "' to load!" << std::endl;

            return nullptr;
        }

        return it->second(node, topic_name, topic_type, callback, qos_profile);
    }

    void register_publisher_factory(
            const std::string& topic_type,
            RegisterPublisherToFactory register_pub_func)
    {
        _publisher_factories[topic_type] = std::move(register_pub_func);
    }

    std::shared_ptr<TopicPublisher> create_publisher(
            const xtypes::DynamicType& topic_type,
            rclcpp::Node& node,
            const std::string& topic_name,
            const rclcpp::QoS& qos_profile)
    {
        auto it = _publisher_factories.find(fastdds_type_to_is_name(std::string(topic_type->get_name())));
        if (it == _publisher_factories.end())
        {
            logger_ << utils::Logger::Level::ERROR
                    << "'create_publisher': could not find a message type named '"
                    << topic_type->get_name() << "' to load!" << std::endl;

            return nullptr;
        }

        return it->second(node, topic_name, qos_profile);
    }

    void register_client_proxy_factory(
            const std::string& service_response_type,
            RegisterServiceClientToFactory register_service_client_func)
    {
        _client_proxy_factories[service_response_type] = std::move(register_service_client_func);
    }

    std::shared_ptr<ServiceClient> create_client_proxy(
            const std::string& service_response_type,
            rclcpp::Node& node,
            const std::string& service_name,
            ServiceClientSystem::RequestCallback* callback,
            const rmw_qos_profile_t& qos_profile)
    {
        auto it = _client_proxy_factories.find(fastdds_type_to_is_name(service_response_type));
        if (it == _client_proxy_factories.end())
        {
            logger_ << utils::Logger::Level::ERROR
                    << "'create_client_proxy': could not find a service type named '"
                    << service_response_type << "' to load!" << std::endl;

            return nullptr;
        }

        return it->second(node, service_name, callback, qos_profile);
    }

    void register_server_proxy_factory(
            const std::string& service_request_type,
            RegisterServiceProviderToFactory register_service_server_func)
    {
        _server_proxy_factories[service_request_type] = std::move(register_service_server_func);
    }

    std::shared_ptr<ServiceProvider> create_server_proxy(
            const std::string& service_request_type,
            rclcpp::Node& node,
            const std::string& service_name,
            const rmw_qos_profile_t& qos_profile)
    {
        auto it = _server_proxy_factories.find(fastdds_type_to_is_name(service_request_type));
        if (it == _server_proxy_factories.end())
        {
            logger_ << utils::Logger::Level::ERROR
                    << "'create_server_proxy': could not find a service type named '"
                    << service_request_type << "' to load!" << std::endl;

            return nullptr;
        }

        return it->second(node, service_name, qos_profile);
    }

private:

    std::unordered_map<std::string, RegisterTypeToFactory> _type_factories;
    std::unordered_map<std::string, RegisterSubscriptionToFactory> _subscription_factories;
    std::unordered_map<std::string, RegisterPublisherToFactory> _publisher_factories;
    std::unordered_map<std::string, RegisterServiceClientToFactory> _client_proxy_factories;
    std::unordered_map<std::string, RegisterServiceProviderToFactory> _server_proxy_factories;

    std::unordered_map<std::string, SerialiseToROS2Function> _serialiser_factories;
    std::unordered_map<std::string, DeserialiseToXtypeFunction> _deserialiser_factories;


    utils::Logger logger_;
};

//==============================================================================
Factory& Factory::instance()
{
    static Factory factory;
    return factory;
}

//==============================================================================
void Factory::register_type_factory(
        const std::string& type_name,
        RegisterTypeToFactory register_type_func)
{
    _pimpl->register_type_factory(
        type_name, std::move(register_type_func));
}

//==============================================================================
xtypes::DynamicType Factory::create_type(
        const std::string& type_name)
{
    return _pimpl->create_type(type_name);
}

void Factory::register_serialiser_factory(
        const std::string& topic_type,
        Factory::SerialiseToROS2Function register_func)
{
    _pimpl->register_serialiser_factory(topic_type, std::move(register_func));
}

void Factory::register_deserialiser_factory(
        const std::string& topic_type,
        Factory::DeserialiseToXtypeFunction register_func)
{
    _pimpl->register_deserialiser_factory(topic_type, std::move(register_func));
}

Factory::SerialiseToROS2Function* Factory::get_serialise_function(
        const xtypes::DynamicType& topic_type)
{
    return _pimpl->get_serialise_function(topic_type);
}

Factory::DeserialiseToXtypeFunction* Factory::get_deserialise_function(
        const xtypes::DynamicType& topic_type)
{
    return _pimpl->get_deserialise_function(topic_type);
}

//==============================================================================
void Factory::register_subscription_factory(
        const std::string& topic_name,
        RegisterSubscriptionToFactory register_sub_func)
{
    _pimpl->register_subscription_factory(
        topic_name, std::move(register_sub_func));
}

//==============================================================================
std::shared_ptr<void> Factory::create_subscription(
        const xtypes::DynamicType& topic_type,
        rclcpp::Node& node,
        const std::string& topic_name,
        TopicSubscriberSystem::SubscriptionCallback* callback,
        const rclcpp::QoS& qos_profile)
{
    return _pimpl->create_subscription(
        topic_type, node, topic_name, callback, qos_profile);
}

//==============================================================================
void Factory::register_publisher_factory(
        const std::string& topic_type,
        RegisterPublisherToFactory register_pub_func)
{
    _pimpl->register_publisher_factory(
        topic_type, std::move(register_pub_func));
}

//==============================================================================
std::shared_ptr<TopicPublisher> Factory::create_publisher(
        const xtypes::DynamicType& topic_type,
        rclcpp::Node& node,
        const std::string& topic_name,
        const rclcpp::QoS& qos_profile)
{
    return _pimpl->create_publisher(
        topic_type, node, topic_name, qos_profile);
}

//==============================================================================
void Factory::register_client_proxy_factory(
        const std::string& service_response_type,
        RegisterServiceClientToFactory register_service_client_func)
{
    _pimpl->register_client_proxy_factory(
        service_response_type, std::move(register_service_client_func));
}

//==============================================================================
std::shared_ptr<ServiceClient> Factory::create_client_proxy(
        const std::string& service_type,
        rclcpp::Node& node,
        const std::string& service_name,
        ServiceClientSystem::RequestCallback* callback,
        const rmw_qos_profile_t& qos_profile)
{
    return _pimpl->create_client_proxy(
        service_type, node, service_name, callback, qos_profile);
}

//==============================================================================
void Factory::register_server_proxy_factory(
        const std::string& service_request_type,
        RegisterServiceProviderToFactory register_service_server_func)
{
    _pimpl->register_server_proxy_factory(
        service_request_type, std::move(register_service_server_func));
}

//==============================================================================
std::shared_ptr<ServiceProvider> Factory::create_server_proxy(
        const std::string& service_request_type,
        rclcpp::Node& node,
        const std::string& service_name,
        const rmw_qos_profile_t& qos_profile)
{
    return _pimpl->create_server_proxy(
        service_request_type, node, service_name, qos_profile);
}

//==============================================================================
Factory::Factory()
    : _pimpl(new Implementation)
{
}

} //  namespace ros2
} //  namespace sh
} //  namespace is
} //  namespace eprosima

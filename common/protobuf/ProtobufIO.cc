// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "thirdparty/google/protobuf/message.h"
#include "thirdparty/google/protobuf/descriptor.h"

#include "thirdparty/rapidjson/document.h"
#include "thirdparty/rapidjson/error/en.h"
#include "thirdparty/rapidjson/stringbuffer.h"
#include "thirdparty/rapidjson/prettywriter.h"

#include <vector>

#include <boost/lexical_cast.hpp>

#include <claire/common/strings/UriEscape.h>
#include <claire/common/strings/StringPiece.h>
#include <claire/common/logging/Logging.h>

namespace claire {

namespace {

template<typename Allocator>
bool SerializeToJsonValue(const ::google::protobuf::Message& message,
                          rapidjson::Value* root,
                          Allocator& allocator)
{
    auto reflection = message.GetReflection();

    std::vector<const ::google::protobuf::FieldDescriptor*> fields;
    reflection->ListFields(message, &fields);

    for (auto it = fields.begin(); it != fields.end(); ++it)
    {
        auto field = *it;
        if (!field->is_repeated() && !reflection->HasField(message, field))
        {
            if (field->is_required())
            {
                LOG(ERROR) << "missing field " << field->full_name() << " .";
                return false;
            }
            continue;
        }

        switch (field->cpp_type())
        {
            #undef HANDLE_CASE_CPP_TYPE
            #define HANDLE_CASE_CPP_TYPE(cpptype, jsontype, method)                 \
            case ::google::protobuf::FieldDescriptor::CPPTYPE_##cpptype:            \
            {                                                                       \
                rapidjson::Value key;                                               \
                key.SetString(field->name().c_str(),                                \
                              static_cast<unsigned int>(field->name().length()));   \
                rapidjson::Value value;                                             \
                if (field->is_repeated())                                           \
                {                                                                   \
                    value.SetArray();                                               \
                    for (int i = 0; i < reflection->FieldSize(message, field); ++i) \
                    {                                                               \
                        value.PushBack(                                             \
                                reflection->GetRepeated##method(message, field, i), \
                                allocator);                                         \
                    }                                                               \
                }                                                                   \
                else                                                                \
                {                                                                   \
                    value.Set##jsontype(reflection->Get##method(message, field));   \
                }                                                                   \
                root->AddMember(key, value, allocator);                             \
                break;                                                              \
            }
            HANDLE_CASE_CPP_TYPE(INT32,  Int,    Int32);
            HANDLE_CASE_CPP_TYPE(UINT32, Uint,   UInt32);
            HANDLE_CASE_CPP_TYPE(FLOAT,  Double, Float);
            HANDLE_CASE_CPP_TYPE(DOUBLE, Double, Double);
            HANDLE_CASE_CPP_TYPE(BOOL,   Bool,   Bool);
            HANDLE_CASE_CPP_TYPE(INT64,  Int64,  Int64);
            HANDLE_CASE_CPP_TYPE(UINT64, Uint64, UInt64);
            #undef HANDLE_CASE_CPP_TYPE

            case ::google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
            {
                rapidjson::Value key;
                key.SetString(field->name().c_str(), static_cast<unsigned int>(field->name().length()));

                rapidjson::Value value;
                if (field->is_repeated())
                {
                    value.SetArray();
                    for (int i = 0;i < reflection->FieldSize(message, field); ++i)
                    {
                        rapidjson::Value item(reflection->GetRepeatedEnum(message, field, i)->number());
                        value.PushBack(item, allocator);
                    }
                }
                else
                {
                    value.SetInt(reflection->GetEnum(message, field)->number());
                }
                root->AddMember(key, value, allocator);
                break;
            }

            case ::google::protobuf::FieldDescriptor::CPPTYPE_STRING:
            {
                rapidjson::Value key;
                key.SetString(field->name().c_str(), static_cast<unsigned int>(field->name().length()));

                rapidjson::Value value;
                if (field->is_repeated())
                {
                    value.SetArray();
                    for (int i = 0;i < reflection->FieldSize(message, field); ++i)
                    {
                        auto field_data =
                            reflection->GetRepeatedStringReference(message, field, i, NULL);
                        if (field->type() == ::google::protobuf::FieldDescriptor::TYPE_BYTES)
                        {
                            auto escaped_data = UriEscape(field_data, EscapeMode::ALL);
                            rapidjson::Value escaped_value(escaped_data.data(),
                                                           static_cast<unsigned int>(escaped_data.length()),
                                                           allocator);
                            value.PushBack(escaped_value, allocator);
                        }
                        else
                        {
                            rapidjson::Value field_value(field_data.data(),
                                                         static_cast<unsigned int>(field_data.length()),
                                                         allocator);
                            value.PushBack(field_value, allocator);
                        }
                    }
                }
                else
                {
                    auto field_data = reflection->GetStringReference(message, field, NULL);
                    if (field->type() == ::google::protobuf::FieldDescriptor::TYPE_BYTES)
                    {
                        auto escaped_data = UriEscape(field_data, EscapeMode::ALL);
                        value.SetString(escaped_data.data(), static_cast<unsigned int>(escaped_data.length()));
                    }
                    else
                    {
                        value.SetString(field_data.data(), static_cast<unsigned int>(field_data.length()));
                    }
                }
                root->AddMember(key, value, allocator);
                break;
            }

            case ::google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
            {
                rapidjson::Value value;
                if (field->is_repeated())
                {
                    value.SetArray();
                    for (int i = 0; i < reflection->FieldSize(message, field); ++i)
                    {
                        rapidjson::Value item(rapidjson::kObjectType);
                        if (!SerializeToJsonValue(reflection->GetRepeatedMessage(message, field, i),
                                                  &item,
                                                  allocator))
                        {
                            return false;
                        }
                        value.PushBack(item, allocator);
                    }
                }
                else
                {
                    value.SetObject();
                    if (!SerializeToJsonValue(reflection->GetMessage(message, field),
                                              &value,
                                              allocator))
                    {
                        return false;
                    }
                }
                root->AddMember(rapidjson::Value(field->name().c_str(), allocator), value, allocator);
                break;
            }
        }
    }
    return true;
}

bool ParseFromJsonValue(const rapidjson::Value& root,
                        ::google::protobuf::Message* message)
{
    const auto reflection = message->GetReflection();
    const auto descriptor = message->GetDescriptor();

    std::vector<const ::google::protobuf::FieldDescriptor*> fields;
    for (int i = 0; i < descriptor->field_count(); ++i)
    {
        fields.push_back(descriptor->field(i));
    }

    //for (int i = 0; i < descriptor->extension_range_count(); ++i)
    //{
    //    const auto range = descriptor->extension_range(i);
    //    for (auto tag = range->start; tag < range->end; ++tag)
    //    {
    //        auto field = reflection->FindKnownExtensionByNumber(tag);
    //        if (field)
    //        {
    //            fields.push_back(field);
    //        }
    //    }
    //}

    for (auto it = fields.begin(); it != fields.end(); ++it)
    {
        auto field = *it;
        if (!root.HasMember(field->name().c_str()))
        {
            if (field->is_required())
            {
                LOG(ERROR) << "missing required field " << field->full_name();
                return false;
            }
            continue;
        }
        auto& value = root[field->name().c_str()];

        if (field->is_repeated())
        {
            if (!value.IsArray())
            {
                LOG(ERROR) << "invalid type for array field "
                           << field->full_name() << " .";
                return false;
            }
        }

        switch (field->cpp_type())
        {
            #undef HANDLE_CASE_CPP_TYPE
            #define HANDLE_CASE_CPP_TYPE(cpptype, method, jsontype, valuetype)                  \
            case ::google::protobuf::FieldDescriptor::CPPTYPE_##cpptype:                        \
            {                                                                                   \
                if (field->is_repeated())                                                       \
                {                                                                               \
                    for (auto item = value.Begin();                                             \
                         item != value.End();                                                   \
                         ++item)                                                                \
                    {                                                                           \
                        if (!item->Is##jsontype())                                              \
                        {                                                                       \
                            LOG(ERROR) << "invalid type for field "                             \
                                       << field->full_name() << " .";                           \
                            return false;                                                       \
                        }                                                                       \
                        reflection->Add##method(message,                                        \
                                                field,                                          \
                                                static_cast<valuetype>(item->Get##jsontype())); \
                    }                                                                           \
                }                                                                               \
                else                                                                            \
                {                                                                               \
                    reflection->Set##method(message,                                            \
                                            field,                                              \
                                            static_cast<valuetype>(value.Get##jsontype()));     \
                }                                                                               \
                break;                                                                          \
            }
            HANDLE_CASE_CPP_TYPE(INT32,  Int32,  Int,    int);
            HANDLE_CASE_CPP_TYPE(UINT32, UInt32, Uint,   unsigned int);
            HANDLE_CASE_CPP_TYPE(FLOAT,  Float,  Double, float);
            HANDLE_CASE_CPP_TYPE(DOUBLE, Double, Double, double);
            HANDLE_CASE_CPP_TYPE(BOOL,   Bool,   Bool,   bool);
            #undef HANDLE_CASE_CPP_TYPE

            #define HANDLE_CASE_CPP_TYPE(cpptype, method, jsontype, valuetype)                  \
            case ::google::protobuf::FieldDescriptor::CPPTYPE_##cpptype:                        \
            {                                                                                   \
                if (field->is_repeated())                                                       \
                {                                                                               \
                    for (auto item = value.Begin();                                             \
                         item != value.End();                                                   \
                         ++item)                                                                \
                    {                                                                           \
                        if (item->Is##jsontype())                                               \
                        {                                                                       \
                            reflection->Add##method(message,                                    \
                                                    field,                                      \
                                                    item->Get##jsontype());                     \
                        }                                                                       \
                        else if (item->IsString())                                              \
                        {                                                                       \
                            auto number = boost::lexical_cast<valuetype>(item->GetString());    \
                            reflection->Add##method(message, field, number);                    \
                        }                                                                       \
                        else                                                                    \
                        {                                                                       \
                            LOG(ERROR) << "invalid type for field "                             \
                                       << field->full_name() << " .";                           \
                            return false;                                                       \
                        }                                                                       \
                    }                                                                           \
                }                                                                               \
                else                                                                            \
                {                                                                               \
                   if (value.Is##jsontype())                                                    \
                   {                                                                            \
                       reflection->Set##method(message, field, value.Get##jsontype());          \
                   }                                                                            \
                   else if (value.IsString())                                                   \
                   {                                                                            \
                       valuetype number  = boost::lexical_cast<valuetype>(value.GetString());   \
                       reflection->Set##method(message, field, number);                         \
                   }                                                                            \
                   else                                                                         \
                   {                                                                            \
                       LOG(ERROR) << "invalid type for field " << field->full_name() << " .";   \
                       return false;                                                            \
                   }                                                                            \
                }                                                                               \
                break;                                                                          \
            }
            HANDLE_CASE_CPP_TYPE(INT64,  Int64,  Int64,  int64_t);
            HANDLE_CASE_CPP_TYPE(UINT64, UInt64, Uint64, uint64_t);
            #undef HANDLE_CASE_CPP_TYPE

            case ::google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
            {
                if (field->is_repeated())
                {
                    for (auto item = value.Begin(); item != value.End(); ++item)
                    {
                        if (!item->IsInt())
                        {
                            LOG(ERROR) << "invalid type for field " << field->full_name() << " .";
                            return false;
                        }

                        const auto enum_value_descriptor =
                            field->enum_type()->FindValueByNumber(item->GetInt());
                        if (!enum_value_descriptor)
                        {
                            LOG(ERROR) << "invalid value for enum field " << field->full_name() << " .";
                            return false;
                        }
                        reflection->AddEnum(message, field, enum_value_descriptor);
                    }
                }
                else
                {
                    if (!value.IsInt())
                    {
                        LOG(ERROR) << "invalid type for field " << field->full_name() << " .";
                        return false;
                    }

                    const auto enum_value_descriptor =
                        field->enum_type()->FindValueByNumber(value.GetInt());
                    if (!enum_value_descriptor)
                    {
                        LOG(ERROR) << "invalid value for enum field " << field->full_name() << " .";
                        return false;
                    }
                    reflection->SetEnum(message, field, enum_value_descriptor);
                }
                break;
            }

            case ::google::protobuf::FieldDescriptor::CPPTYPE_STRING:
            {
                if (field->is_repeated())
                {
                    for (auto item = value.Begin(); item != value.End(); ++item)
                    {
                        if (!item->IsString())
                        {
                            LOG(ERROR) << "invalid type for field " << field->full_name() << " .";
                            return false;
                        }

                        if (field->type() == ::google::protobuf::FieldDescriptor::TYPE_BYTES)
                        {
                            auto unescaped_item = UriUnescape(item->GetString(), EscapeMode::ALL);
                            reflection->AddString(message, field, unescaped_item);
                        }
                        else
                        {
                            reflection->AddString(message, field, item->GetString());
                        }
                    }
                }
                else
                {
                    if (!value.IsString())
                    {
                        LOG(ERROR) << "invalid type for field " << field->full_name() << " .";
                        return false;
                    }

                    if (field->type() == ::google::protobuf::FieldDescriptor::TYPE_BYTES)
                    {
                        auto unescaped_item = UriUnescape(value.GetString(), EscapeMode::ALL);
                        reflection->SetString(message, field, unescaped_item);
                    }
                    else
                    {
                        reflection->SetString(message, field, value.GetString());
                    }
                }
                break;
            }

            case ::google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
            {
                if (field->is_repeated())
                {
                    for (auto item = value.Begin(); item != value.End(); ++item)
                    {
                        if (!item->IsObject())
                        {
                            LOG(ERROR) << "invalid type for field " << field->full_name() << " .";
                            return false;
                        }

                        auto ret = ParseFromJsonValue(*item,
                                                      reflection->AddMessage(message, field));
                        if (!ret)
                        {
                            return ret;
                        }
                    }
                }
                else
                {
                    if (!value.IsObject())
                    {
                        LOG(ERROR) << "invalid type for field " << field->full_name() << " .";
                        return false;
                    }

                    auto ret = ParseFromJsonValue(value,
                                                  reflection->MutableMessage(message, field));
                    if (!ret)
                    {
                        return ret;
                    }
                }
                break;
            }
        }
    }
    return true;
}

} // namespace

bool SerializeToJson(const ::google::protobuf::Message& message,
                     std::string* output)
{
    rapidjson::Document root;
    root.SetObject();
    if (SerializeToJsonValue(message, &root, root.GetAllocator()))
    {
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        root.Accept(writer);
        output->assign(buffer.GetString());
        return true;
    }
    return false;
}

bool ParseFromJson(const StringPiece& json,
                   ::google::protobuf::Message* message)
{
    rapidjson::Document root;
    if (root.Parse<0>(json.data()).HasParseError())
    {
        LOG(ERROR) << "parse json failed: " << StringPiece(GetParseError_En(root.GetParseError()));
        return false;
    }

    return ParseFromJsonValue(root, message);
}

} // namespace claire

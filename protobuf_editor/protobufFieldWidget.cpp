#include "protobufFieldWidget.hpp"

namespace pb = google::protobuf;

namespace protobuf_editor {

ProtobufFieldWidget::ProtobufFieldWidget(const google::protobuf::FieldDescriptor *fieldDescriptor, QWidget *parent) : QWidget(parent), fieldDescriptor_(fieldDescriptor) {
  setEnabled(false);
  if (fieldDescriptor_ == nullptr) {
    // No field descriptor, must be a top-level, thus is not optional and is always set
    fieldIsOptional_ = false;
  } else {
    fieldIsOptional_ = fieldDescriptor_->has_optional_keyword();
  }
}

ProtobufFieldWidget::~ProtobufFieldWidget() {}

void ProtobufFieldWidget::setMessage(pb::Message *currentMessage, pb::Message *parentMessage) {
  currentMessage_ = currentMessage;
  parentMessage_ = parentMessage;

  setDataFromMessage();

  setEnabled(true);
}

void ProtobufFieldWidget::setDataFromMessage() {
  // Nothing to do
}

bool ProtobufFieldWidget::fieldIsOptional() const {
  return fieldIsOptional_;
}

bool ProtobufFieldWidget::fieldIsSet() const {
  if (fieldDescriptor_ == nullptr) {
    // No field descriptor, must be a top-level, thus is not optional and is always set
    return true;
  }
  if (!fieldIsOptional_) {
    // Non-optional fields are always set
    return true;
  }
  if (parentMessage_ == nullptr) {
    throw std::runtime_error("Cannot check if a field is set for a root-level field");
  }
  return parentMessage_->GetReflection()->HasField(*parentMessage_, fieldDescriptor_);
}

} // namespace protobuf_editor
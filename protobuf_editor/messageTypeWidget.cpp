#include "builtInTypeWidget.hpp"
#include "messageTypeWidget.hpp"

#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>

namespace pb = google::protobuf;

namespace protobuf_editor {

MessageTypeWidget::MessageTypeWidget(const pb::Descriptor *descriptor, const pb::FieldDescriptor *fieldDescriptor, QWidget *parent) : ProtobufFieldWidget(fieldDescriptor, parent), descriptor_(descriptor) {
  buildWidget();
}

void MessageTypeWidget::buildWidget() {
  // Create a layout (LayoutA) for our entire widget
  QVBoxLayout *overallLayout = new QVBoxLayout(this);
  // Specify no margin for this layout, since we want it to look like the QGroupBox is what we are
  overallLayout->setContentsMargins(0,0,0,0);
  // TODO: Ideally we'd inherit from QGroupBox, but that seems tricky given that we've already inherited from something which inherits from QObject

  // Create a groupbox for this message
  if (fieldDescriptor_ != nullptr) {
    // We are a nested message
    groupBox_ = new QGroupBox(QString::fromStdString(fieldDescriptor_->full_name()));
  } else {
    // We are a root-level message
    // TODO: Dont create a group box, instead just create a QWidget
    groupBox_ = new QGroupBox(tr("Root-level Message"));
  }
  
  // Put the groupbox (and only that groupbox) into the layout LayoutA
  overallLayout->addWidget(groupBox_);
  
  // Create a layout (LayoutB) inside the groupbox
  QVBoxLayout *groupBoxLayout = new QVBoxLayout(groupBox_);

  // If this field is optional, the groupbox will have a checkbox.
  // When unchecked, the contents of the groupbox will be disabled, as an already-existing feature of the QGroupBox.
  if (fieldIsOptional()) {
    groupBox_->setCheckable(true);
    // groupBox_->setChecked(true);
    connect(groupBox_, &QGroupBox::toggled, [this](bool enabled){
      if (parentMessage_ == nullptr) {
        throw std::runtime_error("Something went wrong. This should not be possible without a message");
      }
      if (!fieldIsOptional()) {
        throw std::runtime_error("Should not be able to toggle groupbox for non-optioal message");
      }
      const pb::Reflection *reflection = parentMessage_->GetReflection();
      if (enabled) {
        pb::Message* newCurrentMessage = reflection->MutableMessage(parentMessage_, fieldDescriptor_);
        setMessage(newCurrentMessage, parentMessage_);
      } else {
        reflection->ClearField(parentMessage_, fieldDescriptor_);
      }
      emit messageUpdated();
    });
  }

  // Iterate over all fields, create widgets for them, and add them to our layout
  for (int fieldIndex=0; fieldIndex<descriptor_->field_count(); ++fieldIndex) {
    const pb::FieldDescriptor *fieldDescriptor = descriptor_->field(fieldIndex);

    // Skip unhandled types for now
    if (fieldDescriptor->real_containing_oneof() != nullptr) {
      std::cout << "Skipping oneof \"" << fieldDescriptor->full_name() << "\" for now" << std::endl;
      groupBoxLayout->addWidget(new QLabel(tr("[skipped] ")+QString::fromStdString(fieldDescriptor->full_name())));
      nestedWidgets_.push_back(nullptr);
      continue;
    }
    
    if (fieldDescriptor->is_map()) {
      // Map is also "repeated", handle map first then move to next item
      std::cout << "Skipping map \"" << fieldDescriptor->full_name() << "\" for now" << std::endl;
      groupBoxLayout->addWidget(new QLabel(tr("[skipped] ")+QString::fromStdString(fieldDescriptor->full_name())));
      nestedWidgets_.push_back(nullptr);
      continue;
    }
    
    if (fieldDescriptor->is_repeated()) {
      std::cout << "Skipping repeated \"" << fieldDescriptor->full_name() << "\" for now" << std::endl;
      groupBoxLayout->addWidget(new QLabel(tr("[skipped] ")+QString::fromStdString(fieldDescriptor->full_name())));
      nestedWidgets_.push_back(nullptr);
      continue;
    }

    if (fieldDescriptor->type() == pb::FieldDescriptor::Type::TYPE_GROUP) {
      std::cout << "Skipping group \"" << fieldDescriptor->full_name() << "\" for now" << std::endl;
      groupBoxLayout->addWidget(new QLabel(tr("[skipped] ")+QString::fromStdString(fieldDescriptor->full_name())));
      nestedWidgets_.push_back(nullptr);
      continue;
    }
    
    ProtobufFieldWidget *widgetForField;
    if (fieldDescriptor->type() == pb::FieldDescriptor::Type::TYPE_MESSAGE) {
      // Is a nested message type
      const pb::Descriptor *nestedDescriptor = fieldDescriptor->message_type();
      if (nestedDescriptor == nullptr) {
          throw std::runtime_error("Nested field descriptor is null");
      }
      widgetForField = new MessageTypeWidget(nestedDescriptor, fieldDescriptor);
    } else {
      // Is a built-in type
      widgetForField = new BuiltInTypeWidget(fieldDescriptor);
    }
    connect(widgetForField, &ProtobufFieldWidget::messageUpdated, this, &ProtobufFieldWidget::messageUpdated);
    groupBoxLayout->addWidget(widgetForField);
    nestedWidgets_.push_back(widgetForField);
  }
}

void MessageTypeWidget::setDataFromMessage() {
  if (groupBox_ == nullptr) {
    throw std::runtime_error("Received a message, but QGroupBox is not set");
  }

  if (currentMessage_ == nullptr) {
    if (!fieldIsOptional()) {
      throw std::runtime_error("Does not make sense to have a null message for a non-optional message field");
    }
    if (fieldIsSet()) {
      throw std::runtime_error("Field is set, but given a null message");
    }
    if (!groupBox_->isCheckable()) {
      throw std::runtime_error("Field is optional, but QGroupBox is not checkable");
    }
    // Message is not set, we're disabled
    groupBox_->setChecked(false);
    // Nothing else to do
    return;
  }

  // If this message has nested messages, set those messages for all of our nested widgets
  if (descriptor_->field_count() != nestedWidgets_.size()) {
    throw std::runtime_error("Expecting descriptor to have the same number of fields as we have widgets");
  }

  // Recursively set the message for nested widgets
  for (int fieldIndex=0; fieldIndex<descriptor_->field_count(); ++fieldIndex) {
    auto *nestedFieldWidget = nestedWidgets_.at(fieldIndex);
    if (nestedFieldWidget == nullptr) {
      // TODO: Throw here once we handle all field types
      std::cout << "Warning! Nested widget is null. This is ok now since we skip certain pb field types" << std::endl;
      continue;
    }
    // Check if this field of the message is a nested message
    const pb::FieldDescriptor *nestedFieldDescriptor = descriptor_->field(fieldIndex);
    if (nestedFieldDescriptor->type() == pb::FieldDescriptor::Type::TYPE_MESSAGE) {
      if (dynamic_cast<MessageTypeWidget*>(nestedFieldWidget) == nullptr) {
        throw std::runtime_error("Field is a message, but the corresponding widget is not for a message");
      }
      // Get the nested message within the message that we were just given, so that we can pass it to the widget, if it exists
      const pb::Reflection *reflection = currentMessage_->GetReflection();
      if (!nestedFieldDescriptor->has_optional_keyword() || reflection->HasField(*currentMessage_, nestedFieldDescriptor)) {
        pb::Message *nestedMessage = currentMessage_->GetReflection()->MutableMessage(currentMessage_, nestedFieldDescriptor);
        nestedFieldWidget->setMessage(nestedMessage, currentMessage_);
      } else {
        nestedFieldWidget->setMessage(nullptr, currentMessage_);
      }
    } else {
      // The message which holds this field's data is also its parent message
      nestedFieldWidget->setMessage(currentMessage_, currentMessage_);
    }
  }

  // Set whether we're enabled or disabled based on the data in the given message
  if (fieldIsOptional()) {
    if (!groupBox_->isCheckable()) {
      throw std::runtime_error("Field is optional, but QGroupBox is not checkable");
    }
    groupBox_->setChecked(fieldIsSet());
  }
}

} // namespace protobuf_editor
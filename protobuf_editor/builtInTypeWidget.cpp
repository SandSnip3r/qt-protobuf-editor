#include "builtInTypeWidget.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>

namespace pb = google::protobuf;

namespace {

int parseEnumFromWidget(const QWidget *widget);
bool parseBoolFromWidget(const QWidget *widget);

template<typename T>
T parseDataFromWidget(const QWidget *widget);

template<typename T>
void writeNumberToWidget(const QWidget *widget, const T &data);
void writeEnumIndexToWidget(QWidget *widget, const int index);
void writeBoolToWidget(QWidget *widget, const bool data);
void writeStringToWidget(QWidget *widget, const std::string &data);

template<>
std::string parseDataFromWidget<std::string>(const QWidget *widget) {
  const QLineEdit *lineEdit = dynamic_cast<const QLineEdit*>(widget);
  if (lineEdit == nullptr) {
    throw std::runtime_error("Parsing data from widget, std::string, but widget is not a lineedit");
  }
  return lineEdit->text().toStdString();
}

template<>
float parseDataFromWidget<float>(const QWidget *widget) {
  const QLineEdit *lineEdit = dynamic_cast<const QLineEdit*>(widget);
  if (lineEdit == nullptr) {
    throw std::runtime_error("Parsing data from widget, float, but widget is not a lineedit");
  }
  return lineEdit->text().toFloat();
}

template<>
double parseDataFromWidget<double>(const QWidget *widget) {
  const QLineEdit *lineEdit = dynamic_cast<const QLineEdit*>(widget);
  if (lineEdit == nullptr) {
    throw std::runtime_error("Parsing data from widget, double, but widget is not a lineedit");
  }
  return lineEdit->text().toDouble();
}

template<>
int32_t parseDataFromWidget<int32_t>(const QWidget *widget) {
  const QLineEdit *lineEdit = dynamic_cast<const QLineEdit*>(widget);
  if (lineEdit == nullptr) {
    throw std::runtime_error("Parsing data from widget, int32_t, but widget is not a lineedit");
  }
  if (std::is_same_v<int, int32_t>) {
    return lineEdit->text().toInt();
  } else {
    throw std::runtime_error("Weird type width");
  }
}

template<>
uint32_t parseDataFromWidget<uint32_t>(const QWidget *widget) {
  const QLineEdit *lineEdit = dynamic_cast<const QLineEdit*>(widget);
  if (lineEdit == nullptr) {
    throw std::runtime_error("Parsing data from widget, uint32_t, but widget is not a lineedit");
  }
  if (std::is_same_v<uint, uint32_t>) {
    return lineEdit->text().toUInt();
  } else {
    throw std::runtime_error("Weird type width");
  }
}

template<>
int64_t parseDataFromWidget<int64_t>(const QWidget *widget) {
  const QLineEdit *lineEdit = dynamic_cast<const QLineEdit*>(widget);
  if (lineEdit == nullptr) {
    throw std::runtime_error("Parsing data from widget, int64_t, but widget is not a lineedit");
  }
  return lineEdit->text().toLongLong();
}

template<>
uint64_t parseDataFromWidget<uint64_t>(const QWidget *widget) {
  const QLineEdit *lineEdit = dynamic_cast<const QLineEdit*>(widget);
  if (lineEdit == nullptr) {
    throw std::runtime_error("Parsing data from widget, uint64_t, but widget is not a lineedit");
  }
  return lineEdit->text().toULongLong();
}

template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
void writeNumberToWidget(QWidget *widget, const T &data) {
  QLineEdit *lineEdit = dynamic_cast<QLineEdit*>(widget);
  if (lineEdit == nullptr) {
    throw std::runtime_error("Writing data to widget, std::is_arithmetic_v, but widget is not a lineedit");
  }
  lineEdit->setText(QString::number(data));
}

}

namespace protobuf_editor {
  
BuiltInTypeWidget::BuiltInTypeWidget(const pb::FieldDescriptor *fieldDescriptor, QWidget *parent) : ProtobufFieldWidget(fieldDescriptor, parent) {
  buildWidget();
}

void BuiltInTypeWidget::buildWidget() {
  if (fieldDescriptor_->type() == pb::FieldDescriptor::Type::TYPE_MESSAGE) {
    throw std::runtime_error("Built-in Type Widget was constructed with a field which is of type \"message\"");
  }

  if (fieldDescriptor_->real_containing_oneof() != nullptr) {
    throw std::runtime_error("Previous logic should prevent us from receiving a oneof-type");
  }
  
  if (fieldDescriptor_->is_map()) {
    throw std::runtime_error("Previous logic should prevent us from receiving a map-type");
  }
  
  if (fieldDescriptor_->is_repeated()) {
    throw std::runtime_error("Previous logic should prevent us from receiving a repeated-type");
  }

  if (fieldDescriptor_->type() == pb::FieldDescriptor::Type::TYPE_GROUP) {
    throw std::runtime_error("Previous logic should prevent us from receiving a group-type");
  }

  // If the field is optional, the label will actually be a checkbox with text, otherwise, it will just be a plain label.
  if (fieldIsOptional()) {
    QCheckBox *labelAsCheckBox = new QCheckBox(QString::fromStdString(fieldDescriptor_->full_name()));
    // Default with the field enabled. When we receive a message, if the field is not set, we'll uncheck this
    labelAsCheckBox->setChecked(true);
    labelWidget_ = labelAsCheckBox;
  } else {
    labelWidget_ = new QLabel(QString::fromStdString(fieldDescriptor_->full_name()));
  }

  if (fieldDescriptor_->type() == pb::FieldDescriptor::Type::TYPE_ENUM) {
    // Enums are a QComboBox widget
    QComboBox *comboBox = new QComboBox;
    const pb::EnumDescriptor *enumDescriptor = fieldDescriptor_->enum_type();
    for (int enumValueIndex=0; enumValueIndex<enumDescriptor->value_count(); ++enumValueIndex) {
      const pb::EnumValueDescriptor *enumValueDescriptor = enumDescriptor->value(enumValueIndex);
      comboBox->addItem(QString::fromStdString(enumValueDescriptor->name()));
    }
    // If the user changes the selection, they're changing the value of the field in the protobuf
    connect(comboBox, &QComboBox::currentIndexChanged, [this](int index){
      if (currentMessage_ == nullptr) {
        throw std::runtime_error("Something went wrong. This should not be possible without a message");
      }
      const pb::Reflection *reflection = currentMessage_->GetReflection();
      const pb::EnumDescriptor *enumDescriptor = fieldDescriptor_->enum_type();
      const pb::EnumValueDescriptor *enumValueDescriptor = enumDescriptor->value(index);
      reflection->SetEnum(currentMessage_, fieldDescriptor_, enumValueDescriptor);
      emit messageUpdated();
    });
    dataWidget_ = comboBox;
  } else if (fieldDescriptor_->type() == pb::FieldDescriptor::Type::TYPE_BOOL) {
    // Booleans are a QCheckBox widget
    QCheckBox *checkBox = new QCheckBox;

    // If the user toggles this checkbox, they're updating the bool in the protobuf
    connect(checkBox, &QCheckBox::toggled, [this](bool checked){
      if (currentMessage_ == nullptr) {
        throw std::runtime_error("Something went wrong. This should not be possible without a message");
      }
      const pb::Reflection *reflection = currentMessage_->GetReflection();
      reflection->SetBool(currentMessage_, fieldDescriptor_, checked);
      emit messageUpdated();
    });

    dataWidget_ = checkBox;
  } else {
    // All other built-in types are a QLineEdit widget
    QLineEdit *lineEdit = new QLineEdit;
    lineEdit->setMinimumWidth(200);
    connect(lineEdit, &QLineEdit::textChanged, [this](const QString &text){
      if (currentMessage_ == nullptr) {
        throw std::runtime_error("Something went wrong. This should not be possible without a message");
      }
      const pb::Reflection *reflection = currentMessage_->GetReflection();
      bool success = true;
      switch (fieldDescriptor_->type()) {
        case pb::FieldDescriptor::Type::TYPE_BYTES:
        case pb::FieldDescriptor::Type::TYPE_STRING: {
          reflection->SetString(currentMessage_, fieldDescriptor_, text.toStdString());
          break;
        }
        case pb::FieldDescriptor::Type::TYPE_FLOAT: {
          auto parsedData = text.toFloat(&success);
          if (success) {
            reflection->SetFloat(currentMessage_, fieldDescriptor_, parsedData);
          }
          break;
        }
        case pb::FieldDescriptor::Type::TYPE_DOUBLE: {
          auto parsedData = text.toDouble(&success);
          if (success) {
            reflection->SetDouble(currentMessage_, fieldDescriptor_, parsedData);
          }
          break;
        }
        case pb::FieldDescriptor::Type::TYPE_INT32:
        case pb::FieldDescriptor::Type::TYPE_SINT32:
        case pb::FieldDescriptor::Type::TYPE_SFIXED32: {
          auto parsedData = text.toInt(&success);
          if (success) {
            reflection->SetInt32(currentMessage_, fieldDescriptor_, parsedData);
          }
          break;
        }
        case pb::FieldDescriptor::Type::TYPE_UINT32:
        case pb::FieldDescriptor::Type::TYPE_FIXED32: {
          auto parsedData = text.toUInt(&success);
          if (success) {
            reflection->SetUInt32(currentMessage_, fieldDescriptor_, parsedData);
          }
          break;
        }
        case pb::FieldDescriptor::Type::TYPE_INT64:
        case pb::FieldDescriptor::Type::TYPE_SINT64:
        case pb::FieldDescriptor::Type::TYPE_SFIXED64: {
          auto parsedData = text.toLongLong(&success);
          if (success) {
            reflection->SetInt64(currentMessage_, fieldDescriptor_, parsedData);
          }
          break;
        }
        case pb::FieldDescriptor::Type::TYPE_UINT64:
        case pb::FieldDescriptor::Type::TYPE_FIXED64: {
          auto parsedData = text.toULongLong(&success);
          if (success) {
            reflection->SetUInt64(currentMessage_, fieldDescriptor_, parsedData);
          }
          break;
        }
        default:
          throw std::runtime_error("Unhandled type");
          break;
      }
      if (success) {
        emit messageUpdated();
      } else {
        // TODO:
        std::cout << "Failed to parse" << std::endl;
      }
    });
    dataWidget_ = lineEdit;
  }

  if (labelWidget_ == nullptr) {
    throw std::runtime_error("Didnt build a label widget");
  }

  if (dataWidget_ == nullptr) {
    throw std::runtime_error("Didnt build a data widget");
  }

  if (fieldIsOptional()) {
    QCheckBox *labelAsCheckBox = dynamic_cast<QCheckBox*>(labelWidget_);
    if (labelAsCheckBox == nullptr) {
      throw std::runtime_error("Label for optional is not a checkbox");
    }

    // When the checkbox is toggled, we will enable/disable the connected widget
    connect(labelAsCheckBox, &QCheckBox::toggled, dataWidget_, &QWidget::setEnabled);

    // When the checkbox is toggled, the user is setting or unsetting this optional field
    connect(labelAsCheckBox, &QCheckBox::toggled, [this](bool checked) {
      if (currentMessage_ == nullptr)  {
        throw std::runtime_error("Something went wrong. This should not be possible without a message");
      }

      const pb::Reflection *reflection = currentMessage_->GetReflection();
      if (!fieldIsOptional()) {
        throw std::runtime_error("Tried to set/unset optional, but this field isnt optional");
      }

      if (!checked) {
        // Box was unchecked, unset value
        reflection->ClearField(currentMessage_, fieldDescriptor_);
      } else {
        // Box was checked
        //  1. Set the protobuf field with a default value
        //  2. Set the widget with the same default value
        switch (fieldDescriptor_->type()) {
          case pb::FieldDescriptor::Type::TYPE_ENUM: {
            auto defaultEnumValue = fieldDescriptor_->default_value_enum();
            reflection->SetEnum(currentMessage_, fieldDescriptor_, defaultEnumValue);
            writeEnumIndexToWidget(dataWidget_, defaultEnumValue->index());
            break;
          }
          case pb::FieldDescriptor::Type::TYPE_BOOL: {
            const auto defaultValue = fieldDescriptor_->default_value_bool();
            reflection->SetBool(currentMessage_, fieldDescriptor_, defaultValue);
            writeBoolToWidget(dataWidget_, defaultValue);
            break;
          }
          case pb::FieldDescriptor::Type::TYPE_BYTES:
          case pb::FieldDescriptor::Type::TYPE_STRING: {
            const auto defaultValue = fieldDescriptor_->default_value_string();
            reflection->SetString(currentMessage_, fieldDescriptor_, defaultValue);
            writeStringToWidget(dataWidget_, defaultValue);
            break;
          }
          case pb::FieldDescriptor::Type::TYPE_FLOAT: {
            const auto defaultValue = fieldDescriptor_->default_value_float();
            reflection->SetFloat(currentMessage_, fieldDescriptor_, defaultValue);
            writeNumberToWidget(dataWidget_, defaultValue);
            break;
          }
          case pb::FieldDescriptor::Type::TYPE_DOUBLE: {
            const auto defaultValue = fieldDescriptor_->default_value_double();
            reflection->SetDouble(currentMessage_, fieldDescriptor_, defaultValue);
            writeNumberToWidget(dataWidget_, defaultValue);
            break;
          }
          case pb::FieldDescriptor::Type::TYPE_INT32:
          case pb::FieldDescriptor::Type::TYPE_SINT32:
          case pb::FieldDescriptor::Type::TYPE_SFIXED32: {
            const auto defaultValue = fieldDescriptor_->default_value_int32();
            reflection->SetInt32(currentMessage_, fieldDescriptor_, defaultValue);
            writeNumberToWidget(dataWidget_, defaultValue);
            break;
          }
          case pb::FieldDescriptor::Type::TYPE_UINT32:
          case pb::FieldDescriptor::Type::TYPE_FIXED32: {
            const auto defaultValue = fieldDescriptor_->default_value_uint32();
            reflection->SetUInt32(currentMessage_, fieldDescriptor_, defaultValue);
            writeNumberToWidget(dataWidget_, defaultValue);
            break;
          }
          case pb::FieldDescriptor::Type::TYPE_INT64:
          case pb::FieldDescriptor::Type::TYPE_SINT64:
          case pb::FieldDescriptor::Type::TYPE_SFIXED64: {
            const auto defaultValue = fieldDescriptor_->default_value_int64();
            reflection->SetInt64(currentMessage_, fieldDescriptor_, defaultValue);
            writeNumberToWidget(dataWidget_, defaultValue);
            break;
          }
          case pb::FieldDescriptor::Type::TYPE_UINT64:
          case pb::FieldDescriptor::Type::TYPE_FIXED64: {
            const auto defaultValue = fieldDescriptor_->default_value_uint64();
            reflection->SetUInt64(currentMessage_, fieldDescriptor_, defaultValue);
            writeNumberToWidget(dataWidget_, defaultValue);
            break;
          }
          default:
            throw std::runtime_error("Unhandled type");
            break;
        }
      }

      emit messageUpdated();
    });
  }
  
  // Add the widgets to the layout
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  layout->addWidget(labelWidget_);
  layout->addWidget(dataWidget_);
}

void BuiltInTypeWidget::setDataFromMessage() {
  if (currentMessage_ == nullptr) {
    throw std::runtime_error("Setting data from message, but message is null");
  }

  // Set whether the field is enabled or not
  if (fieldIsOptional()) {
    auto *labelAsCheckbox = dynamic_cast<QCheckBox*>(labelWidget_);
    if (labelAsCheckbox == nullptr) {
      throw std::runtime_error("Field is optional, but label is not of type QCheckBox");
    }
    const bool isSet = fieldIsSet();
    labelAsCheckbox->setChecked(isSet);

    // Set data based on what's in the message
    if (!isSet) {
      // Nothing else to do
      return;
    }
  }

  const pb::Reflection *reflection = currentMessage_->GetReflection();
  if (fieldDescriptor_->type() == pb::FieldDescriptor::Type::TYPE_ENUM) {
    auto *dataWidgetAsComboBox = dynamic_cast<QComboBox*>(dataWidget_);
    if (dataWidgetAsComboBox == nullptr) {
      throw std::runtime_error("For type enum expected data widget to be a QComboBox");
    }

    const pb::EnumValueDescriptor *enumValueDescriptor = reflection->GetEnum(*currentMessage_, fieldDescriptor_);
    // Note: This assumes that we added enum values into the combobox in order
    const int index = enumValueDescriptor->index();
    dataWidgetAsComboBox->setCurrentIndex(index);
  } else if (fieldDescriptor_->type() == pb::FieldDescriptor::Type::TYPE_BOOL) {
    auto *dataWidgetAsCheckBox = dynamic_cast<QCheckBox*>(dataWidget_);
    if (dataWidgetAsCheckBox == nullptr) {
      throw std::runtime_error("For type bool expected data widget to be a QCheckBox");
    }

    const bool isTrue = reflection->GetBool(*currentMessage_, fieldDescriptor_);
    dataWidgetAsCheckBox->setChecked(isTrue);
  } else {
    auto *dataWidgetAsLineEdit = dynamic_cast<QLineEdit*>(dataWidget_);
    if (dataWidgetAsLineEdit == nullptr) {
      throw std::runtime_error("Expected data widget to be a QLineEdit");
    }

    switch (fieldDescriptor_->type()) {
      case pb::FieldDescriptor::Type::TYPE_BYTES:
      case pb::FieldDescriptor::Type::TYPE_STRING: {
        const auto data = reflection->GetString(*currentMessage_, fieldDescriptor_);
        dataWidgetAsLineEdit->setText(QString::fromStdString(data));
        break;
      }
      case pb::FieldDescriptor::Type::TYPE_FLOAT: {
        const auto data = reflection->GetFloat(*currentMessage_, fieldDescriptor_);
        dataWidgetAsLineEdit->setText(QString::number(data));
        break;
      }
      case pb::FieldDescriptor::Type::TYPE_DOUBLE: {
        const auto data = reflection->GetDouble(*currentMessage_, fieldDescriptor_);
        dataWidgetAsLineEdit->setText(QString::number(data));
        break;
      }
      case pb::FieldDescriptor::Type::TYPE_INT32:
      case pb::FieldDescriptor::Type::TYPE_SINT32:
      case pb::FieldDescriptor::Type::TYPE_SFIXED32: {
        const auto data = reflection->GetInt32(*currentMessage_, fieldDescriptor_);
        dataWidgetAsLineEdit->setText(QString::number(data));
        break;
      }
      case pb::FieldDescriptor::Type::TYPE_UINT32:
      case pb::FieldDescriptor::Type::TYPE_FIXED32: {
        const auto data = reflection->GetUInt32(*currentMessage_, fieldDescriptor_);
        dataWidgetAsLineEdit->setText(QString::number(data));
        break;
      }
      case pb::FieldDescriptor::Type::TYPE_INT64:
      case pb::FieldDescriptor::Type::TYPE_SINT64:
      case pb::FieldDescriptor::Type::TYPE_SFIXED64: {
        const auto data = reflection->GetInt64(*currentMessage_, fieldDescriptor_);
        dataWidgetAsLineEdit->setText(QString::number(data));
        break;
      }
      case pb::FieldDescriptor::Type::TYPE_UINT64:
      case pb::FieldDescriptor::Type::TYPE_FIXED64: {
        const auto data = reflection->GetUInt64(*currentMessage_, fieldDescriptor_);
        dataWidgetAsLineEdit->setText(QString::number(data));
        break;
      }
      default:
        throw std::runtime_error("Unhandled type");
        break;
    }
  }
}

} // namespace protobuf_editor

namespace {

int parseEnumFromWidget(const QWidget *widget) {
  return {};
}

bool parseBoolFromWidget(const QWidget *widget) {
  const QCheckBox *checkBox = dynamic_cast<const QCheckBox*>(widget);
  if (checkBox == nullptr) {
    throw std::runtime_error("Parsing bool data from widget, but widget is not a checkbox");
  }
  return checkBox->isChecked();
}

void writeEnumIndexToWidget(QWidget *widget, const int index) {
  QComboBox *comboBox = dynamic_cast<QComboBox*>(widget);
  if (comboBox == nullptr) {
    throw std::runtime_error("Writing enum data to widget, but widget is not a combobox");
  }
  return comboBox->setCurrentIndex(index);
}

void writeBoolToWidget(QWidget *widget, const bool data) {
  QCheckBox *checkBox = dynamic_cast<QCheckBox*>(widget);
  if (checkBox == nullptr) {
    throw std::runtime_error("Writing data to widget, bool, but widget is not a checkbox");
  }
  return checkBox->setChecked(data);
}

void writeStringToWidget(QWidget *widget, const std::string &data) {
  QLineEdit *lineEdit = dynamic_cast<QLineEdit*>(widget);
  if (lineEdit == nullptr) {
    throw std::runtime_error("Writing string to widget, but widget is not a lineedit");
  }
  lineEdit->setText(QString::fromStdString(data));
}

} // anonymous namespace

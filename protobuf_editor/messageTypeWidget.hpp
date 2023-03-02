#ifndef PROTOBUF_EDITOR_MESSAGE_TYPE_WIDGET_HPP_
#define PROTOBUF_EDITOR_MESSAGE_TYPE_WIDGET_HPP_

#include "protobufFieldWidget.hpp"

#include <google/protobuf/message.h>

#include <QGroupBox>

#include <vector>

namespace protobuf_editor {

class MessageTypeWidget : public ProtobufFieldWidget {
  Q_OBJECT
public:
  MessageTypeWidget(const google::protobuf::Descriptor *descriptor, const google::protobuf::FieldDescriptor *fieldDescriptor=nullptr, QWidget *parent=nullptr);
private:
  const google::protobuf::Descriptor* const descriptor_;
  std::vector<ProtobufFieldWidget*> nestedWidgets_;
  QGroupBox *groupBox_{nullptr};
  void buildWidget();
  void setDataFromMessage() override;
signals:
};

} // namespace protobuf_editor

#endif // PROTOBUF_EDITOR_MESSAGE_TYPE_WIDGET_HPP_

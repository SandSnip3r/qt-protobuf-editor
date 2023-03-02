#ifndef PROTOBUF_EDITOR_PROTOBUF_FIELD_WIDGET_HPP_
#define PROTOBUF_EDITOR_PROTOBUF_FIELD_WIDGET_HPP_

#include <google/protobuf/message.h>

#include <QWidget>

namespace protobuf_editor {

class ProtobufFieldWidget : public QWidget {
  Q_OBJECT
public:
  explicit ProtobufFieldWidget(const google::protobuf::FieldDescriptor *fieldDescriptor=nullptr, QWidget *parent=nullptr);
  void setMessage(google::protobuf::Message *currentMessage, google::protobuf::Message *parentMessage=nullptr);
  virtual ~ProtobufFieldWidget() = 0;
protected:
  const google::protobuf::FieldDescriptor* const fieldDescriptor_;
  google::protobuf::Message *currentMessage_{nullptr};
  google::protobuf::Message *parentMessage_{nullptr};
  virtual void setDataFromMessage();
  bool fieldIsOptional() const;
  bool fieldIsSet() const;
private:
  bool fieldIsOptional_;
signals:
  void messageUpdated();
};

} // namespace protobuf_editor

#endif // PROTOBUF_EDITOR_PROTOBUF_FIELD_WIDGET_HPP_
